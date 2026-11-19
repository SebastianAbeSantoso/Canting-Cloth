// Canting canvas: add this component to BP_ClothActor.
//
// Blueprint calls DrawAtUV every tick while the mouse is held and EndStroke on release.
// The component stores every stroke as points in panel centimetres (the source of truth),
// keeps a wax distance grid in memory, uploads only the changed pixels to a texture,
// and counts enclosed (sealed) shapes when a stroke ends.
//
// Texture channels read by M_BatikCloth:
//   R = wax signed distance (0.5 = wax edge; decode with (R - 0.5) * 16)
//   G = 1 inside sealed (enclosed) regions, 0 elsewhere

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CantingWaxGrid.h"
#include "CantingCanvasComponent.generated.h"

class UMaterialInstanceDynamic;
class UTexture2D;

USTRUCT(BlueprintType)
struct FCantingStroke
{
	GENERATED_BODY()

	/** Stroke points in panel centimetres, (0,0) = UV (0,0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	TArray<FVector2D> PointsCm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	float RadiusCm = 1.2f;
};

USTRUCT(BlueprintType)
struct FCantingInscriptionStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	int32 StrokeCount = 0;

	/** Total length of all strokes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	float InkLengthCm = 0.0f;

	/** Areas fully walled in by wax that don't touch the panel edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	int32 EnclosedShapes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	float EnclosedAreaCm2 = 0.0f;

	/** Fraction of the panel covered by wax, 0 to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	float WaxCoverage = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCantingInscriptionChanged, const FCantingInscriptionStats&, Stats);

UCLASS(ClassGroup = (Canting), meta = (BlueprintSpawnableComponent))
class CANTINGCLOTH_API UCantingCanvasComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCantingCanvasComponent();

	/** Real-world size of the cloth panel. UV (1,1) maps to this corner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting|Setup")
	FVector2D PanelSizeCm = FVector2D(100.0, 100.0);

	/** Texture width in pixels. Height follows the panel's aspect ratio. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting|Setup", meta = (ClampMin = "64", ClampMax = "2048"))
	int32 GridResolution = 512;

	/** Texture parameter in the cloth material that receives the wax texture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting|Setup")
	FName TextureParameterName = TEXT("WaxField");

	/** Material slot on the owner's first Static Mesh component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting|Setup", meta = (ClampMin = "0"))
	int32 MaterialIndex = 0;

	/** Wax line half-width. 1.2 cm on a 100 cm panel matches the old 24 px brush on the 1024 px render target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Brush", meta = (ClampMin = "0.05"))
	float BrushRadiusCm = 1.2f;

	/** A new point is stored only after the cursor moves this far. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Brush", meta = (ClampMin = "0.01"))
	float MinPointSpacingCm = 0.3f;

	/** Gaps in an outline up to this width still count as sealed. 0 = any gap leaks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules", meta = (ClampMin = "0.0"))
	float SealGapToleranceCm = 0.0f;

	/** Enclosed areas smaller than this are ignored (e.g. the tiny hole inside a dot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules", meta = (ClampMin = "0.0"))
	float MinEnclosedAreaCm2 = 1.0f;

	/** Fires after a stroke ends, an undo, or a clear. */
	UPROPERTY(BlueprintAssignable, Category = "Canting")
	FOnCantingInscriptionChanged OnInscriptionChanged;

	/** Call every tick while drawing. Starts a stroke automatically on the first call. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	void DrawAtUV(FVector2D UV);

	/** Call when the mouse button is released. Updates the stats and fires OnInscriptionChanged. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	void EndStroke();

	/** Removes the most recent stroke. Returns false if there was nothing to undo. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	bool UndoLastStroke();

	/** Removes all strokes. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	void ClearInscription();

	UFUNCTION(BlueprintPure, Category = "Canting")
	FCantingInscriptionStats GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category = "Canting")
	TArray<FCantingStroke> GetStrokes() const { return Strokes; }

	UFUNCTION(BlueprintPure, Category = "Canting")
	bool IsStrokeActive() const { return bStrokeActive; }

	UFUNCTION(BlueprintPure, Category = "Canting")
	UTexture2D* GetWaxTexture() const { return WaxTexture; }

protected:
	virtual void BeginPlay() override;

private:
	void InitCanvas();
	void ConnectMaterial();
	CantingCore::FRectI StampStroke(const FCantingStroke& Stroke, const CantingCore::FRectI* Clip);
	CantingCore::FRectI StampLastPoints(const FCantingStroke& Stroke, int32 NumPoints);
	CantingCore::FRectI StrokeBounds(const FCantingStroke& Stroke) const;
	void RebuildRegion(const CantingCore::FRectI& Region);
	void RefreshAnalysis();
	void PackRect(const CantingCore::FRectI& Rect, uint8* Dest, int32 DestPitch) const;
	void UploadRect(const CantingCore::FRectI& Rect);

	UPROPERTY(VisibleInstanceOnly, Category = "Canting|Debug")
	TArray<FCantingStroke> Strokes;

	UPROPERTY(VisibleInstanceOnly, Category = "Canting|Debug")
	FCantingInscriptionStats Stats;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> WaxTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ClothMaterial;

	CantingCore::FWaxGrid Grid;
	std::vector<uint8_t> SealedMask;
	FVector2D LastSampleCm = FVector2D::ZeroVector;
	double TexelsPerCm = 1.0;
	bool bStrokeActive = false;
};
