#include "CraftLoopWidgets.h"

#include "CraftLoopSubsystem.h"
#include "CraftRules.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "CraftLoopWidgets"

// ---- Palette ---------------------------------------------------------------------------------------------------------
// High-contrast text on a near-opaque dark panel; warm accent for the craft, clear red/green for good and bad.

namespace CraftStyle
{
	const FLinearColor TextColor(0.97f, 0.95f, 0.91f);
	const FLinearColor Muted(0.80f, 0.77f, 0.72f);
	const FLinearColor Accent(1.0f, 0.76f, 0.36f);
	const FLinearColor Good(0.52f, 0.86f, 0.50f);
	const FLinearColor Warning(1.0f, 0.50f, 0.40f);
	const FLinearColor Panel(0.035f, 0.03f, 0.026f, 0.95f);
	const FLinearColor Cloth(0.93f, 0.89f, 0.80f);
	const FLinearColor ClothLine(0.55f, 0.50f, 0.42f, 0.45f);

	FSlateFontInfo Font(int32 Size, bool bBold)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
	}

	const FSlateBrush* WhiteBrush()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}
}

namespace
{
	const FLinearColor PieceColors[] = {
		FLinearColor(0.80f, 0.42f, 0.22f),
		FLinearColor(0.24f, 0.52f, 0.72f),
		FLinearColor(0.46f, 0.66f, 0.28f),
		FLinearColor(0.66f, 0.40f, 0.70f),
		FLinearColor(0.78f, 0.62f, 0.20f),
	};
	constexpr float TrayScale = 0.55f;
	constexpr float HeaderHeight = 26.0f;

	FLinearColor PieceColor(int32 Index)
	{
		return PieceColors[Index % UE_ARRAY_COUNT(PieceColors)];
	}

	FPaintGeometry Rect(const FGeometry& Geometry, const FVector2D& Position, const FVector2D& Size)
	{
		return Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position)));
	}

	void DrawBox(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Out, Layer, Rect(Geometry, Position, Size), CraftStyle::WhiteBrush(), ESlateDrawEffect::None, Color);
	}

	void DrawOutline(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2D& Position, const FVector2D& Size,
		const FLinearColor& Color, float Thickness = 2.0f)
	{
		const FVector2f P(Position);
		const FVector2f S(Size);
		TArray<FVector2f> Points = { P, P + FVector2f(S.X, 0.0f), P + S, P + FVector2f(0.0f, S.Y), P };
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
	}

	void DrawLine(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2D& A, const FVector2D& B,
		const FLinearColor& Color, float Thickness = 1.0f)
	{
		TArray<FVector2f> Points = { FVector2f(A), FVector2f(B) };
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
	}

	void DrawText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2D& Position, const FText& Text,
		const FSlateFontInfo& Font, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(Out, Layer, Rect(Geometry, Position, FVector2D(400.0, Font.Size * 1.6)), Text, Font, ESlateDrawEffect::None, Color);
	}

	FVector2D ToLocal(const FGeometry& Geometry, const FPointerEvent& Event)
	{
		return FVector2D(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
	}
}

// ---- Arrange board ---------------------------------------------------------------------------------------------------

void SCraftArrangeBoard::Construct(const FArguments& InArgs)
{
	Loop = InArgs._Loop;
}

FVector2D SCraftArrangeBoard::ComputeDesiredSize(float) const
{
	return FVector2D(450.0, 380.0);
}

bool SCraftArrangeBoard::ComputeLayout(FLayout& Out) const
{
	const UCraftLoopSubsystem* Subsystem = Loop.Get();
	FCraftFabric Fabric;
	if (!Subsystem || !Subsystem->GetChosenFabric(Fabric))
	{
		return false;
	}
	const FVector2D Size = GetCachedGeometry().GetLocalSize().X > 0.0 ? FVector2D(GetCachedGeometry().GetLocalSize()) : ComputeDesiredSize(1.0f);
	Out.SheetW = Fabric.SheetWidth;
	Out.SheetH = Fabric.SheetHeight;
	Out.Cell = FMath::Max(8.0f, FMath::Min(static_cast<float>(Size.X * 0.58 / Out.SheetW), static_cast<float>((Size.Y - HeaderHeight - 4.0) / Out.SheetH)));
	Out.SheetOrigin = FVector2D(0.0, HeaderHeight);
	Out.TrayOrigin = FVector2D(Out.SheetW * Out.Cell + 22.0, HeaderHeight);
	return true;
}

