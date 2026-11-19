#include "CraftLoopSubsystem.h"

#include "CantingCanvasComponent.h"
#include "CraftLoopHUD.h"
#include "CraftRules.h"
#include "CraftSettings.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TextureResource.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "CraftLoop"

namespace
{
	constexpr float DyeSeconds = 1.5f;
	constexpr float StitchSeconds = 1.5f;
	constexpr float StitchedScale = 0.6f;
	constexpr float StitchedLiftCm = 15.0f;
	constexpr int32 LegacyMaskSize = 256;
}

// ---------------------------------------------------------------------------------------------------------------------
// Lifecycle

bool UCraftLoopSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

void UCraftLoopSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UCraftSettings::Get()->Commissions.Num() == 0)
	{
		UE_LOG(LogCraftLoop, Warning, TEXT("No commissions in Project Settings > Canting Cloth Craft Loop; the craft loop is idle."));
		return;
	}

	if (AActor* Found = FindCloth())
	{
		ClothStartTransform = Found->GetActorTransform();
		UE_LOG(LogCraftLoop, Log, TEXT("Craft loop using cloth actor %s (%s)."), *Found->GetName(),
			GetCanvas() ? TEXT("Canting Canvas component") : TEXT("legacy render target"));
	}
	else
	{
		UE_LOG(LogCraftLoop, Warning, TEXT("No cloth actor found; canting will measure nothing."));
	}

	BeginCommission();
	if (UCraftSettings::Get()->bShowBuiltInUI)
	{
		ShowHUD();
	}
}

void UCraftLoopSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AnimationTimer);
		World->GetTimerManager().ClearTimer(ExpeditionTimer);
		World->GetTimerManager().ClearTimer(AutoPlayTimer);
	}
	HideHUD();
	SetClothDrawable(true);
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------------------------------------------------
// Stage flow

void UCraftLoopSubsystem::BeginCommission()
{
	Garment = FCraftGarment();
	Placements.Reset();
	bDyeChosen = false;
	bDipping = false;
	FinalDipDepth = 0.0f;
	bStitching = false;
	StitchScores.Reset();
	Expedition = FCraftExpeditionResult();
	RevealedEvents = 0;
	bRewardsCollected = false;
	ResetCloth();
	SetStage(ECraftStage::Commission);
}

void UCraftLoopSubsystem::SetStage(ECraftStage NewStage)
{
	Stage = NewStage;
	SetClothDrawable(Stage == ECraftStage::Canting);
	UE_LOG(LogCraftLoop, Log, TEXT("Stage -> %s (hour %d)"), *UEnum::GetValueAsString(Stage), Hour);
	OnStageChanged.Broadcast(Stage);
	Broadcast();
}

void UCraftLoopSubsystem::AdvanceStage()
{
	if (Stage != ECraftStage::Finished)
	{
		SetStage(static_cast<ECraftStage>(static_cast<uint8>(Stage) + 1));
	}
}

void UCraftLoopSubsystem::SpendTime(int32 Hours, const FText& Label)
{
	Hour += FMath::Max(Hours, 0);
	OnTimeChanged.Broadcast(Hour, Label);
	CheckScheduledEvents();
}

void UCraftLoopSubsystem::CheckScheduledEvents()
{
	if (!bIndigoDelivered && Hour >= UCraftSettings::Get()->IndigoDeliveryHour)
	{
		bIndigoDelivered = true;
		Notify(LOCTEXT("IndigoArrives", "Bu Sari from the next street drops off a pot of fermented indigo. A deeper, steadier dye is now available."));
	}
}

void UCraftLoopSubsystem::Notify(const FText& Message)
{
	UE_LOG(LogCraftLoop, Log, TEXT("Notice: %s"), *Message.ToString());
	OnNotice.Broadcast(Message);
	if (HUD.IsValid())
	{
		HUD->ShowNotice(Message);
	}
}

void UCraftLoopSubsystem::Broadcast()
{
	OnLoopUpdated.Broadcast();
	if (HUD.IsValid())
	{
		HUD->Refresh();
	}
}

