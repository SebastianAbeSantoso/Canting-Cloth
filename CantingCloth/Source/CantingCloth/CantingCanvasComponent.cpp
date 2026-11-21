#include "CantingCanvasComponent.h"

#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "TextureResource.h"

#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogCanting, Log, All);

// Stroke points are handed to CantingCore as interleaved doubles (x0, y0, x1, y1, ...).
static_assert(sizeof(FVector2D) == 2 * sizeof(double), "FVector2D must be two doubles");

namespace
{
	const double* AsXY(const TArray<FVector2D>& Points)
	{
		return reinterpret_cast<const double*>(Points.GetData());
	}

	/** Logs a message and, outside shipping builds, shows it on screen so problems are visible in PIE. */
	void Report(const FString& Message, bool bWarning)
	{
		if (bWarning)
		{
			UE_LOG(LogCanting, Warning, TEXT("%s"), *Message);
		}
		else
		{
			UE_LOG(LogCanting, Log, TEXT("%s"), *Message);
		}
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, bWarning ? 20.0f : 6.0f, bWarning ? FColor::Orange : FColor::Cyan, Message);
		}
#endif
	}

	bool HasTextureParameter(UMaterialInterface* Material, FName Name)
	{
		if (!Material)
		{
			return false;
		}
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		Material->GetAllTextureParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			if (Info.Name == Name)
			{
				return true;
			}
		}
		return false;
	}
}

UCantingCanvasComponent::UCantingCanvasComponent()
{
	// Ticks only while a stroke is open, to end it when DrawAtUV stops arriving.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCantingCanvasComponent::BeginPlay()
{
	Super::BeginPlay();
	InitCanvas();
	ConnectMaterial();
}

void UCantingCanvasComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// No DrawAtUV for a whole frame: the mouse was released or left the cloth.
	if (bStrokeActive && GFrameCounter > LastDrawFrame + 1)
	{
		EndStroke();
	}
}

void UCantingCanvasComponent::InitCanvas()
{
	const double PanelX = FMath::Max(PanelSizeCm.X, 1.0);
	const double PanelY = FMath::Max(PanelSizeCm.Y, 1.0);
	const int32 TextureWidth = FMath::Clamp(GridResolution, 64, 2048);
	TexelsPerCm = static_cast<double>(TextureWidth) / PanelX;
	const int32 TextureHeight = FMath::Clamp(FMath::RoundToInt32(PanelY * TexelsPerCm), 1, 4096);

	Grid.Init(TextureWidth, TextureHeight);
	SealedMask.assign(static_cast<size_t>(TextureWidth) * static_cast<size_t>(TextureHeight), uint8_t(0));
	FilterMask.assign(SealedMask.size(), uint8_t(0));
	DeflectMask.assign(SealedMask.size(), uint8_t(0));
	Strokes.Reset();
	Stats = FCantingInscriptionStats();
	SetStrokeActive(false);

	WaxTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_B8G8R8A8);
	if (!WaxTexture)
	{
		Report(TEXT("CantingCanvas: could not create the wax texture."), true);
		return;
	}
	WaxTexture->SRGB = false;           // the texture stores distances, not colours
	WaxTexture->Filter = TF_Bilinear;
	WaxTexture->AddressX = TA_Clamp;
	WaxTexture->AddressY = TA_Clamp;

	// Put the clean cloth into the texture's own memory before the GPU copy is made, so the first frames
	// can't show leftover bytes as wax even if the first region update arrives late.
	if (FTexturePlatformData* PlatformData = WaxTexture->GetPlatformData())
	{
		if (PlatformData->Mips.Num() > 0)
		{
			FTexture2DMipMap& Mip = PlatformData->Mips[0];
			uint8* MipData = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
			if (MipData && Mip.BulkData.GetBulkDataSize() >= static_cast<int64>(TextureWidth) * TextureHeight * 4)
			{
				PackRect(Grid.FullRect(), MipData, TextureWidth * 4);
			}
			Mip.BulkData.Unlock();
		}
	}
	WaxTexture->UpdateResource();
	UploadRect(Grid.FullRect());
}