FVector2D SCraftArrangeBoard::PieceSizePx(const FLayout& Layout, int32 Index, bool bRotated) const
{
	FCraftGarmentBlueprint Blueprint;
	if (!Loop.IsValid() || !Loop->GetChosenBlueprint(Blueprint) || !Blueprint.Pieces.IsValidIndex(Index))
	{
		return FVector2D::ZeroVector;
	}
	const FIntPoint Cells = UCraftRules::PieceFootprint(Blueprint.Pieces[Index], bRotated);
	return FVector2D(Cells.X * Layout.Cell, Cells.Y * Layout.Cell);
}

FVector2D SCraftArrangeBoard::PiecePosition(const FLayout& Layout, int32 Index) const
{
	const TArray<FCraftPiecePlacement> Placements = Loop->GetPlacements();
	if (Placements.IsValidIndex(Index) && Placements[Index].bPlaced)
	{
		return Layout.SheetOrigin + FVector2D(Placements[Index].X * Layout.Cell, Placements[Index].Y * Layout.Cell);
	}
	// Tray: unplaced pieces stacked in a column at tray scale, wrapping to a new column when full.
	const double MaxY = GetCachedGeometry().GetLocalSize().Y > 0.0 ? GetCachedGeometry().GetLocalSize().Y : ComputeDesiredSize(1.0f).Y;
	FVector2D Cursor = Layout.TrayOrigin + FVector2D(0.0, 22.0);
	double ColumnWidth = 0.0;
	for (int32 Other = 0; Other < Placements.Num(); ++Other)
	{
		if (Placements[Other].bPlaced)
		{
			continue;
		}
		const FVector2D Size = PieceSizePx(Layout, Other, false) * TrayScale;
		if (Cursor.Y + Size.Y > MaxY && Cursor.Y > Layout.TrayOrigin.Y + 22.0)
		{
			Cursor = FVector2D(Cursor.X + ColumnWidth + 10.0, Layout.TrayOrigin.Y + 22.0);
			ColumnWidth = 0.0;
		}
		if (Other == Index)
		{
			return Cursor;
		}
		Cursor.Y += Size.Y + 10.0;
		ColumnWidth = FMath::Max(ColumnWidth, Size.X);
	}
	return Cursor;
}