bool UCraftLoopSubsystem::RequireStage(ECraftStage Expected, const TCHAR* Action) const
{
	if (Stage != Expected || bBusy)
	{
		UE_LOG(LogCraftLoop, Verbose, TEXT("%s ignored: stage is %s%s."), Action, *UEnum::GetValueAsString(Stage), bBusy ? TEXT(" (busy)") : TEXT(""));
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// Actions

void UCraftLoopSubsystem::AcceptCommission()
{
	if (RequireStage(ECraftStage::Commission, TEXT("AcceptCommission")))
	{
		AdvanceStage();
	}
}

bool UCraftLoopSubsystem::ChooseBlueprint(FName BlueprintId)
{
	FCraftCommission Commission;
	if (!RequireStage(ECraftStage::SelectBlueprint, TEXT("ChooseBlueprint")) || !GetCurrentCommission(Commission)
		|| !UCraftSettings::Get()->FindBlueprint(BlueprintId))
	{
		return false;
	}
	if (Commission.AllowedBlueprints.Num() > 0 && !Commission.AllowedBlueprints.Contains(BlueprintId))
	{
		Notify(LOCTEXT("WrongBlueprint", "That isn't what the customer asked for."));
		return false;
	}
	Garment.BlueprintId = BlueprintId;
	AdvanceStage();
	return true;
}

bool UCraftLoopSubsystem::ChooseFabric(FName FabricId)
{
	if (!RequireStage(ECraftStage::SelectCloth, TEXT("ChooseFabric")) || !UCraftSettings::Get()->FindFabric(FabricId))
	{
		return false;
	}
	Garment.FabricId = FabricId;
	FCraftGarmentBlueprint Blueprint;
	GetChosenBlueprint(Blueprint);
	Placements.Init(FCraftPiecePlacement(), Blueprint.Pieces.Num());
	AdvanceStage();
	return true;
}

bool UCraftLoopSubsystem::PlacePiece(int32 PieceIndex, int32 X, int32 Y, bool bRotated)
{
	FCraftGarmentBlueprint Blueprint;
	FCraftFabric Fabric;
	if (!RequireStage(ECraftStage::Arrange, TEXT("PlacePiece")) || !GetChosenBlueprint(Blueprint) || !GetChosenFabric(Fabric)
		|| !Placements.IsValidIndex(PieceIndex))
	{
		return false;
	}
	FCraftPiecePlacement Candidate;
	Candidate.bPlaced = true;
	Candidate.X = X;
	Candidate.Y = Y;
	Candidate.bRotated = bRotated;
	if (!UCraftRules::IsPlacementValid(Fabric, Blueprint, Placements, PieceIndex, Candidate))
	{
		return false;
	}
	Placements[PieceIndex] = Candidate;
	Broadcast();
	return true;
}

void UCraftLoopSubsystem::RemovePiece(int32 PieceIndex)
{
	if (RequireStage(ECraftStage::Arrange, TEXT("RemovePiece")) && Placements.IsValidIndex(PieceIndex))
	{
		Placements[PieceIndex].bPlaced = false;
		Broadcast();
	}
}

bool UCraftLoopSubsystem::CanCommitArrangement() const
{
	if (Stage != ECraftStage::Arrange || Placements.Num() == 0)
	{
		return false;
	}
	for (const FCraftPiecePlacement& Placement : Placements)
	{
		if (!Placement.bPlaced)
		{
			return false;
		}
	}
	return true;
}

bool UCraftLoopSubsystem::CommitArrangement()
{
	FCraftGarmentBlueprint Blueprint;
	FCraftFabric Fabric;
	if (!RequireStage(ECraftStage::Arrange, TEXT("CommitArrangement")) || !CanCommitArrangement()
		|| !GetChosenBlueprint(Blueprint) || !GetChosenFabric(Fabric))
	{
		return false;
	}
	UCraftRules::EvaluateLayout(Fabric, Blueprint, Placements, Garment.CrossGrainFraction, Garment.LayoutWaste);
	SpendTime(UCraftSettings::Get()->CostArrange, LOCTEXT("TimeArrange", "Laid out the pattern"));
	AdvanceStage();
	return true;
}

void UCraftLoopSubsystem::SetCutLength(float CutLength)
{
	if (Stage == ECraftStage::Cut && !bBusy)
	{
		Garment.CutLength = FMath::Clamp(CutLength, UCraftRules::MinCutLength, UCraftRules::MaxCutLength);
	}
}

void UCraftLoopSubsystem::CommitCut()
{
	if (RequireStage(ECraftStage::Cut, TEXT("CommitCut")))
	{
		SpendTime(UCraftSettings::Get()->CostCut, LOCTEXT("TimeCut", "Cut the panels"));
		AdvanceStage();
	}
}

void UCraftLoopSubsystem::FinishCanting()
{
	if (!RequireStage(ECraftStage::Canting, TEXT("FinishCanting")))
	{
		return;
	}
	if (UCantingCanvasComponent* Canvas = GetCanvas())
	{
		Canvas->EndStroke();
	}

	int32 Strokes = 0;
	float Coverage = 0.0f;
	int32 Enclosed = 0;
	MeasureCloth(Strokes, Coverage, Enclosed);

	const FCraftFabric* Fabric = UCraftSettings::Get()->FindFabric(Garment.FabricId);
	Garment.Inscription = UCraftRules::InterpretInscription(Strokes, Coverage, Enclosed, Fabric ? *Fabric : FCraftFabric(), false, KnownMotifs.Num());
	UE_LOG(LogCraftLoop, Log, TEXT("Inscription: strokes %d, coverage %.3f, enclosed %d -> strength %.2f, stability %.2f"),
		Strokes, Coverage, Enclosed, Garment.Inscription.Strength, Garment.Inscription.Stability);

	SpendTime(UCraftSettings::Get()->CostCanting, LOCTEXT("TimeCanting", "Drew the inscription"));
	AdvanceStage();
}

void UCraftLoopSubsystem::ClearCanting()
{
	if (!RequireStage(ECraftStage::Canting, TEXT("ClearCanting")))
	{
		return;
	}
	if (UCantingCanvasComponent* Canvas = GetCanvas())
	{
		Canvas->ClearInscription();
	}
	else if (UTextureRenderTarget2D* Target = UCraftSettings::Get()->LegacyWaxRenderTarget.LoadSynchronous())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, Target, UCraftSettings::Get()->LegacyClearColor);
	}
	Broadcast();
}