void UCantingCanvasComponent::ConnectMaterial()
{
	AActor* OwnerActor = GetOwner();
	UStaticMeshComponent* Mesh = OwnerActor ? OwnerActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!Mesh || !WaxTexture)
	{
		Report(TEXT("CantingCanvas: the cloth actor has no Static Mesh component, so the wax won't be visible."), true);
		return;
	}
	ClothMaterial = Mesh->CreateDynamicMaterialInstance(MaterialIndex);
	if (!ClothMaterial)
	{
		Report(FString::Printf(TEXT("CantingCanvas: material slot %d on %s has no material."), MaterialIndex, *Mesh->GetName()), true);
		return;
	}
	ClothMaterial->SetTextureParameterValue(TextureParameterName, WaxTexture);
	if (!HasTextureParameter(ClothMaterial.Get(), TextureParameterName))
	{
		Report(FString::Printf(
			TEXT("CantingCanvas: %s has no texture parameter named %s, so the wax won't show. In the material, right-click the wax Texture Sample, choose Convert to Parameter, and name it %s."),
			*GetNameSafe(ClothMaterial->GetMaterial()), *TextureParameterName.ToString(), *TextureParameterName.ToString()), true);
	}
}

void UCantingCanvasComponent::SetStrokeActive(bool bActive)
{
	bStrokeActive = bActive;
	SetComponentTickEnabled(bActive);
}

void UCantingCanvasComponent::DrawAtUV(FVector2D UV)
{
	if (!Grid.IsValid())
	{
		return;
	}

	// A missed frame means the cursor left the cloth (or drawing was blocked) and has come back.
	// End the old stroke there, so the line doesn't jump across the gap.
	const uint64 Frame = GFrameCounter;
	if (bStrokeActive && Frame > LastDrawFrame + 1)
	{
		EndStroke();
	}
	LastDrawFrame = Frame;

	const FVector2D PointCm(FMath::Clamp(UV.X, 0.0, 1.0) * PanelSizeCm.X, FMath::Clamp(UV.Y, 0.0, 1.0) * PanelSizeCm.Y);
	LastSampleCm = PointCm;

	if (!bStrokeActive)
	{
		FCantingStroke& NewStroke = Strokes.AddDefaulted_GetRef();
		NewStroke.RadiusCm = BrushRadiusCm;
		NewStroke.PointsCm.Add(PointCm);
		SetStrokeActive(true);
		UploadRect(StampLastPoints(NewStroke, 1));
		return;
	}

	FCantingStroke& Stroke = Strokes.Last();
	if (FVector2D::Distance(Stroke.PointsCm.Last(), PointCm) < MinPointSpacingCm)
	{
		return;
	}
	Stroke.PointsCm.Add(PointCm);
	UploadRect(StampLastPoints(Stroke, 2));
}

void UCantingCanvasComponent::EndStroke()
{
	if (!bStrokeActive)
	{
		return;
	}
	SetStrokeActive(false);

	// Reach the last spot the cursor touched, even if it was closer than MinPointSpacingCm.
	FCantingStroke& Stroke = Strokes.Last();
	if (FVector2D::Distance(Stroke.PointsCm.Last(), LastSampleCm) > 0.01)
	{
		Stroke.PointsCm.Add(LastSampleCm);
		UploadRect(StampLastPoints(Stroke, 2));
	}
	RefreshAnalysis();
}

bool UCantingCanvasComponent::UndoLastStroke()
{
	if (Strokes.Num() == 0 || !Grid.IsValid())
	{
		return false;
	}
	const CantingCore::FRectI Affected = StrokeBounds(Strokes.Last());
	Strokes.Pop();
	SetStrokeActive(false);
	RebuildRegion(Affected);
	RefreshAnalysis();
	return true;
}

void UCantingCanvasComponent::ClearInscription()
{
	if (!Grid.IsValid())
	{
		return;
	}
	Strokes.Reset();
	SetStrokeActive(false);
	Grid.Clear(Grid.FullRect());
	RefreshAnalysis();
}

FString UCantingCanvasComponent::DrawingPath(const FString& Name)
{
	const FString FileName = FPaths::MakeValidFileName(Name.IsEmpty() ? FString(TEXT("drawing")) : Name, TEXT('_')) + TEXT(".txt");
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("CantingDrawings"), FileName));
}