int32 SCraftArrangeBoard::HitTestPiece(const FLayout& Layout, const FVector2D& Local) const
{
	const TArray<FCraftPiecePlacement> Placements = Loop->GetPlacements();
	for (int32 Index = Placements.Num() - 1; Index >= 0; --Index)
	{
		const bool bPlaced = Placements[Index].bPlaced;
		const FVector2D Position = PiecePosition(Layout, Index);
		const FVector2D Size = bPlaced ? PieceSizePx(Layout, Index, Placements[Index].bRotated) : PieceSizePx(Layout, Index, false) * TrayScale;
		if (Local.X >= Position.X && Local.Y >= Position.Y && Local.X <= Position.X + Size.X && Local.Y <= Position.Y + Size.Y)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

FIntPoint SCraftArrangeBoard::DropCell(const FLayout& Layout) const
{
	const FVector2D TopLeft = DragMouse - DragGrabOffset - Layout.SheetOrigin;
	return FIntPoint(FMath::RoundToInt(TopLeft.X / Layout.Cell), FMath::RoundToInt(TopLeft.Y / Layout.Cell));
}

bool SCraftArrangeBoard::IsDropValid(const FLayout& Layout) const
{
	FCraftGarmentBlueprint Blueprint;
	FCraftFabric Fabric;
	if (!Loop.IsValid() || !Loop->GetChosenBlueprint(Blueprint) || !Loop->GetChosenFabric(Fabric))
	{
		return false;
	}
	const FIntPoint Cell = DropCell(Layout);
	FCraftPiecePlacement Candidate;
	Candidate.bPlaced = true;
	Candidate.X = Cell.X;
	Candidate.Y = Cell.Y;
	Candidate.bRotated = bDragRotated;
	return UCraftRules::IsPlacementValid(Fabric, Blueprint, Loop->GetPlacements(), DragIndex, Candidate);
}

FReply SCraftArrangeBoard::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FLayout Layout;
	if (!ComputeLayout(Layout))
	{
		return FReply::Unhandled();
	}
	const FVector2D Local = ToLocal(MyGeometry, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (DragIndex != INDEX_NONE)
		{
			// Turn the piece in hand.
			bDragRotated = !bDragRotated;
			DragGrabOffset = FVector2D(DragGrabOffset.Y, DragGrabOffset.X);
			return FReply::Handled();
		}
		// Turn a placed piece where it lies, if it still fits.
		const int32 Index = HitTestPiece(Layout, Local);
		const TArray<FCraftPiecePlacement> Placements = Loop->GetPlacements();
		if (Placements.IsValidIndex(Index) && Placements[Index].bPlaced)
		{
			Loop->PlacePiece(Index, Placements[Index].X, Placements[Index].Y, !Placements[Index].bRotated);
		}
		return FReply::Handled();
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	const int32 Index = HitTestPiece(Layout, Local);
	if (Index == INDEX_NONE)
	{
		return FReply::Handled();
	}
	const TArray<FCraftPiecePlacement> Placements = Loop->GetPlacements();
	const bool bPlaced = Placements[Index].bPlaced;
	DragIndex = Index;
	bDragRotated = bPlaced && Placements[Index].bRotated;
	const FVector2D Position = PiecePosition(Layout, Index);
	const double Scale = bPlaced ? 1.0 : TrayScale;
	DragGrabOffset = (Local - Position) / Scale;
	DragMouse = Local;
	return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SCraftArrangeBoard::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DragIndex == INDEX_NONE)
	{
		return FReply::Unhandled();
	}
	DragMouse = ToLocal(MyGeometry, MouseEvent);
	return FReply::Handled();
}

FReply SCraftArrangeBoard::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DragIndex == INDEX_NONE || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}
	FLayout Layout;
	const int32 Index = DragIndex;
	DragIndex = INDEX_NONE;
	DragMouse = ToLocal(MyGeometry, MouseEvent);
	if (ComputeLayout(Layout) && Loop.IsValid())
	{
		DragIndex = Index; // IsDropValid and DropCell read the drag state
		const bool bValid = IsDropValid(Layout);
		const FIntPoint Cell = DropCell(Layout);
		const bool bOverSheet = DragMouse.X >= Layout.SheetOrigin.X && DragMouse.X <= Layout.SheetOrigin.X + Layout.SheetW * Layout.Cell
			&& DragMouse.Y >= Layout.SheetOrigin.Y && DragMouse.Y <= Layout.SheetOrigin.Y + Layout.SheetH * Layout.Cell;
		DragIndex = INDEX_NONE;
		if (bValid)
		{
			Loop->PlacePiece(Index, Cell.X, Cell.Y, bDragRotated);
		}
		else if (!bOverSheet)
		{
			Loop->RemovePiece(Index);
		}
		// Invalid drop on the sheet: the piece snaps back to where it was.
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SCraftArrangeBoard::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (DragIndex != INDEX_NONE && InKeyEvent.GetKey() == EKeys::R)
	{
		bDragRotated = !bDragRotated;
		DragGrabOffset = FVector2D(DragGrabOffset.Y, DragGrabOffset.X);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

int32 SCraftArrangeBoard::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FLayout Layout;
	FCraftGarmentBlueprint Blueprint;
	if (!ComputeLayout(Layout) || !Loop.IsValid() || !Loop->GetChosenBlueprint(Blueprint))
	{
		return LayerId;
	}
	const TArray<FCraftPiecePlacement> Placements = Loop->GetPlacements();
	const FVector2D SheetSize(Layout.SheetW * Layout.Cell, Layout.SheetH * Layout.Cell);
	const FSlateFontInfo LabelFont = CraftStyle::Font(12, true);
	const FSlateFontInfo SmallFont = CraftStyle::Font(11, true);

	// Header labels
	DrawText(OutDrawElements, LayerId, AllottedGeometry, FVector2D(0.0, 2.0), LOCTEXT("SheetLabel", "Fabric (grain runs top to bottom)"), LabelFont, CraftStyle::Muted);
	DrawText(OutDrawElements, LayerId, AllottedGeometry, Layout.TrayOrigin + FVector2D(0.0, -24.0), LOCTEXT("TrayLabel", "Pieces"), LabelFont, CraftStyle::Muted);

	// Sheet with cell grid; slightly stronger vertical lines show the grain.
	DrawBox(OutDrawElements, LayerId, AllottedGeometry, Layout.SheetOrigin, SheetSize, CraftStyle::Cloth);
	for (int32 X = 1; X < Layout.SheetW; ++X)
	{
		const double PX = Layout.SheetOrigin.X + X * Layout.Cell;
		DrawLine(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(PX, Layout.SheetOrigin.Y), FVector2D(PX, Layout.SheetOrigin.Y + SheetSize.Y), CraftStyle::ClothLine, 1.0f);
	}
	for (int32 Y = 1; Y < Layout.SheetH; ++Y)
	{
		const double PY = Layout.SheetOrigin.Y + Y * Layout.Cell;
		DrawLine(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(Layout.SheetOrigin.X, PY), FVector2D(Layout.SheetOrigin.X + SheetSize.X, PY),
			CraftStyle::ClothLine * FLinearColor(1.0f, 1.0f, 1.0f, 0.5f), 1.0f);
	}
	DrawOutline(OutDrawElements, LayerId + 1, AllottedGeometry, Layout.SheetOrigin, SheetSize, FLinearColor(0.45f, 0.40f, 0.33f), 2.0f);

	// Pieces (placed on the sheet, or waiting in the tray), skipping the one in hand.
	const int32 PieceLayer = LayerId + 2;
	for (int32 Index = 0; Index < Placements.Num() && Index < Blueprint.Pieces.Num(); ++Index)
	{
		if (Index == DragIndex)
		{
			continue;
		}
		const bool bPlaced = Placements[Index].bPlaced;
		const FVector2D Position = PiecePosition(Layout, Index);
		const FVector2D Size = bPlaced ? PieceSizePx(Layout, Index, Placements[Index].bRotated) : PieceSizePx(Layout, Index, false) * TrayScale;
		DrawBox(OutDrawElements, PieceLayer, AllottedGeometry, Position, Size, PieceColor(Index));
		DrawOutline(OutDrawElements, PieceLayer + 1, AllottedGeometry, Position, Size, FLinearColor(0.08f, 0.06f, 0.05f), 2.0f);
		DrawText(OutDrawElements, PieceLayer + 2, AllottedGeometry, Position + FVector2D(5.0, 3.0), Blueprint.Pieces[Index].Name, SmallFont, FLinearColor::White);
		if (bPlaced && Placements[Index].bRotated)
		{
			DrawText(OutDrawElements, PieceLayer + 2, AllottedGeometry, Position + FVector2D(5.0, 20.0), LOCTEXT("Across", "across grain"), CraftStyle::Font(10), FLinearColor::White);
		}
	}

	// Piece in hand: a snapped ghost on the sheet (green = fits, red = doesn't) and the piece under the cursor.
	if (DragIndex != INDEX_NONE)
	{
		const int32 DragLayer = LayerId + 6;
		const FVector2D Size = PieceSizePx(Layout, DragIndex, bDragRotated);
		const FIntPoint Cell = DropCell(Layout);
		const FVector2D Ghost = Layout.SheetOrigin + FVector2D(Cell.X * Layout.Cell, Cell.Y * Layout.Cell);
		const bool bValid = IsDropValid(Layout);
		DrawBox(OutDrawElements, DragLayer, AllottedGeometry, Ghost, Size, (bValid ? CraftStyle::Good : CraftStyle::Warning) * FLinearColor(1.0f, 1.0f, 1.0f, 0.45f));
		DrawOutline(OutDrawElements, DragLayer + 1, AllottedGeometry, Ghost, Size, bValid ? CraftStyle::Good : CraftStyle::Warning, 2.0f);

		const FVector2D Held = DragMouse - DragGrabOffset;
		DrawBox(OutDrawElements, DragLayer + 2, AllottedGeometry, Held, Size, PieceColor(DragIndex) * FLinearColor(1.0f, 1.0f, 1.0f, 0.8f));
		DrawOutline(OutDrawElements, DragLayer + 3, AllottedGeometry, Held, Size, FLinearColor::White, 2.0f);
		DrawText(OutDrawElements, DragLayer + 4, AllottedGeometry, Held + FVector2D(5.0, 3.0), Blueprint.Pieces[DragIndex].Name, SmallFont, FLinearColor::White);
		return DragLayer + 4;
	}
	return PieceLayer + 2;
}

// ---- Cut panel -------------------------------------------------------------------------------------------------------

namespace
{
	constexpr float CutTop = 18.0f;
	constexpr float CutBottomPad = 22.0f;

	float CutBaseHeight(const FGeometry& Geometry)
	{
		return (static_cast<float>(Geometry.GetLocalSize().Y) - CutTop - CutBottomPad) / UCraftRules::MaxCutLength;
	}
}

void SCraftCutPanel::Construct(const FArguments& InArgs)
{
	Loop = InArgs._Loop;
}

float SCraftCutPanel::LengthAtY(const FGeometry& Geometry, float LocalY) const
{
	return FMath::Clamp((LocalY - CutTop) / FMath::Max(CutBaseHeight(Geometry), 1.0f), UCraftRules::MinCutLength, UCraftRules::MaxCutLength);
}

float SCraftCutPanel::YForLength(const FGeometry& Geometry, float Length) const
{
	return CutTop + Length * CutBaseHeight(Geometry);
}

FReply SCraftCutPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Loop.IsValid())
	{
		return FReply::Unhandled();
	}
	bDragging = true;
	Loop->SetCutLength(LengthAtY(MyGeometry, static_cast<float>(ToLocal(MyGeometry, MouseEvent).Y)));
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SCraftCutPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bDragging || !Loop.IsValid())
	{
		return FReply::Unhandled();
	}
	Loop->SetCutLength(LengthAtY(MyGeometry, static_cast<float>(ToLocal(MyGeometry, MouseEvent).Y)));
	return FReply::Handled();
}