bool UCraftLoopSubsystem::UndoCantingStroke()
{
	UCantingCanvasComponent* Canvas = GetCanvas();
	if (!RequireStage(ECraftStage::Canting, TEXT("UndoCantingStroke")) || !Canvas)
	{
		return false;
	}
	const bool bUndone = Canvas->UndoLastStroke();
	Broadcast();
	return bUndone;
}

bool UCraftLoopSubsystem::ChooseDye(bool bIndigo)
{
	if (!RequireStage(ECraftStage::Dye, TEXT("ChooseDye")) || bDipping || (bIndigo && !bIndigoDelivered))
	{
		return false;
	}
	Garment.bIndigoDye = bIndigo;
	bDyeChosen = true;
	const UCraftSettings* Settings = UCraftSettings::Get();
	if (UMaterialInstanceDynamic* Material = GetClothMaterial())
	{
		Material->SetVectorParameterValue(Settings->DyeColorParameterName, bIndigo ? Settings->IndigoColor : Settings->SogaColor);
	}
	Broadcast();
	return true;
}

void UCraftLoopSubsystem::StartDip()
{
	if (RequireStage(ECraftStage::Dye, TEXT("StartDip")) && bDyeChosen && !bDipping)
	{
		bDipping = true;
		DipStartTime = Now();
		Broadcast();
	}
}

void UCraftLoopSubsystem::StopDip()
{
	if (Stage == ECraftStage::Dye && bDipping)
	{
		FinishDip(GetDipDepth());
	}
}

float UCraftLoopSubsystem::GetDipDepth() const
{
	if (!bDipping)
	{
		return FinalDipDepth;
	}
	return FMath::Clamp(static_cast<float>((Now() - DipStartTime) * UCraftSettings::Get()->DipRate), 0.0f, 1.0f);
}