bool UCantingCanvasComponent::SaveStrokesToFile(const FString& Name, FString& OutPath)
{
	EndStroke();
	OutPath = DrawingPath(Name);

	// Plain text, one stroke per line, so drawings can be read by the engine-free tests too. Numbers are written
	// with full precision, so a loaded drawing gives exactly the same wax and stats as the original:
	//   CantingStrokes 1
	//   panel <width cm> <height cm>
	//   stroke <radius cm> <x0> <y0> <x1> <y1> ...
	FString Text = FString::Printf(TEXT("CantingStrokes 1\npanel %.17g %.17g\n"), PanelSizeCm.X, PanelSizeCm.Y);
	for (const FCantingStroke& Stroke : Strokes)
	{
		Text += FString::Printf(TEXT("stroke %.9g"), Stroke.RadiusCm);
		for (const FVector2D& Point : Stroke.PointsCm)
		{
			Text += FString::Printf(TEXT(" %.17g %.17g"), Point.X, Point.Y);
		}
		Text += TEXT("\n");
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutPath), true);
	if (!FFileHelper::SaveStringToFile(Text, *OutPath))
	{
		Report(FString::Printf(TEXT("CantingCanvas: could not write %s"), *OutPath), true);
		return false;
	}
	Report(FString::Printf(TEXT("CantingCanvas: saved %d strokes to %s"), Strokes.Num(), *OutPath), false);
	return true;
}

bool UCantingCanvasComponent::LoadStrokesFromFile(const FString& Name)
{
	if (!Grid.IsValid())
	{
		return false;
	}
	const FString Path = DrawingPath(Name);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		Report(FString::Printf(TEXT("CantingCanvas: could not read %s"), *Path), true);
		return false;
	}

	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	TArray<FCantingStroke> Loaded;
	FVector2D FilePanel = PanelSizeCm;
	for (const FString& Line : Lines)
	{
		TArray<FString> Parts;
		Line.ParseIntoArrayWS(Parts);
		if (Parts.Num() >= 3 && Parts[0] == TEXT("panel"))
		{
			FilePanel = FVector2D(FCString::Atod(*Parts[1]), FCString::Atod(*Parts[2]));
		}
		else if (Parts.Num() >= 4 && Parts[0] == TEXT("stroke"))
		{
			FCantingStroke& Stroke = Loaded.AddDefaulted_GetRef();
			Stroke.RadiusCm = FCString::Atof(*Parts[1]);
			for (int32 Index = 2; Index + 1 < Parts.Num(); Index += 2)
			{
				Stroke.PointsCm.Add(FVector2D(FCString::Atod(*Parts[Index]), FCString::Atod(*Parts[Index + 1])));
			}
		}
	}
	if (!FilePanel.Equals(PanelSizeCm, 0.01))
	{
		Report(FString::Printf(TEXT("CantingCanvas: %s was drawn on a %.0f x %.0f cm panel; this one is %.0f x %.0f cm. Loading it as it is."),
			*Path, FilePanel.X, FilePanel.Y, PanelSizeCm.X, PanelSizeCm.Y), true);
	}

	Strokes = MoveTemp(Loaded);
	SetStrokeActive(false);
	Grid.Clear(Grid.FullRect());
	for (const FCantingStroke& Stroke : Strokes)
	{
		StampStroke(Stroke, nullptr);
	}
	RefreshAnalysis();
	Report(FString::Printf(TEXT("CantingCanvas: loaded %d strokes from %s"), Strokes.Num(), *Path), false);
	return true;
}

CantingCore::FRectI UCantingCanvasComponent::StampStroke(const FCantingStroke& Stroke, const CantingCore::FRectI* Clip)
{
	return Grid.StampPolyline(AsXY(Stroke.PointsCm), Stroke.PointsCm.Num(), TexelsPerCm,
		static_cast<float>(Stroke.RadiusCm * TexelsPerCm), Clip);
}

CantingCore::FRectI UCantingCanvasComponent::StampLastPoints(const FCantingStroke& Stroke, int32 NumPoints)
{
	const int32 Count = FMath::Min(NumPoints, Stroke.PointsCm.Num());
	if (Count <= 0)
	{
		return CantingCore::FRectI();
	}
	const double* First = AsXY(Stroke.PointsCm) + 2 * (Stroke.PointsCm.Num() - Count);
	return Grid.StampPolyline(First, Count, TexelsPerCm, static_cast<float>(Stroke.RadiusCm * TexelsPerCm));
}

CantingCore::FRectI UCantingCanvasComponent::StrokeBounds(const FCantingStroke& Stroke) const
{
	return CantingCore::FWaxGrid::PolylineBounds(AsXY(Stroke.PointsCm), Stroke.PointsCm.Num(), TexelsPerCm,
		static_cast<float>(Stroke.RadiusCm * TexelsPerCm));
}