FReply SCraftCutPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bDragging = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SCraftCutPanel::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::ResizeUpDown);
}

int32 SCraftCutPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!Loop.IsValid())
	{
		return LayerId;
	}
	const FVector2D Size(AllottedGeometry.GetLocalSize());
	const double CenterX = Size.X * 0.36;
	const double TopHalfWidth = 60.0;
	const double FlareHalfWidth = 120.0;  // half-width at the longest possible hem
	const float Length = Loop->GetCutLength();
	const double HemY = YForLength(AllottedGeometry, Length);
	const double MaxY = YForLength(AllottedGeometry, UCraftRules::MaxCutLength);

	auto HalfWidthAt = [&](double Y)
	{
		const double T = FMath::Clamp((Y - CutTop) / FMath::Max(MaxY - CutTop, 1.0), 0.0, 1.0);
		return FMath::Lerp(TopHalfWidth, FlareHalfWidth, T);
	};

	// Faint outline of the full possible panel.
	{
		TArray<FVector2f> Ghost = {
			FVector2f(CenterX - TopHalfWidth, CutTop), FVector2f(CenterX + TopHalfWidth, CutTop),
			FVector2f(CenterX + FlareHalfWidth, MaxY), FVector2f(CenterX - FlareHalfWidth, MaxY), FVector2f(CenterX - TopHalfWidth, CutTop) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Ghost, ESlateDrawEffect::None,
			CraftStyle::Muted * FLinearColor(1.0f, 1.0f, 1.0f, 0.5f), true, 1.5f);
	}

	// The panel as it will be cut: filled in horizontal slices down to the hem.
	constexpr int32 Slices = 48;
	for (int32 Slice = 0; Slice < Slices; ++Slice)
	{
		const double Y0 = FMath::Lerp(static_cast<double>(CutTop), HemY, static_cast<double>(Slice) / Slices);
		const double Y1 = FMath::Lerp(static_cast<double>(CutTop), HemY, static_cast<double>(Slice + 1) / Slices);
		const double Half = HalfWidthAt(0.5 * (Y0 + Y1));
		DrawBox(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(CenterX - Half, Y0), FVector2D(2.0 * Half, Y1 - Y0 + 0.5), CraftStyle::Cloth);
	}
	{
		const double HemHalf = HalfWidthAt(HemY);
		TArray<FVector2f> Outline = {
			FVector2f(CenterX - HemHalf, HemY), FVector2f(CenterX - TopHalfWidth, CutTop), FVector2f(CenterX + TopHalfWidth, CutTop), FVector2f(CenterX + HemHalf, HemY) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Outline, ESlateDrawEffect::None,
			FLinearColor(0.45f, 0.40f, 0.33f), true, 2.0f);
	}

	// Reference marks: short, blueprint, long.
	const FSlateFontInfo MarkFont = CraftStyle::Font(12, true);
	const struct { float Length; FText Label; } Marks[] = {
		{ UCraftRules::MinCutLength, LOCTEXT("MarkShort", "Short") },
		{ 1.0f, LOCTEXT("MarkStandard", "Blueprint") },
		{ UCraftRules::MaxCutLength, LOCTEXT("MarkLong", "Long") } };
	const double MarkX = CenterX + FlareHalfWidth + 14.0;
	for (const auto& Mark : Marks)
	{
		const double Y = YForLength(AllottedGeometry, Mark.Length);
		DrawLine(OutDrawElements, LayerId + 2, AllottedGeometry, FVector2D(MarkX, Y), FVector2D(MarkX + 14.0, Y), CraftStyle::Muted, 2.0f);
		DrawText(OutDrawElements, LayerId + 2, AllottedGeometry, FVector2D(MarkX + 20.0, Y - 10.0), Mark.Label, MarkFont, CraftStyle::Muted);
	}

	// The hem line you drag, with a handle.
	const double HemHalf = HalfWidthAt(HemY);
	DrawLine(OutDrawElements, LayerId + 3, AllottedGeometry, FVector2D(CenterX - HemHalf - 12.0, HemY), FVector2D(CenterX + HemHalf + 12.0, HemY), CraftStyle::Accent, 4.0f);
	DrawBox(OutDrawElements, LayerId + 3, AllottedGeometry, FVector2D(CenterX + HemHalf + 4.0, HemY - 9.0), FVector2D(18.0, 18.0), CraftStyle::Accent);
	DrawText(OutDrawElements, LayerId + 4, AllottedGeometry, FVector2D(CenterX - 54.0, CutTop + 8.0), LOCTEXT("DragHem", "Drag the hem"), CraftStyle::Font(12, true),
		FLinearColor(0.25f, 0.2f, 0.15f));
	return LayerId + 4;
}

