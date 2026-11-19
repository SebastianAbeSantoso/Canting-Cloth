// Built-in Slate UI for the craft loop: a header (stage, hour, gold), one panel per stage, and notices.
// Deliberately plain (handoff pillar P5: minimal, purposeful UI). Replaceable by Blueprint widgets:
// turn off bShowBuiltInUI in Project Settings and drive UCraftLoopSubsystem directly.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class UCraftLoopSubsystem;

class SCraftLoopHUD : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCraftLoopHUD) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UCraftLoopSubsystem>, Loop)
	SLATE_END_ARGS()

	SCraftLoopHUD();

	void Construct(const FArguments& InArgs);

	/** Rebuilds the stage panel from the subsystem's current state. */
	void Refresh();

	void ShowNotice(const FText& Message);

private:
	TSharedRef<SWidget> BuildPanel();
	TSharedRef<SWidget> BuildCommission();
	TSharedRef<SWidget> BuildBlueprintChoice();
	TSharedRef<SWidget> BuildFabricChoice();
	TSharedRef<SWidget> BuildArrange();
	TSharedRef<SWidget> BuildCut();
	TSharedRef<SWidget> BuildCanting();
	TSharedRef<SWidget> BuildDye();
	TSharedRef<SWidget> BuildStitch();
	TSharedRef<SWidget> BuildInspect();
	TSharedRef<SWidget> BuildDeliver();
	TSharedRef<SWidget> BuildExpedition();
	TSharedRef<SWidget> BuildReturn();
	TSharedRef<SWidget> BuildFinished();

	FText GetHeaderText() const;
	EVisibility GetNoticeVisibility() const;

	TWeakObjectPtr<UCraftLoopSubsystem> Loop;
	TSharedPtr<SBox> PanelBox;
	FText NoticeText;
	double NoticeShownAt = -1000.0;

	FSlateColorBrush PanelBrush;
	FSlateColorBrush NoticeBrush;
};
