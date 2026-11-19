// Runs the whole commission loop for the workshop level. Created automatically in every game world.
//
// Blueprints reach it with "Get CraftLoopSubsystem" and can call any action below or bind the events.
// Each action checks the current stage, so a stray click can never skip a step.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CraftTypes.h"
#include "CraftLoopSubsystem.generated.h"

class UCantingCanvasComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class SCraftLoopHUD;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftStageChanged, ECraftStage, NewStage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftTimeChanged, int32, Hour, const FText&, ActionLabel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftNotice, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftLoopUpdated);

UCLASS()
class CANTINGCLOTH_API UCraftLoopSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "Craft")
	FOnCraftStageChanged OnStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Craft")
	FOnCraftTimeChanged OnTimeChanged;

	/** Scheduled events and short messages for the player. */
	UPROPERTY(BlueprintAssignable, Category = "Craft")
	FOnCraftNotice OnNotice;

	/** Fires after any state change. Widgets can simply refresh on this. */
	UPROPERTY(BlueprintAssignable, Category = "Craft")
	FOnCraftLoopUpdated OnLoopUpdated;

	// ---- Actions (each one only works in its own stage) ----

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void AcceptCommission();

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	bool ChooseBlueprint(FName BlueprintId);

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	bool ChooseFabric(FName FabricId);

	/** Arrange: puts a pattern piece on the sheet. Fails (and changes nothing) if it doesn't fit or overlaps. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	bool PlacePiece(int32 PieceIndex, int32 X, int32 Y, bool bRotated);

	/** Arrange: takes a piece back off the sheet. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void RemovePiece(int32 PieceIndex);

	/** Arrange: confirms the layout once every piece is on the sheet. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	bool CommitArrangement();

	/** Cut: moves the hem (0.8 short ... 1.2 long). Free until committed. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void SetCutLength(float CutLength);

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void CommitCut();

	/** Measures the wax on the cloth, interprets it, and moves on to dyeing. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void FinishCanting();

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void ClearCanting();

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	bool UndoCantingStroke();

	/** Dye: picks the vat. Indigo only works once the neighbour has delivered it. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	bool ChooseDye(bool bIndigo);

	/** Dye: lowers the cloth into the vat (hold). */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void StartDip();

	/** Dye: lifts the cloth out (release). The depth reached decides the dye quality, then the wax is boiled off. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void StopDip();

	/** Stitch: threads the needle and starts the first seam. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void StartStitching();

	/** Stitch: one stitch at the needle's current position. Best when the marker is in the middle. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void PressStitch();

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void ConfirmInspection();

	/** Hands the garment over; the expedition is resolved immediately, then replayed line by line. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void DeliverGarment();

	/** Shows every remaining chronicle line at once. The outcome is already decided. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void SkipExpedition();

	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void FinishExpedition();

	/** Collects the rewards and calls in the next customer (or ends the slice). */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void NextCommission();

	/** Starts the slice over from the first customer. */
	UFUNCTION(BlueprintCallable, Category = "Craft|Actions")
	void RestartLoop();

	/** Debug: takes the default action for the current stage (first allowed choice, no drawing). */
	UFUNCTION(BlueprintCallable, Category = "Craft|Debug")
	void AutoStep();

	/** Debug: plays through every commission with AutoStep. Console: Craft.AutoPlay */
	UFUNCTION(BlueprintCallable, Category = "Craft|Debug")
	void StartAutoPlay();

	// ---- State ----

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	ECraftStage GetStage() const { return Stage; }

	/** True only while the player may draw wax on the cloth. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool IsCantingActive() const { return Stage == ECraftStage::Canting && !bBusy; }

	/** True while a dye or stitch animation is playing. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool IsBusy() const { return bBusy; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetHour() const { return Hour; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	TArray<FName> GetKnownMotifs() const { return KnownMotifs; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool IsIndigoAvailable() const { return bIndigoDelivered; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool GetCurrentCommission(FCraftCommission& OutCommission) const;

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetCommissionIndex() const { return CommissionIndex; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	FCraftGarment GetGarment() const { return Garment; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	FCraftExpeditionResult GetExpedition() const { return Expedition; }

	/** How many chronicle lines are visible so far. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetRevealedEventCount() const { return RevealedEvents; }

	/** Live reading of the drawing while canting. Only available when the cloth has a Canting Canvas component. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool GetLiveInscription(FCraftInscription& OutInscription) const;

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool HasLiveCanvas() const;

	/** Sub-step inside interactive stages: Dye 0 = choose vat, 1 = ready to dip, 2 = dipping, 3 = revealing;
	 *  Stitch 0 = ready, 1 = stitching, 2 = assembling. 0 elsewhere. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetStagePhase() const;

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	TArray<FCraftPiecePlacement> GetPlacements() const { return Placements; }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	bool CanCommitArrangement() const;

	/** Blueprint and fabric chosen for the current garment (false if not chosen yet). */
	bool GetChosenBlueprint(FCraftGarmentBlueprint& OutBlueprint) const;
	bool GetChosenFabric(FCraftFabric& OutFabric) const;

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	float GetCutLength() const { return Garment.CutLength; }

	/** 0-1 dye depth; rises while dipping. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	float GetDipDepth() const;

	/** 0-1 position of the needle marker on the stitch bar. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	float GetStitchMarker() const;

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetStitchesDone() const { return StitchScores.Num(); }

	UFUNCTION(BlueprintPure, Category = "Craft|State")
	int32 GetStitchesTotal() const;

	/** Accuracy of the most recent stitch, or -1 before the first. */
	UFUNCTION(BlueprintPure, Category = "Craft|State")
	float GetLastStitchAccuracy() const { return StitchScores.Num() > 0 ? StitchScores.Last() : -1.0f; }