void UCraftLoopSubsystem::FinishDip(float Depth)
{
	const UCraftSettings* Settings = UCraftSettings::Get();
	bDipping = false;
	FinalDipDepth = FMath::Clamp(Depth, 0.0f, 1.0f);
	Garment.DyeQuality = UCraftRules::DyeQualityFromDepth(FinalDipDepth, Settings->DipGoodMin, Settings->DipGoodMax);
	const bool bIndigo = Garment.bIndigoDye;
	UE_LOG(LogCraftLoop, Log, TEXT("Dip depth %.2f -> dye quality %.2f"), FinalDipDepth, Garment.DyeQuality);

	// Re-read the inscription with the dye's effect on the resist.
	const FCraftFabric* Fabric = Settings->FindFabric(Garment.FabricId);
	const FCraftInscription Measured = Garment.Inscription;
	Garment.Inscription = UCraftRules::InterpretInscription(Measured.StrokeCount, Measured.WaxCoverage, Measured.EnclosedShapes,
		Fabric ? *Fabric : FCraftFabric(), bIndigo, KnownMotifs.Num(), Garment.DyeQuality);

	if (Garment.DyeQuality >= 0.99f)
	{
		Notify(LOCTEXT("DipPerfect", "A deep, even colour. The resist held perfectly."));
	}
	else if (FinalDipDepth < Settings->DipGoodMin)
	{
		Notify(LOCTEXT("DipPale", "Lifted too soon: the colour is pale and the resist barely set."));
	}
	else
	{
		Notify(LOCTEXT("DipCracked", "Left in too long: the wax softened and fine cracks let the dye vein through."));
	}

	// Reveal: the colour comes up only as deep as the dip went.
	UMaterialInstanceDynamic* Material = GetClothMaterial();
	const FName DyedParameter = Settings->DyedParameterName;
	const float TargetDyed = FMath::Max(FinalDipDepth, 0.2f);
	TWeakObjectPtr<UMaterialInstanceDynamic> WeakMaterial(Material);
	PlayAnimation(DyeSeconds,
		[WeakMaterial, DyedParameter, TargetDyed](float Alpha)
		{
			if (WeakMaterial.IsValid())
			{
				WeakMaterial->SetScalarParameterValue(DyedParameter, Alpha * TargetDyed);
			}
		},
		[this, bIndigo]()
		{
			SpendTime(UCraftSettings::Get()->CostDye,
				bIndigo ? LOCTEXT("TimeDyeIndigo", "Dyed in indigo, wax boiled off") : LOCTEXT("TimeDyeSoga", "Dyed in soga, wax boiled off"));
			AdvanceStage();
		});
}

void UCraftLoopSubsystem::StartStitching()
{
	if (RequireStage(ECraftStage::Stitch, TEXT("StartStitching")) && !bStitching)
	{
		bStitching = true;
		StitchStartTime = Now();
		StitchScores.Reset();
		Broadcast();
	}
}

int32 UCraftLoopSubsystem::GetStitchesTotal() const
{
	FCraftGarmentBlueprint Blueprint;
	const int32 Seams = GetChosenBlueprint(Blueprint) ? Blueprint.SeamCount : 1;
	return FMath::Max(1, Seams * UCraftSettings::Get()->StitchesPerSeam);
}

float UCraftLoopSubsystem::GetStitchMarker() const
{
	if (!bStitching)
	{
		return 0.0f;
	}
	// Triangle wave: the needle sweeps 0 -> 1 -> 0.
	const double Cycles = (Now() - StitchStartTime) * UCraftSettings::Get()->StitchMarkerSpeed;
	const float Phase = static_cast<float>(Cycles - FMath::FloorToDouble(Cycles));
	return 1.0f - FMath::Abs(2.0f * Phase - 1.0f);
}

void UCraftLoopSubsystem::PressStitch()
{
	if (Stage == ECraftStage::Stitch && bStitching && !bBusy)
	{
		StitchAt(GetStitchMarker());
	}
}

void UCraftLoopSubsystem::StitchAt(float MarkerPosition)
{
	StitchScores.Add(UCraftRules::StitchAccuracy(MarkerPosition, UCraftSettings::Get()->StitchTolerance));
	if (StitchScores.Num() < GetStitchesTotal())
	{
		Broadcast();
		return;
	}

	bStitching = false;
	float Sum = 0.0f;
	for (const float Score : StitchScores)
	{
		Sum += Score;
	}
	Garment.SeamQuality = Sum / StitchScores.Num();
	UE_LOG(LogCraftLoop, Log, TEXT("Seam quality %.2f over %d stitches"), Garment.SeamQuality, StitchScores.Num());

	const UCraftSettings* Settings = UCraftSettings::Get();
	FCraftCommission Commission;
	const FCraftGarmentBlueprint* Blueprint = Settings->FindBlueprint(Garment.BlueprintId);
	const FCraftFabric* Fabric = Settings->FindFabric(Garment.FabricId);
	if (GetCurrentCommission(Commission) && Blueprint && Fabric)
	{
		Garment = UCraftRules::BuildGarment(Garment, Commission, *Blueprint, *Fabric);
	}

	TWeakObjectPtr<AActor> WeakCloth = Cloth;
	const FTransform Start = ClothStartTransform;
	PlayAnimation(StitchSeconds,
		[WeakCloth, Start](float Alpha)
		{
			if (AActor* Actor = WeakCloth.Get())
			{
				const float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
				FTransform Folded = Start;
				Folded.SetScale3D(Start.GetScale3D() * FMath::Lerp(1.0f, StitchedScale, Eased));
				Folded.AddToTranslation(FVector(0.0, 0.0, StitchedLiftCm * Eased));
				Actor->SetActorTransform(Folded);
			}
		},
		[this]()
		{
			SpendTime(UCraftSettings::Get()->CostStitch, LOCTEXT("TimeStitch", "Stitched the panels together"));
			AdvanceStage();
		});
}