void UCantingCanvasComponent::RebuildRegion(const CantingCore::FRectI& Region)
{
	const CantingCore::FRectI Clip = Region.ClippedTo(Grid.FullRect());
	if (Clip.IsEmpty())
	{
		return;
	}
	Grid.Clear(Clip);
	for (const FCantingStroke& Stroke : Strokes)
	{
		if (StrokeBounds(Stroke).Intersects(Clip))
		{
			StampStroke(Stroke, &Clip);
		}
	}
}

void UCantingCanvasComponent::RefreshAnalysis()
{
	const double TexelsPerCm2 = TexelsPerCm * TexelsPerCm;
	const float ToleranceTexels = static_cast<float>(0.5 * SealGapToleranceCm * TexelsPerCm);
	const int32 MinAreaTexels = FMath::Max(1, FMath::CeilToInt32(MinEnclosedAreaCm2 * TexelsPerCm2));
	const CantingCore::FRegionResult Regions = CantingCore::AnalyseRegions(Grid, ToleranceTexels, MinAreaTexels, SealedMask);

	double InkCm = 0.0;
	for (const FCantingStroke& Stroke : Strokes)
	{
		InkCm += CantingCore::PolylineLength(AsXY(Stroke.PointsCm), Stroke.PointsCm.Num());
	}
	const double TotalTexels = static_cast<double>(Grid.GetWidth()) * static_cast<double>(Grid.GetHeight());

	Stats.StrokeCount = Strokes.Num();
	Stats.InkLengthCm = static_cast<float>(InkCm);
	Stats.EnclosedShapes = Regions.EnclosedRegions;
	Stats.EnclosedAreaCm2 = static_cast<float>(static_cast<double>(Regions.EnclosedTexels) / TexelsPerCm2);
	Stats.WaxCoverage = static_cast<float>(static_cast<double>(Regions.WaxTexels) / TotalTexels);

	// Which ward the drawing makes: sealed cells with curved walls feed the filter ward, straight walls the deflect ward.
	Stats.bWardsRead = bReadWards;
	Stats.FilterFill = Stats.DeflectFill = 0.0f;
	Stats.FilterCells = Stats.DeflectCells = Stats.MixedCells = 0;
	if (bReadWards)
	{
		CantingCore::FWardParams Params;
		Params.TexelsPerCm = TexelsPerCm;
		Params.BrushRadiusCm = BrushRadiusCm;
		Params.GapToleranceCm = SealGapToleranceCm;
		Params.MinCellCm2 = FMath::Max(static_cast<double>(MinEnclosedAreaCm2), 1.0);
		Params.MaxCellCm2 = MaxCellAreaCm2;
		const CantingCore::FWardReading Reading = CantingCore::ReadWards(Grid, Params, &FilterMask, &DeflectMask);
		const double Full = FMath::Max(static_cast<double>(FullWardCellAreaCm2), 1.0);
		Stats.FilterFill = static_cast<float>(Reading.FilterPowerCm2 / Full);
		Stats.DeflectFill = static_cast<float>(Reading.DeflectPowerCm2 / Full);
		Stats.FilterCells = Reading.FilterCells;
		Stats.DeflectCells = Reading.DeflectCells;
		Stats.MixedCells = Reading.MixedCells;
	}
	else
	{
		std::fill(FilterMask.begin(), FilterMask.end(), uint8_t(0));
		std::fill(DeflectMask.begin(), DeflectMask.end(), uint8_t(0));
	}
#if !UE_BUILD_SHIPPING
	if (bReadWards && bShowWardReading && GEngine)
	{
		const bool bFilter = Stats.FilterFill >= Stats.DeflectFill;
		const FString Ward = (Stats.FilterFill + Stats.DeflectFill) <= 0.0f ? FString(TEXT("no ward yet (no sealed cells)"))
			: FString::Printf(TEXT("%s ward"), bFilter ? TEXT("FILTER (poison)") : TEXT("DEFLECT (physical)"));
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 8.0f, FColor::Cyan, FString::Printf(
			TEXT("Ward reading: %s | filter %d%% (%d cells), deflect %d%% (%d cells), %d in between | wax %d%%"),
			*Ward, FMath::RoundToInt32(100.0f * Stats.FilterFill), Stats.FilterCells, FMath::RoundToInt32(100.0f * Stats.DeflectFill),
			Stats.DeflectCells, Stats.MixedCells, FMath::RoundToInt32(100.0f * Stats.WaxCoverage)));
	}
