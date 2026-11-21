// Core data types for the Canting Cloth craft loop:
// Commission -> Blueprint -> Cloth -> Arrange -> Cut -> Canting -> Dye -> Stitch -> Inspect -> Deliver -> Expedition -> Return.

#pragma once

#include "CoreMinimal.h"
#include "CraftTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCraftLoop, Log, All);

/** Stages of one commission, in play order. AdvanceStage moves to the next value. */
UENUM(BlueprintType)
enum class ECraftStage : uint8
{
	Commission,
	SelectBlueprint,
	SelectCloth,
	Arrange,
	Cut,
	Canting,
	Dye,
	Stitch,
	Inspect,
	Deliver,
	Expedition,
	Return,
	Finished
};

UENUM(BlueprintType)
enum class ECraftEffectFamily : uint8
{
	/** Protects against environmental hazards (miasma, poison). */
	WardPoison,
	/** Protects against physical attacks. */
	WardPhysical
};

UENUM(BlueprintType)
enum class ECraftExpeditionOutcome : uint8
{
	Success,
	Injured,
	Retreat
};

UENUM(BlueprintType)
enum class ECraftEventType : uint8
{
	Travel,
	Encounter,
	Hazard,
	InscriptionActivates,
	Result
};

USTRUCT(BlueprintType)
struct FCraftFabric
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	float Durability = 40.0f;

	/** Wax coverage (0-1 of the panel) the cloth can hold before the inscription becomes unstable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MagicCapacity = 0.12f;

	/** Size of the bolt in layout cells (Arrange stage). The grain runs along the height. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "2"))
	int32 SheetWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "2"))
	int32 SheetHeight = 14;
};

/** One pattern piece of a blueprint, in layout cells, unrotated (its height runs along the grain). */
USTRUCT(BlueprintType)
struct FCraftPatternPiece
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "1"))
	int32 Width = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "1"))
	int32 Height = 6;
};

/** Where a pattern piece sits on the fabric sheet. Rotated pieces run across the grain. */
USTRUCT(BlueprintType)
struct FCraftPiecePlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	bool bPlaced = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	bool bRotated = false;
};

USTRUCT(BlueprintType)
struct FCraftGarmentBlueprint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	float BaseWeight = 1.0f;

	/** Above this finished weight the wearer is slowed on the expedition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	float MaxWeight = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseCoverage = 0.7f;

	/** Pieces laid out on the fabric in the Arrange stage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	TArray<FCraftPatternPiece> Pieces;

	/** Seams joined in the Stitch stage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (ClampMin = "1"))
	int32 SeamCount = 3;
};

USTRUCT(BlueprintType)
struct FCraftCommission
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FText CustomerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (MultiLine = true))
	FText RequestText;

	/** Blueprint ids the customer will accept. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	TArray<FName> AllowedBlueprints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	ECraftEffectFamily RequiredEffect = ECraftEffectFamily::WardPoison;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FText HazardName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	float HazardStrength = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FText EnemyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	float EnemyAttack = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 RewardGold = 30;

	/** Knowledge brought home on Success or Injured (the Gotong Royong / Pulang payoff). Empty = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	FName RewardMotif;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (MultiLine = true))
	FText ReturnSuccessText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (MultiLine = true))
	FText ReturnInjuredText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft", meta = (MultiLine = true))
	FText ReturnRetreatText;
};

/** Measured geometry of the wax drawing plus its magical interpretation. */
USTRUCT(BlueprintType)
struct FCraftInscription
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 StrokeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float WaxCoverage = 0.0f;

	/** Areas fully walled in by wax: closed klowong outlines whose resist holds in the dye bath. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 EnclosedShapes = 0;

	/** WaxCoverage / fabric MagicCapacity. Above 1 the wax overloads the cloth. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float CapacityFill = 0.0f;

	/** 0-1: how much magic the drawing carries. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Strength = 0.0f;

	/** 0-1: how well the ward holds together. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Stability = 0.0f;

	/** True when the drawing itself decided the ward (curved walls = filter, straight walls = deflect).
	 *  False with the legacy render target, or when the canvas has bReadWards off: then the commission decides. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	bool bWardsRead = false;

	/** The ward the drawing makes: whichever family has more power. Only meaningful when bWardsRead. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	ECraftEffectFamily DrawnEffect = ECraftEffectFamily::WardPoison;

	/** 0-1 strength of each ward family on its own (1 = enough sealed cells for a full ward). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float FilterStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float DeflectStrength = 0.0f;

	/** Sealed cells that read mostly curved (filter) or mostly straight (deflect). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 FilterCells = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	int32 DeflectCells = 0;
};

/** Everything the player decided while making the garment, and the stats that come out of it. */
USTRUCT(BlueprintType)
struct FCraftGarment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	FName BlueprintId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	FName FabricId;

	/** Share of the pattern area laid across the grain: drapes and covers better, but weaker. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float CrossGrainFraction = 0.0f;

	/** Share of the fabric sheet left unused by the layout. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float LayoutWaste = 0.0f;

	/** Hem length relative to the blueprint (0.8 short ... 1.2 long). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float CutLength = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	bool bIndigoDye = false;

	/** 0-1: how well the dip was timed. Pale dye barely sets the resist; too long and the wax cracks. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float DyeQuality = 0.5f;

	/** 0-1: average accuracy of the stitches. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float SeamQuality = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	FCraftInscription Inscription;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Weight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Coverage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Durability = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	ECraftEffectFamily Effect = ECraftEffectFamily::WardPoison;

	/** 0-100: how much damage the ward absorbs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Protection = 0.0f;
};

USTRUCT(BlueprintType)
struct FCraftExpeditionEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	ECraftEventType Type = ECraftEventType::Travel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	FText Text;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float Damage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float HPAfter = 100.0f;
};

USTRUCT(BlueprintType)
struct FCraftExpeditionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	TArray<FCraftExpeditionEvent> Events;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	ECraftExpeditionOutcome Outcome = ECraftExpeditionOutcome::Success;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Craft")
	float FinalHP = 100.0f;
};