void UCraftLoopSubsystem::ConfirmInspection()
{
	if (RequireStage(ECraftStage::Inspect, TEXT("ConfirmInspection")))
	{
		AdvanceStage();
	}
}

void UCraftLoopSubsystem::DeliverGarment()
{
	FCraftCommission Commission;
	if (!RequireStage(ECraftStage::Deliver, TEXT("DeliverGarment")) || !GetCurrentCommission(Commission))
	{
		return;
	}
	const FCraftGarmentBlueprint* Blueprint = UCraftSettings::Get()->FindBlueprint(Garment.BlueprintId);
	Expedition = UCraftRules::ResolveExpedition(Commission, Garment, Blueprint ? *Blueprint : FCraftGarmentBlueprint());
	RevealedEvents = 0;
	UE_LOG(LogCraftLoop, Log, TEXT("Expedition resolved: %s, final HP %.0f"), *UEnum::GetValueAsString(Expedition.Outcome), Expedition.FinalHP);

	AdvanceStage();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ExpeditionTimer, FTimerDelegate::CreateUObject(this, &UCraftLoopSubsystem::RevealNextEvent),
			UCraftSettings::Get()->ExpeditionLineInterval, true, 0.3f);
	}
}

void UCraftLoopSubsystem::RevealNextEvent()
{
	if (RevealedEvents < Expedition.Events.Num())
	{
		++RevealedEvents;
		Broadcast();
	}
	if (RevealedEvents >= Expedition.Events.Num())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpeditionTimer);
		}
	}
}

void UCraftLoopSubsystem::SkipExpedition()
{
	if (!RequireStage(ECraftStage::Expedition, TEXT("SkipExpedition")))
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExpeditionTimer);
	}
	RevealedEvents = Expedition.Events.Num();
	Broadcast();
}

void UCraftLoopSubsystem::FinishExpedition()
{
	if (!RequireStage(ECraftStage::Expedition, TEXT("FinishExpedition")))
	{
		return;
	}
	SkipExpedition();

	FCraftCommission Commission;
	if (!bRewardsCollected && GetCurrentCommission(Commission))
	{
		bRewardsCollected = true;
		switch (Expedition.Outcome)
		{
		case ECraftExpeditionOutcome::Success: Gold += Commission.RewardGold; break;
		case ECraftExpeditionOutcome::Injured: Gold += Commission.RewardGold / 2; break;
		default: Gold += Commission.RewardGold / 3; break;
		}
		if (Expedition.Outcome != ECraftExpeditionOutcome::Retreat && !Commission.RewardMotif.IsNone() && !KnownMotifs.Contains(Commission.RewardMotif))
		{
			KnownMotifs.Add(Commission.RewardMotif);
			Notify(FText::Format(LOCTEXT("MotifLearned", "You learned the {0} motif. Your future inscriptions hold together better."),
				FText::FromName(Commission.RewardMotif)));
		}
	}
	AdvanceStage();
}

void UCraftLoopSubsystem::NextCommission()
{
	if (!RequireStage(ECraftStage::Return, TEXT("NextCommission")))
	{
		return;
	}
	++CommissionIndex;
	if (CommissionIndex < UCraftSettings::Get()->Commissions.Num())
	{
		BeginCommission();
	}
	else
	{
		SetStage(ECraftStage::Finished);
	}
}