protected:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void BeginCommission();
	void SetStage(ECraftStage NewStage);
	void AdvanceStage();
	void SpendTime(int32 Hours, const FText& Label);
	void CheckScheduledEvents();
	void Notify(const FText& Message);
	void Broadcast();
	bool RequireStage(ECraftStage Expected, const TCHAR* Action) const;

	// Cloth in the level
	AActor* FindCloth();
	UCantingCanvasComponent* GetCanvas() const;
	void SetClothDrawable(bool bDrawable);
	void ResetCloth();
	bool MeasureCloth(int32& OutStrokes, float& OutCoverage, int32& OutEnclosed);
	UMaterialInstanceDynamic* GetClothMaterial();

	// Animations (dye reveal, stitching)
	void PlayAnimation(float Duration, TFunction<void(float)> OnStep, TFunction<void()> OnDone);
	void TickAnimation();

	void RevealNextEvent();
	void FinishDip(float Depth);
	void StitchAt(float MarkerPosition);
	double Now() const;

	void ShowHUD();
	void HideHUD();

	ECraftStage Stage = ECraftStage::Commission;
	int32 Hour = 0;
	int32 CommissionIndex = 0;
	int32 Gold = 0;
	TArray<FName> KnownMotifs;
	bool bIndigoDelivered = false;
	bool bBusy = false;
	bool bRewardsCollected = false;

	FCraftGarment Garment;
	TArray<FCraftPiecePlacement> Placements;
	bool bDyeChosen = false;
	bool bDipping = false;
	double DipStartTime = 0.0;
	float FinalDipDepth = 0.0f;
	bool bStitching = false;
	double StitchStartTime = 0.0;
	TArray<float> StitchScores;
	FCraftExpeditionResult Expedition;
	int32 RevealedEvents = 0;

	TWeakObjectPtr<AActor> Cloth;
	FTransform ClothStartTransform;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionResponse>> OriginalVisibilityResponse;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ClothMaterial;

	FTimerHandle AnimationTimer;
	FTimerHandle ExpeditionTimer;
	FTimerHandle AutoPlayTimer;
	double AnimationStart = 0.0;
	float AnimationDuration = 0.0f;
	TFunction<void(float)> AnimationStep;
	TFunction<void()> AnimationDone;

	TSharedPtr<SCraftLoopHUD> HUD;
	int32 HUDRetries = 0;
};