// ---- Gauge -----------------------------------------------------------------------------------------------------------

void SCraftGauge::Construct(const FArguments& InArgs)
{
	Value = InArgs._Value;
	Zones = InArgs._Zones;
	Height = InArgs._Height;
	bFill = InArgs._bFill;
}

int32 SCraftGauge::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size(AllottedGeometry.GetLocalSize());
	DrawBox(OutDrawElements, LayerId, AllottedGeometry, FVector2D::ZeroVector, Size, FLinearColor(0.16f, 0.14f, 0.12f));
	for (const FCraftGaugeZone& Zone : Zones)
	{
		DrawBox(OutDrawElements, LayerId + 1, AllottedGeometry, FVector2D(Zone.Min * Size.X, 0.0), FVector2D((Zone.Max - Zone.Min) * Size.X, Size.Y), Zone.Color);
	}
	const double V = FMath::Clamp(static_cast<double>(Value.Get()), 0.0, 1.0);
	if (bFill)
	{
		DrawBox(OutDrawElements, LayerId + 2, AllottedGeometry, FVector2D(0.0, Size.Y * 0.3), FVector2D(V * Size.X, Size.Y * 0.4), FLinearColor(0.97f, 0.95f, 0.91f, 0.9f));
	}
	DrawBox(OutDrawElements, LayerId + 3, AllottedGeometry, FVector2D(V * Size.X - 2.0, -3.0), FVector2D(4.0, Size.Y + 6.0), FLinearColor::White);
	DrawOutline(OutDrawElements, LayerId + 3, AllottedGeometry, FVector2D::ZeroVector, Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), 2.0f);
	return LayerId + 3;
}

#undef LOCTEXT_NAMESPACE