void UCraftLoopSubsystem::RestartLoop()
{
	if (bBusy)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExpeditionTimer);
	}
	Hour = 0;
	Gold = 0;
	CommissionIndex = 0;
	KnownMotifs.Reset();
	bIndigoDelivered = false;
	OnTimeChanged.Broadcast(Hour, LOCTEXT("TimeRestart", "A new day in the workshop"));
	BeginCommission();
}

// ---------------------------------------------------------------------------------------------------------------------
// Debug

void UCraftLoopSubsystem::AutoStep()
{
	if (bBusy)
	{
		return;
	}
	FCraftCommission Commission;
	GetCurrentCommission(Commission);
	switch (Stage)
	{
	case ECraftStage::Commission: AcceptCommission(); break;
	case ECraftStage::SelectBlueprint:
		ChooseBlueprint(Commission.AllowedBlueprints.Num() > 0 ? Commission.AllowedBlueprints[0] : UCraftSettings::Get()->Blueprints[0].Id);
		break;
	case ECraftStage::SelectCloth: ChooseFabric(UCraftSettings::Get()->Fabrics[0].Id); break;
	case ECraftStage::Arrange:
	{
		FCraftGarmentBlueprint Blueprint;
		FCraftFabric Fabric;
		if (GetChosenBlueprint(Blueprint) && GetChosenFabric(Fabric) && UCraftRules::AutoArrange(Fabric, Blueprint, Placements))
		{
			CommitArrangement();
		}
		else
		{
			UE_LOG(LogCraftLoop, Error, TEXT("AutoStep: the pattern pieces don't fit on this fabric."));
		}
		break;
	}
	case ECraftStage::Cut: CommitCut(); break;
	case ECraftStage::Canting: FinishCanting(); break;
	case ECraftStage::Dye:
		if (!bDyeChosen)
		{
			ChooseDye(bIndigoDelivered);
		}
		else
		{
			const UCraftSettings* Settings = UCraftSettings::Get();
			FinishDip(0.5f * (Settings->DipGoodMin + Settings->DipGoodMax));
		}
		break;
	case ECraftStage::Stitch:
		if (!bStitching)
		{
			StartStitching();
		}
		else
		{
			StitchAt(0.5f);
		}
		break;
	case ECraftStage::Inspect: ConfirmInspection(); break;
	case ECraftStage::Deliver: DeliverGarment(); break;
	case ECraftStage::Expedition: FinishExpedition(); break;
	case ECraftStage::Return: NextCommission(); break;
	default:
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoPlayTimer);
		}
		UE_LOG(LogCraftLoop, Log, TEXT("AutoPlay finished: %d gold, %d motifs, hour %d."), Gold, KnownMotifs.Num(), Hour);
		break;
	}
}

void UCraftLoopSubsystem::StartAutoPlay()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AutoPlayTimer, FTimerDelegate::CreateUObject(this, &UCraftLoopSubsystem::AutoStep), 0.4f, true);
	}
}

static FAutoConsoleCommandWithWorld GCraftAutoPlayCommand(
	TEXT("Craft.AutoPlay"),
	TEXT("Plays through every commission with default choices and no drawing."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UCraftLoopSubsystem* Loop = World ? World->GetSubsystem<UCraftLoopSubsystem>() : nullptr)
		{
			Loop->StartAutoPlay();
		}
	}));

static FAutoConsoleCommandWithWorld GCraftStepCommand(
	TEXT("Craft.Step"),
	TEXT("Takes the default action for the current craft stage."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UCraftLoopSubsystem* Loop = World ? World->GetSubsystem<UCraftLoopSubsystem>() : nullptr)
		{
			Loop->AutoStep();
		}
	}));

// ---------------------------------------------------------------------------------------------------------------------
// Queries

bool UCraftLoopSubsystem::GetCurrentCommission(FCraftCommission& OutCommission) const
{
	const TArray<FCraftCommission>& Commissions = UCraftSettings::Get()->Commissions;
	if (!Commissions.IsValidIndex(CommissionIndex))
	{
		return false;
	}
	OutCommission = Commissions[CommissionIndex];
	return true;
}

bool UCraftLoopSubsystem::HasLiveCanvas() const
{
	return GetCanvas() != nullptr;
}

