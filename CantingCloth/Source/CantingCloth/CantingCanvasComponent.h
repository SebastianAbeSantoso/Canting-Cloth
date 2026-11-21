// Canting canvas: add this component to BP_ClothActor.
//
// Blueprint calls DrawAtUV every tick while the mouse is held on the cloth. The component ends the
// stroke by itself when DrawAtUV stops arriving (mouse released or moved off the cloth); calling
// EndStroke on release is optional and only makes it end one frame sooner.
//
// The component stores every stroke as points in panel centimetres (the source of truth),
// keeps a wax distance grid in memory, uploads only the changed pixels to a texture,
// and counts enclosed (sealed) shapes when a stroke ends.
//
// Texture channels read by M_BatikCloth (0 means "nothing" in every channel):
//   R = wax: 0 = no wax nearby, about 0.5 = wax edge, 1 = deep inside the wax.
//       Signed distance in texels: d = (0.5 - R) * 16 (negative inside the wax).
//   G = inside sealed cells, how much the cell counts as a filter ward (curved walls), 0 elsewhere.
//   B = inside sealed cells, how much the cell counts as a deflect ward (straight walls), 0 elsewhere.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CantingStructure.h"
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

	/** True if the canvas read which ward the drawing makes (bReadWards). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	bool bWardsRead = false;

	/** Ward power of each family relative to a full ward: 1 = FullWardCellAreaCm2 of sealed cells. Can exceed 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	float FilterFill = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	float DeflectFill = 0.0f;

	/** Sealed cells that read mostly curved (filter), mostly straight (deflect), or in between. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	int32 FilterCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	int32 DeflectCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canting")
	int32 MixedCells = 0;
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

	/** Gaps in an outline up to this width still count as sealed. 0 = any gap leaks. At most 1.5 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules", meta = (ClampMin = "0.0", ClampMax = "1.5"))
	float SealGapToleranceCm = 1.0f;

	/** Read which ward the drawing makes (curved walls = filter, straight = deflect).
	 *  Turn off to fall back to the commission choosing the ward (sprint plan cut checkpoint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules")
	bool bReadWards = true;

	/** Sealed-cell area for a full-strength ward. Each cell counts for at most MaxCellAreaCm2,
	 *  so a full ward needs about FullWardCellAreaCm2 / MaxCellAreaCm2 cells (12 with the defaults). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules", meta = (ClampMin = "1.0"))
	float FullWardCellAreaCm2 = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules", meta = (ClampMin = "1.0"))
	float MaxCellAreaCm2 = 60.0f;

	/** Print the ward reading on screen after every stroke (not in shipping builds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Debug")
	bool bShowWardReading = true;

	/** Enclosed areas smaller than this are ignored (e.g. the tiny hole inside a dot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canting|Rules", meta = (ClampMin = "0.0"))
	float MinEnclosedAreaCm2 = 1.0f;

	/** Fires after a stroke ends, an undo, a clear, or a load. */
	UPROPERTY(BlueprintAssignable, Category = "Canting")
	FOnCantingInscriptionChanged OnInscriptionChanged;

	/** Call every tick while drawing. Starts a stroke on the first call, and starts a new one after a missed frame. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	void DrawAtUV(FVector2D UV);

	/** Ends the current stroke now. Optional: the component also ends it one frame after DrawAtUV stops. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	void EndStroke();

	/** Removes the most recent stroke. Returns false if there was nothing to undo. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	bool UndoLastStroke();

	/** Removes all strokes. */
	UFUNCTION(BlueprintCallable, Category = "Canting")
	void ClearInscription();

	/**
	 * Writes the strokes to <project>/CantingDrawings/<Name>.txt, for tuning and replaying drawings.
	 * Console: Canting.SaveStrokes <Name>
	 */
	UFUNCTION(BlueprintCallable, Category = "Canting|Debug")
	bool SaveStrokesToFile(const FString& Name, FString& OutPath);

	/**
	 * Replaces the drawing with the strokes in <project>/CantingDrawings/<Name>.txt.
	 * Console: Canting.LoadStrokes <Name>
	 */
	UFUNCTION(BlueprintCallable, Category = "Canting|Debug")
	bool LoadStrokesFromFile(const FString& Name);

	UFUNCTION(BlueprintPure, Category = "Canting")
	FCantingInscriptionStats GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category = "Canting")
	TArray<FCantingStroke> GetStrokes() const { return Strokes; }

	UFUNCTION(BlueprintPure, Category = "Canting")
	bool IsStrokeActive() const { return bStrokeActive; }

	UFUNCTION(BlueprintPure, Category = "Canting")
	UTexture2D* GetWaxTexture() const { return WaxTexture; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void InitCanvas();
	void ConnectMaterial();
	void SetStrokeActive(bool bActive);
	CantingCore::FRectI StampStroke(const FCantingStroke& Stroke, const CantingCore::FRectI* Clip);
	CantingCore::FRectI StampLastPoints(const FCantingStroke& Stroke, int32 NumPoints);
	CantingCore::FRectI StrokeBounds(const FCantingStroke& Stroke) const;
	void RebuildRegion(const CantingCore::FRectI& Region);
	void RefreshAnalysis();
	void PackRect(const CantingCore::FRectI& Rect, uint8* Dest, int32 DestPitch) const;
	void UploadRect(const CantingCore::FRectI& Rect);
	static FString DrawingPath(const FString& Name);

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
	std::vector<uint8_t> FilterMask;
	std::vector<uint8_t> DeflectMask;
	FVector2D LastSampleCm = FVector2D::ZeroVector;
	double TexelsPerCm = 1.0;
	/** Engine frame of the last DrawAtUV call; a gap of a frame or more ends the stroke. */
	uint64 LastDrawFrame = 0;
	bool bStrokeActive = false;
};
