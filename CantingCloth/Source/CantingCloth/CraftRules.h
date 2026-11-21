// The rules of the craft loop as pure functions: no world, no actors, no UI.
// The subsystem feeds them state; Blueprints can call them too (for previews or tests).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftTypes.h"
#include "CraftRules.generated.h"

/** Result of measuring a wax mask (used when the cloth has no Canting Canvas component). */
struct FCraftMaskStats
{
	float WaxCoverage = 0.0f;
	int32 EnclosedShapes = 0;
};

UCLASS()
class CANTINGCLOTH_API UCraftRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Turns measured wax geometry into magic.
	 * Strength = how much of the fabric's capacity the wax fills (capped at 1).
	 * Stability = closed outlines (the resist holds in the dye bath) minus wax beyond capacity (wax bleeds).
	 */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static FCraftInscription InterpretInscription(int32 StrokeCount, float WaxCoverage, int32 EnclosedShapes,
		const FCraftFabric& Fabric, bool bIndigoDye, int32 KnownMotifCount, float DyeQuality = 0.5f);

	/**
	 * Lets the drawing decide the ward. Stores each family's fill (1 = full ward) and cell counts in the inscription,
	 * then, if bWardsRead and any sealed cell exists, sets DrawnEffect to the stronger family and Strength to its fill
	 * (capped at 1). Call after InterpretInscription, which sets Strength from wax alone.
	 * With bWardsRead false the inscription is left as InterpretInscription made it, and BuildGarment uses the
	 * commission's effect (the sprint plan's fallback).
	 */
	static void ApplyWardReading(FCraftInscription& Inscription, bool bWardsRead, float FilterFill, float DeflectFill,
		int32 FilterCells, int32 DeflectCells);

	/** Fills Weight, Coverage, Durability, Effect and Protection from the player's choices.
	 *  The effect comes from the drawing when Inscription.bWardsRead, otherwise from the commission. */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static FCraftGarment BuildGarment(const FCraftGarment& Choices, const FCraftCommission& Commission,
		const FCraftGarmentBlueprint& Blueprint, const FCraftFabric& Fabric);

	/** Resolves the whole expedition up front. The chronicle only replays these events, so skipping changes nothing. */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static FCraftExpeditionResult ResolveExpedition(const FCraftCommission& Commission, const FCraftGarment& Garment,
		const FCraftGarmentBlueprint& Blueprint);

	static constexpr float MinCutLength = 0.8f;
	static constexpr float MaxCutLength = 1.2f;

	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static float CutCoverageMultiplier(float CutLength);

	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static float CutWeightMultiplier(float CutLength);

	/** Size of a piece on the sheet, after rotation. */
	static FIntPoint PieceFootprint(const FCraftPatternPiece& Piece, bool bRotated);

	/** True if the piece fits on the sheet at Candidate without overlapping any other placed piece. */
	static bool IsPlacementValid(const FCraftFabric& Fabric, const FCraftGarmentBlueprint& Blueprint,
		const TArray<FCraftPiecePlacement>& Placements, int32 PieceIndex, const FCraftPiecePlacement& Candidate);

	/** Share of pattern area laid across the grain, and share of the sheet left unused. */
	static void EvaluateLayout(const FCraftFabric& Fabric, const FCraftGarmentBlueprint& Blueprint,
		const TArray<FCraftPiecePlacement>& Placements, float& OutCrossGrainFraction, float& OutWaste);

	/** First-fit layout (grain first, then rotated). Returns false if some piece doesn't fit. */
	static bool AutoArrange(const FCraftFabric& Fabric, const FCraftGarmentBlueprint& Blueprint, TArray<FCraftPiecePlacement>& OutPlacements);

	/** 1 inside the ideal window, falling off for a pale or over-long dip. */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static float DyeQualityFromDepth(float Depth, float GoodMin, float GoodMax);

	/** 1 at the centre of the stitch bar, 0 at Tolerance from it. */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static float StitchAccuracy(float MarkerPosition, float Tolerance);

	/**
	 * Measures a wax mask (non-zero = wax). Background regions that never touch the border and are at least
	 * MinEnclosedFraction of the mask count as enclosed shapes (4-connected, so diagonal lines seal).
	 */
	static FCraftMaskStats AnalyseWaxMask(const TArray<uint8>& Mask, int32 Width, int32 Height, float MinEnclosedFraction = 0.001f);

	/** Builds a wax mask from render-target pixels: the most common brightness is the bare cloth, anything clearly different is wax. */
	static TArray<uint8> MaskFromPixels(const TArray<FColor>& Pixels, int32 Width, int32 Height, int32 MaxSize, int32& OutWidth, int32& OutHeight);
};