int32 UCraftLoopSubsystem::GetStagePhase() const
{
	switch (Stage)
	{
	case ECraftStage::Dye: return bBusy ? 3 : (bDipping ? 2 : (bDyeChosen ? 1 : 0));
	case ECraftStage::Stitch: return bBusy ? 2 : (bStitching ? 1 : 0);
	default: return bBusy ? 1 : 0;
	}
}

bool UCraftLoopSubsystem::GetChosenBlueprint(FCraftGarmentBlueprint& OutBlueprint) const
{
	const FCraftGarmentBlueprint* Found = UCraftSettings::Get()->FindBlueprint(Garment.BlueprintId);
	if (Found)
	{
		OutBlueprint = *Found;
	}
	return Found != nullptr;
}

bool UCraftLoopSubsystem::GetChosenFabric(FCraftFabric& OutFabric) const
{
	const FCraftFabric* Found = UCraftSettings::Get()->FindFabric(Garment.FabricId);
	if (Found)
	{
		OutFabric = *Found;
	}
	return Found != nullptr;
}

double UCraftLoopSubsystem::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : FPlatformTime::Seconds();
}

bool UCraftLoopSubsystem::GetLiveInscription(FCraftInscription& OutInscription) const
{
	const UCantingCanvasComponent* Canvas = GetCanvas();
	if (!Canvas)
	{
		return false;
	}
	const FCantingInscriptionStats Stats = Canvas->GetStats();
	const FCraftFabric* Fabric = UCraftSettings::Get()->FindFabric(Garment.FabricId);
	OutInscription = UCraftRules::InterpretInscription(Stats.StrokeCount, Stats.WaxCoverage, Stats.EnclosedShapes,
		Fabric ? *Fabric : FCraftFabric(), false, KnownMotifs.Num());
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// Cloth

AActor* UCraftLoopSubsystem::FindCloth()
{
	if (Cloth.IsValid())
	{
		return Cloth.Get();
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	// Prefer the actor that carries the C++ canvas.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->FindComponentByClass<UCantingCanvasComponent>())
		{
			Cloth = *It;
			return *It;
		}
	}
	if (UClass* ClothClass = UCraftSettings::Get()->ClothActorClass.LoadSynchronous())
	{
		for (TActorIterator<AActor> It(World, ClothClass); It; ++It)
		{
			Cloth = *It;
			return *It;
		}
	}
	return nullptr;
}

UCantingCanvasComponent* UCraftLoopSubsystem::GetCanvas() const
{
	const AActor* Actor = Cloth.Get();
	return Actor ? Actor->FindComponentByClass<UCantingCanvasComponent>() : nullptr;
}

void UCraftLoopSubsystem::SetClothDrawable(bool bDrawable)
{
	AActor* Actor = Cloth.Get();
	if (!Actor)
	{
		return;
	}
	// The workshop controller line-traces the cloth on the Visibility channel to paint.
	// Ignoring that channel outside the Canting stage blocks drawing without touching any Blueprint.
	TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!OriginalVisibilityResponse.Contains(Primitive))
		{
			OriginalVisibilityResponse.Add(Primitive, Primitive->GetCollisionResponseToChannel(ECC_Visibility));
		}
		const ECollisionResponse Original = OriginalVisibilityResponse[Primitive];
		Primitive->SetCollisionResponseToChannel(ECC_Visibility, bDrawable ? Original : ECR_Ignore);
	}
}

void UCraftLoopSubsystem::ResetCloth()
{
	const UCraftSettings* Settings = UCraftSettings::Get();
	if (AActor* Actor = Cloth.Get())
	{
		Actor->SetActorTransform(ClothStartTransform);
	}
	if (UCantingCanvasComponent* Canvas = GetCanvas())
	{
		Canvas->ClearInscription();
	}
	else if (UTextureRenderTarget2D* Target = Settings->LegacyWaxRenderTarget.LoadSynchronous())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, Target, Settings->LegacyClearColor);
	}
	if (ClothMaterial)
	{
		ClothMaterial->SetScalarParameterValue(Settings->DyedParameterName, 0.0f);
	}
}

