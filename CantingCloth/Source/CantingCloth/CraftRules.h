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
		const FCraftFabric& Fabric, bool bIndigoDye, int32 KnownMotifCount);

	/** Fills Weight, Coverage, Durability, Effect and Protection from the player's choices. */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static FCraftGarment BuildGarment(const FCraftGarment& Choices, const FCraftCommission& Commission,
		const FCraftGarmentBlueprint& Blueprint, const FCraftFabric& Fabric);

	/** Resolves the whole expedition up front. The chronicle only replays these events, so skipping changes nothing. */
	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static FCraftExpeditionResult ResolveExpedition(const FCraftCommission& Commission, const FCraftGarment& Garment,
		const FCraftGarmentBlueprint& Blueprint);

	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static float CutCoverageMultiplier(ECraftCutStyle Cut);

	UFUNCTION(BlueprintPure, Category = "Craft|Rules")
	static float CutWeightMultiplier(ECraftCutStyle Cut);

	/**
	 * Measures a wax mask (non-zero = wax). Background regions that never touch the border and are at least
	 * MinEnclosedFraction of the mask count as enclosed shapes (4-connected, so diagonal lines seal).
	 */
	static FCraftMaskStats AnalyseWaxMask(const TArray<uint8>& Mask, int32 Width, int32 Height, float MinEnclosedFraction = 0.001f);

	/** Builds a wax mask from render-target pixels: the most common brightness is the bare cloth, anything clearly different is wax. */
	static TArray<uint8> MaskFromPixels(const TArray<FColor>& Pixels, int32 Width, int32 Height, int32 MaxSize, int32& OutWidth, int32& OutHeight);
};