#endif

	// The cell masks can change anywhere, so send the whole texture (512 x 512 x 4 = 1 MB, once per stroke).
	UploadRect(Grid.FullRect());
	OnInscriptionChanged.Broadcast(Stats);
}

void UCantingCanvasComponent::PackRect(const CantingCore::FRectI& Rect, uint8* Dest, int32 DestPitch) const
{
	const std::vector<uint8_t>& Distance = Grid.GetData();
	const size_t GridWidth = static_cast<size_t>(Grid.GetWidth());
	for (int32 Y = Rect.Y0; Y < Rect.Y1; ++Y)
	{
		uint8* Out = Dest + static_cast<size_t>(Y - Rect.Y0) * static_cast<size_t>(DestPitch);
		for (int32 X = Rect.X0; X < Rect.X1; ++X)
		{
			const size_t Index = static_cast<size_t>(Y) * GridWidth + static_cast<size_t>(X);
			*Out++ = DeflectMask[Index];                         // B: deflect share inside sealed cells
			*Out++ = FilterMask[Index];                          // G: filter share inside sealed cells
			*Out++ = static_cast<uint8>(255 - Distance[Index]);  // R: wax, 0 = none (the grid stores 255 = none)
			*Out++ = 255;                                        // A
		}
	}
}

void UCantingCanvasComponent::UploadRect(const CantingCore::FRectI& InRect)
{
	const CantingCore::FRectI Rect = InRect.ClippedTo(Grid.FullRect());
	if (!WaxTexture || !WaxTexture->GetResource() || Rect.IsEmpty())
	{
		return;
	}
	const int32 RectWidth = Rect.X1 - Rect.X0;
	const int32 RectHeight = Rect.Y1 - Rect.Y0;
	const int32 Pitch = RectWidth * 4;

	// The upload runs later on the render thread, so it gets its own copy, freed by the callback.
	uint8* Buffer = static_cast<uint8*>(FMemory::Malloc(static_cast<SIZE_T>(Pitch) * static_cast<SIZE_T>(RectHeight)));
	PackRect(Rect, Buffer, Pitch);
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(
		static_cast<uint32>(Rect.X0), static_cast<uint32>(Rect.Y0), 0, 0,
		static_cast<uint32>(RectWidth), static_cast<uint32>(RectHeight));

	WaxTexture->UpdateTextureRegions(0, 1, Region, static_cast<uint32>(Pitch), 4, Buffer,
		[](uint8* Data, const FUpdateTextureRegion2D* UsedRegion)
		{
			FMemory::Free(Data);
			delete UsedRegion;
		});
}

// ---------------------------------------------------------------------------------------------------------------------
// Console commands for collecting and replaying drawings (not in shipping builds)

#if !UE_BUILD_SHIPPING
namespace
{
	UCantingCanvasComponent* FindCanvas(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UCantingCanvasComponent* Canvas = It->FindComponentByClass<UCantingCanvasComponent>())
			{
				return Canvas;
			}
		}
		return nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GCantingSaveStrokesCommand(
		TEXT("Canting.SaveStrokes"),
		TEXT("Saves the cloth's strokes to <project>/CantingDrawings/<name>.txt. Usage: Canting.SaveStrokes <name>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCantingCanvasComponent* Canvas = FindCanvas(World))
			{
				FString Path;
				Canvas->SaveStrokesToFile(Args.Num() > 0 ? Args[0] : FString(TEXT("drawing")), Path);
			}
			else
			{
				UE_LOG(LogCanting, Warning, TEXT("Canting.SaveStrokes: no Canting Canvas in this world. Run it while playing."));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GCantingLoadStrokesCommand(
		TEXT("Canting.LoadStrokes"),
		TEXT("Replaces the cloth's drawing with <project>/CantingDrawings/<name>.txt. Usage: Canting.LoadStrokes <name>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCantingCanvasComponent* Canvas = FindCanvas(World))
			{
				Canvas->LoadStrokesFromFile(Args.Num() > 0 ? Args[0] : FString(TEXT("drawing")));
			}
			else
			{
				UE_LOG(LogCanting, Warning, TEXT("Canting.LoadStrokes: no Canting Canvas in this world. Run it while playing."));
			}
		}));
}
#endif
