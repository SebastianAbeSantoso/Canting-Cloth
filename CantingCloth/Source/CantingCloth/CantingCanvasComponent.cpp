#include "CantingCanvasComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RHI.h"

// Stroke points are handed to CantingCore as interleaved doubles (x0, y0, x1, y1, ...).
static_assert(sizeof(FVector2D) == 2 * sizeof(double), "FVector2D must be two doubles");

namespace
{
	const double* AsXY(const TArray<FVector2D>& Points)
	{
		return reinterpret_cast<const double*>(Points.GetData());
	}
}

UCantingCanvasComponent::UCantingCanvasComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCantingCanvasComponent::BeginPlay()
{
	Super::BeginPlay();
	InitCanvas();
	ConnectMaterial();
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
	Strokes.Reset();
	Stats = FCantingInscriptionStats();
	bStrokeActive = false;

	WaxTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_B8G8R8A8);
	if (!WaxTexture)
	{
		UE_LOG(LogTemp, Error, TEXT("CantingCanvas: could not create the wax texture."));
		return;
	}
	WaxTexture->SRGB = false;           // the texture stores distances, not colours
	WaxTexture->Filter = TF_Bilinear;
	WaxTexture->AddressX = TA_Clamp;
	WaxTexture->AddressY = TA_Clamp;
	WaxTexture->UpdateResource();
	UploadRect(Grid.FullRect());
}

void UCantingCanvasComponent::ConnectMaterial()
{
	AActor* OwnerActor = GetOwner();
	UStaticMeshComponent* Mesh = OwnerActor ? OwnerActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!Mesh || !WaxTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("CantingCanvas: no Static Mesh component on the owner, wax will not be visible."));
		return;
	}
	ClothMaterial = Mesh->CreateDynamicMaterialInstance(MaterialIndex);
	if (!ClothMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("CantingCanvas: material slot %d has no material."), MaterialIndex);
		return;
	}
	ClothMaterial->SetTextureParameterValue(TextureParameterName, WaxTexture);
}

void UCantingCanvasComponent::DrawAtUV(FVector2D UV)
{
	if (!Grid.IsValid())
	{
		return;
	}
	const FVector2D PointCm(FMath::Clamp(UV.X, 0.0, 1.0) * PanelSizeCm.X, FMath::Clamp(UV.Y, 0.0, 1.0) * PanelSizeCm.Y);
	LastSampleCm = PointCm;

	if (!bStrokeActive)
	{
		FCantingStroke& NewStroke = Strokes.AddDefaulted_GetRef();
		NewStroke.RadiusCm = BrushRadiusCm;
		NewStroke.PointsCm.Add(PointCm);
		bStrokeActive = true;
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
	bStrokeActive = false;

	// Reach the exact spot where the mouse was released, even if it was closer than MinPointSpacingCm.
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
	bStrokeActive = false;
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
	bStrokeActive = false;
	Grid.Clear(Grid.FullRect());
	RefreshAnalysis();
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

	// The sealed mask can change anywhere, so send the whole texture (512 x 512 x 4 = 1 MB, once per stroke).
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
			*Out++ = 0;                  // B
			*Out++ = SealedMask[Index];  // G: sealed region
			*Out++ = Distance[Index];    // R: wax distance
			*Out++ = 255;                // A
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
