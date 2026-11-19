// Interactive Slate widgets for the craft stages: the fabric layout board, the hem panel, and timing gauges.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SLeafWidget.h"

class UCraftLoopSubsystem;
struct FButtonStyle;

/** Shared palette so every craft widget reads the same. Colours are authored in sRGB. */
namespace CraftStyle
{
	/** Colour from on-screen (sRGB) 0-255 values. */
	FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255);

	extern const FLinearColor TextColor;
	extern const FLinearColor Muted;
	extern const FLinearColor Accent;
	extern const FLinearColor Good;
	extern const FLinearColor Warning;
	extern const FLinearColor Panel;
	extern const FLinearColor Cloth;
	extern const FLinearColor ClothLine;
	extern const FLinearColor ButtonText;
	extern const FLinearColor ButtonDetail;
	extern const FLinearColor DisabledText;

	FSlateFontInfo Font(int32 Size, bool bBold = false);
	/** Bold font with a dark outline, for labels drawn over coloured shapes. */
	FSlateFontInfo OutlinedFont(int32 Size);
	const FSlateBrush* WhiteBrush();
	/** Solid cream button, near-black text; dark grey when disabled. */
	const FButtonStyle& ButtonStyle();
}

/**
 * Arrange stage: the fabric sheet as a grid (grain runs top to bottom) and a tray of pattern pieces.
 * Drag a piece onto the sheet; right-click (or press R while dragging) to turn it across the grain.
 * Dropping it off the sheet puts it back in the tray.
 */
class SCraftArrangeBoard : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCraftArrangeBoard) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UCraftLoopSubsystem>, Loop)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	struct FLayout
	{
		float Cell = 20.0f;
		FVector2D SheetOrigin = FVector2D::ZeroVector;
		FVector2D TrayOrigin = FVector2D::ZeroVector;
		int32 SheetW = 0;
		int32 SheetH = 0;
	};

	bool ComputeLayout(FLayout& Out) const;
	/** Top-left of a piece in local pixels: on the sheet if placed, otherwise its slot in the tray. */
	FVector2D PiecePosition(const FLayout& Layout, int32 Index) const;
	FVector2D PieceSizePx(const FLayout& Layout, int32 Index, bool bRotated) const;
	int32 HitTestPiece(const FLayout& Layout, const FVector2D& Local) const;
	/** Sheet cell under the dragged piece's top-left corner, snapped. */
	FIntPoint DropCell(const FLayout& Layout) const;
	bool IsDropValid(const FLayout& Layout) const;

	TWeakObjectPtr<UCraftLoopSubsystem> Loop;
	int32 DragIndex = INDEX_NONE;
	bool bDragRotated = false;
	FVector2D DragGrabOffset = FVector2D::ZeroVector;
	FVector2D DragMouse = FVector2D::ZeroVector;
};

/** Cut stage: the main panel's outline with a hem line you drag up (shorter) or down (longer). */
class SCraftCutPanel : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCraftCutPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UCraftLoopSubsystem>, Loop)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(440.0, 250.0); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	float LengthAtY(const FGeometry& Geometry, float LocalY) const;
	float YForLength(const FGeometry& Geometry, float Length) const;

	TWeakObjectPtr<UCraftLoopSubsystem> Loop;
	bool bDragging = false;
};

/** A horizontal bar with coloured zones and a moving marker (dye depth, needle position). */
struct FCraftGaugeZone
{
	float Min = 0.0f;
	float Max = 1.0f;
	FLinearColor Color = FLinearColor::White;
};

class SCraftGauge : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCraftGauge)
		: _Height(34.0f)
		, _bFill(false)
	{}
		SLATE_ATTRIBUTE(float, Value)
		SLATE_ARGUMENT(TArray<FCraftGaugeZone>, Zones)
		SLATE_ARGUMENT(float, Height)
		/** Fill up to the value (a level) instead of drawing a single marker line. */
		SLATE_ARGUMENT(bool, bFill)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300.0, Height); }

private:
	TAttribute<float> Value;
	TArray<FCraftGaugeZone> Zones;
	float Height = 34.0f;
	bool bFill = false;
};