bool UCraftLoopSubsystem::MeasureCloth(int32& OutStrokes, float& OutCoverage, int32& OutEnclosed)
{
	OutStrokes = 0;
	OutCoverage = 0.0f;
	OutEnclosed = 0;

	if (const UCantingCanvasComponent* Canvas = GetCanvas())
	{
		const FCantingInscriptionStats Stats = Canvas->GetStats();
		OutStrokes = Stats.StrokeCount;
		OutCoverage = Stats.WaxCoverage;
		OutEnclosed = Stats.EnclosedShapes;
		return true;
	}

	// Legacy path: read back the render target the Blueprint brush paints into.
	UTextureRenderTarget2D* Target = UCraftSettings::Get()->LegacyWaxRenderTarget.LoadSynchronous();
	FTextureRenderTargetResource* Resource = Target ? Target->GameThread_GetRenderTargetResource() : nullptr;
	if (!Resource)
	{
		UE_LOG(LogCraftLoop, Warning, TEXT("No canvas component and no wax render target; the inscription is empty."));
		return false;
	}
	TArray<FColor> Pixels;
	if (!Resource->ReadPixels(Pixels))
	{
		UE_LOG(LogCraftLoop, Warning, TEXT("Could not read the wax render target."));
		return false;
	}
	int32 MaskWidth = 0;
	int32 MaskHeight = 0;
	const TArray<uint8> Mask = UCraftRules::MaskFromPixels(Pixels, Target->SizeX, Target->SizeY, LegacyMaskSize, MaskWidth, MaskHeight);
	const FCraftMaskStats Stats = UCraftRules::AnalyseWaxMask(Mask, MaskWidth, MaskHeight);
	OutCoverage = Stats.WaxCoverage;
	OutEnclosed = Stats.EnclosedShapes;
	return true;
}

UMaterialInstanceDynamic* UCraftLoopSubsystem::GetClothMaterial()
{
	if (ClothMaterial)
	{
		return ClothMaterial;
	}
	AActor* Actor = Cloth.Get();
	UStaticMeshComponent* Mesh = Actor ? Actor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!Mesh)
	{
		return nullptr;
	}
	// Returns the existing dynamic instance if the canvas component already made one.
	const int32 Slot = GetCanvas() ? GetCanvas()->MaterialIndex : 0;
	ClothMaterial = Mesh->CreateDynamicMaterialInstance(Slot);
	return ClothMaterial;
}

// ---------------------------------------------------------------------------------------------------------------------
// Animation

void UCraftLoopSubsystem::PlayAnimation(float Duration, TFunction<void(float)> OnStep, TFunction<void()> OnDone)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnStep(1.0f);
		OnDone();
		return;
	}
	bBusy = true;
	SetClothDrawable(false);
	AnimationStart = World->GetTimeSeconds();
	AnimationDuration = FMath::Max(Duration, 0.01f);
	AnimationStep = MoveTemp(OnStep);
	AnimationDone = MoveTemp(OnDone);
	World->GetTimerManager().SetTimer(AnimationTimer, FTimerDelegate::CreateUObject(this, &UCraftLoopSubsystem::TickAnimation), 1.0f / 60.0f, true);
	Broadcast();
}

void UCraftLoopSubsystem::TickAnimation()
{
	UWorld* World = GetWorld();
	const float Alpha = World ? FMath::Clamp(static_cast<float>((World->GetTimeSeconds() - AnimationStart) / AnimationDuration), 0.0f, 1.0f) : 1.0f;
	if (AnimationStep)
	{
		AnimationStep(Alpha);
	}
	if (Alpha < 1.0f)
	{
		return;
	}
	if (World)
	{
		World->GetTimerManager().ClearTimer(AnimationTimer);
	}
	bBusy = false;
	TFunction<void()> Done = MoveTemp(AnimationDone);
	AnimationStep.Reset();
	AnimationDone.Reset();
	if (Done)
	{
		Done();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// HUD

void UCraftLoopSubsystem::ShowHUD()
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		// The viewport can appear a frame late (e.g. standalone); try again next tick, but not forever (servers have none).
		if (World && ++HUDRetries < 120)
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCraftLoopSubsystem::ShowHUD));
		}
		return;
	}
	if (!HUD.IsValid())
	{
		SAssignNew(HUD, SCraftLoopHUD).Loop(this);
		Viewport->AddViewportWidgetContent(HUD.ToSharedRef(), 10);
	}
	HUD->Refresh();
}

void UCraftLoopSubsystem::HideHUD()
{
	if (!HUD.IsValid())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->RemoveViewportWidgetContent(HUD.ToSharedRef());
		}
	}
	HUD.Reset();
}

#undef LOCTEXT_NAMESPACE
