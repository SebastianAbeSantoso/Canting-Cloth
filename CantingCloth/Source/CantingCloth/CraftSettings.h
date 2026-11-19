// Project Settings -> Game -> Canting Cloth Craft Loop.
// Holds the vertical-slice content (customers, blueprints, fabrics) and every tuning number,
// so balancing never needs a code change. Defaults live in the constructor; edits are saved to DefaultGame.ini.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CraftTypes.h"
#include "CraftSettings.generated.h"

class UTextureRenderTarget2D;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Canting Cloth Craft Loop"))
class CANTINGCLOTH_API UCraftSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCraftSettings();

	static const UCraftSettings* Get() { return GetDefault<UCraftSettings>(); }

	const FCraftCommission* FindCommission(FName Id) const;
	const FCraftGarmentBlueprint* FindBlueprint(FName Id) const;
	const FCraftFabric* FindFabric(FName Id) const;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ---- Content ----

	/** Customers in the order they arrive. */
	UPROPERTY(Config, EditAnywhere, Category = "Content")
	TArray<FCraftCommission> Commissions;

	UPROPERTY(Config, EditAnywhere, Category = "Content")
	TArray<FCraftGarmentBlueprint> Blueprints;

	UPROPERTY(Config, EditAnywhere, Category = "Content")
	TArray<FCraftFabric> Fabrics;

	// ---- Global time economy (hours). Looking is free; committing costs time. ----

	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	int32 CostArrange = 1;

	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	int32 CostCut = 1;

	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	int32 CostCanting = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	int32 CostDye = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	int32 CostStitch = 2;

	/** Scheduled event: a neighbouring artisan brings indigo once the clock reaches this hour. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	int32 IndigoDeliveryHour = 4;

	// ---- Inscription interpretation ----

	/** Stability with no closed shapes at all. */
	UPROPERTY(Config, EditAnywhere, Category = "Inscription", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseStability = 0.3f;

	/** Stability gained per closed shape, up to MaxCountedShapes. */
	UPROPERTY(Config, EditAnywhere, Category = "Inscription", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StabilityPerShape = 0.175f;

	UPROPERTY(Config, EditAnywhere, Category = "Inscription", meta = (ClampMin = "1"))
	int32 MaxCountedShapes = 4;

	/** Stability lost per 100% of wax beyond the fabric's capacity. */
	UPROPERTY(Config, EditAnywhere, Category = "Inscription", meta = (ClampMin = "0.0"))
	float OverfillPenalty = 1.0f;

	/** Stability bonus from the community's indigo dye. */
	UPROPERTY(Config, EditAnywhere, Category = "Inscription", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndigoStabilityBonus = 0.1f;

	/** Base stability bonus for every motif learned from returning adventurers. */
	UPROPERTY(Config, EditAnywhere, Category = "Inscription", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MotifStabilityBonus = 0.1f;

	// ---- Workshop hookup (used only if the cloth has no Canting Canvas component) ----

	/** The cloth actor in the level. An actor with a Canting Canvas component is preferred if one exists. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	TSoftClassPtr<AActor> ClothActorClass;

	/** Render target the legacy Blueprint brush paints into; read back to measure the drawing. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	TSoftObjectPtr<UTextureRenderTarget2D> LegacyWaxRenderTarget;

	/** Colour the legacy render target is cleared to between commissions. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	FLinearColor LegacyClearColor = FLinearColor::Black;

	/** Scalar parameter on the cloth material animated 0 -> 1 when dyeing. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	FName DyedParameterName = TEXT("IsDyed");

	/** Optional vector parameter on the cloth material for the dye colour. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	FName DyeColorParameterName = TEXT("DyeColor");

	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	FLinearColor SogaColor = FLinearColor(0.32f, 0.17f, 0.07f);

	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	FLinearColor IndigoColor = FLinearColor(0.06f, 0.1f, 0.32f);

	/** Show the built-in Slate UI. Turn off once Blueprint widgets drive the loop through UCraftLoopSubsystem. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	bool bShowBuiltInUI = true;

	/** Seconds between expedition chronicle lines. */
	UPROPERTY(Config, EditAnywhere, Category = "Workshop", meta = (ClampMin = "0.1"))
	float ExpeditionLineInterval = 1.2f;
};
