#include "CraftLoopHUD.h"

#include "CraftLoopSubsystem.h"
#include "CraftLoopWidgets.h"
#include "CraftRules.h"
#include "CraftSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "CraftLoopHUD"

namespace CraftHUD
{
	using namespace CraftStyle;

	constexpr float PanelWidth = 500.0f;
	constexpr double NoticeSeconds = 7.0;

	// Sizes chosen for reading at a distance on a 1080p screen.
	constexpr int32 TitleSize = 24;
	constexpr int32 BodySize = 16;
	constexpr int32 DetailSize = 14;

	const FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.9f);

	TSharedRef<SWidget> Title(const FText& Text)
	{
		return SNew(STextBlock).Text(Text).Font(Font(TitleSize, true)).ColorAndOpacity(Accent).AutoWrapText(true)
			.ShadowOffset(FVector2D(1.0, 1.0)).ShadowColorAndOpacity(Shadow);
	}

	TSharedRef<SWidget> Body(const FText& Text, const FLinearColor& Color = CraftStyle::TextColor)
	{
		return SNew(STextBlock).Text(Text).Font(Font(BodySize)).ColorAndOpacity(Color).AutoWrapText(true).LineHeightPercentage(1.1f);
	}

	TSharedRef<SWidget> LiveText(TAttribute<FText> Text, const FLinearColor& Color = CraftStyle::TextColor, int32 Size = BodySize, bool bBold = false)
	{
		return SNew(STextBlock).Text(Text).Font(Font(Size, bBold)).ColorAndOpacity(Color).AutoWrapText(true);
	}

	TSharedRef<SWidget> Stat(const FText& Label, const FText& Value, const FLinearColor& Color = CraftStyle::TextColor)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(Label).Font(Font(BodySize)).ColorAndOpacity(Muted)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text(Value).Font(Font(BodySize, true)).ColorAndOpacity(Color)
			];
	}

	/** A solid cream button with near-black text (~17:1); dark grey with light text when disabled.
	 *  Optional press/release events for hold actions. */
	TSharedRef<SButton> MakeButton(const FText& Label, const FText& Detail, TAttribute<bool> Enabled,
		FSimpleDelegate OnPressed = FSimpleDelegate(), FSimpleDelegate OnReleased = FSimpleDelegate())
	{
		TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(Font(BodySize + 1, true))
				.ColorAndOpacity_Lambda([Enabled]() { return FSlateColor(Enabled.Get() ? CraftStyle::ButtonText : CraftStyle::DisabledText); })
			];
		if (!Detail.IsEmpty())
		{
			Content->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Detail).Font(Font(DetailSize)).AutoWrapText(true)
				.ColorAndOpacity_Lambda([Enabled]() { return FSlateColor(Enabled.Get() ? CraftStyle::ButtonDetail : CraftStyle::DisabledText); })
			];
		}
		return SNew(SButton)
			.ButtonStyle(&CraftStyle::ButtonStyle())
			.IsEnabled(Enabled)
			.OnPressed(OnPressed)
			.OnReleased(OnReleased)
			.ContentPadding(FMargin(16.0f, 11.0f))
			[
				Content
			];
	}

	TSharedRef<SWidget> Button(const FText& Label, const FText& Detail, TFunction<void()> OnClick, TAttribute<bool> Enabled = true)
	{
		TSharedRef<SButton> Result = MakeButton(Label, Detail, Enabled);
		Result->SetOnClicked(FOnClicked::CreateLambda([OnClick]()
		{
			OnClick();
			return FReply::Handled();
		}));
		return Result;
	}

	FText EffectName(ECraftEffectFamily Effect)
	{
		switch (Effect)
		{
		case ECraftEffectFamily::WardPhysical: return LOCTEXT("WardPhysical", "Ward against blows");
		default: return LOCTEXT("WardPoison", "Ward against miasma");
		}
	}

	FText StageName(ECraftStage Stage)
	{
		switch (Stage)
		{
		case ECraftStage::Commission: return LOCTEXT("StageCommission", "Commission");
		case ECraftStage::SelectBlueprint: return LOCTEXT("StageBlueprint", "Choose a blueprint");
		case ECraftStage::SelectCloth: return LOCTEXT("StageCloth", "Choose the cloth");
		case ECraftStage::Arrange: return LOCTEXT("StageArrange", "Arrange");
		case ECraftStage::Cut: return LOCTEXT("StageCut", "Cut");
		case ECraftStage::Canting: return LOCTEXT("StageCanting", "Canting");
		case ECraftStage::Dye: return LOCTEXT("StageDye", "Dye & reveal");
		case ECraftStage::Stitch: return LOCTEXT("StageStitch", "Stitch");
		case ECraftStage::Inspect: return LOCTEXT("StageInspect", "Inspect");
		case ECraftStage::Deliver: return LOCTEXT("StageDeliver", "Deliver");
		case ECraftStage::Expedition: return LOCTEXT("StageExpedition", "Expedition");
		case ECraftStage::Return: return LOCTEXT("StageReturn", "Return");
		default: return LOCTEXT("StageFinished", "Day's end");
		}
	}

	FText Percent(float Value)
	{
		FNumberFormattingOptions Options;
		Options.SetUseGrouping(false).SetMaximumFractionalDigits(0);
		return FText::AsPercent(Value, &Options);
	}

	FText Number(float Value, int32 Digits = 1)
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(Digits);
		return FText::AsNumber(Value, &Options);
	}

	FText QualityWord(float Quality)
	{
		if (Quality >= 0.85f) return LOCTEXT("QualityFine", "fine");
		if (Quality >= 0.55f) return LOCTEXT("QualityDecent", "decent");
		if (Quality >= 0.25f) return LOCTEXT("QualityRough", "rough");
		return LOCTEXT("QualityPoor", "poor");
	}

	FText Hours(int32 Value)
	{
		return FText::Format(LOCTEXT("HoursValue", "Takes {0} h."), FText::AsNumber(Value));
	}

	TSharedRef<SVerticalBox> Stack(std::initializer_list<TSharedRef<SWidget>> Widgets)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		for (const TSharedRef<SWidget>& Widget : Widgets)
		{
			Box->AddSlot().AutoHeight().Padding(0.0f, 5.0f)[Widget];
		}
		return Box;
	}

	void Add(const TSharedRef<SVerticalBox>& Box, const TSharedRef<SWidget>& Widget, float Top = 5.0f)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, Top, 0.0f, 5.0f)[Widget];
	}
}

using namespace CraftHUD;

SCraftLoopHUD::SCraftLoopHUD()
	: PanelBrush(CraftStyle::Panel)
	, NoticeBrush(CraftStyle::SRGB(58, 40, 14, 248))
{
}

void SCraftLoopHUD::Construct(const FArguments& InArgs)
{
	Loop = InArgs._Loop;

	ChildSlot
	[
		SNew(SOverlay)
		// Header: stage, clock, purse.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(20.0f)
		[
			SNew(SBorder)
			.BorderImage(&PanelBrush)
			.Padding(FMargin(16.0f, 10.0f))
			[
				SNew(STextBlock)
				.Text(this, &SCraftLoopHUD::GetHeaderText)
				.Font(Font(18, true))
				.ColorAndOpacity(CraftStyle::TextColor)
			]
		]
		// Stage panel on the right, leaving the cloth visible.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(20.0f)
		[
			SNew(SBox)
			.WidthOverride(PanelWidth)
			.MaxDesiredHeight(900.0f)
			[
				SNew(SBorder)
				.BorderImage(&PanelBrush)
				.Padding(20.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(PanelBox, SBox)
					]
				]
			]
		]
		// Notices from scheduled events and stage results.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(20.0f, 20.0f, 20.0f, 48.0f)
		[
			SNew(SBox)
			.MaxDesiredWidth(720.0f)
			.Visibility(this, &SCraftLoopHUD::GetNoticeVisibility)
			[
				SNew(SBorder)
				.BorderImage(&NoticeBrush)
				.Padding(FMargin(18.0f, 12.0f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return NoticeText; })
					.Font(Font(18, true))
					.ColorAndOpacity(CraftStyle::SRGB(255, 236, 196))
					.AutoWrapText(true)
				]
			]
		]
	];

	Refresh();
}

int32 SCraftLoopHUD::PanelKey() const
{
	const UCraftLoopSubsystem* Subsystem = Loop.Get();
	if (!Subsystem)
	{
		return INDEX_NONE;
	}
	int32 Phase = Subsystem->GetStagePhase();
	// "Ready to dip" and "dipping" share one panel: rebuilding would destroy the button being held.
	if (Subsystem->GetStage() == ECraftStage::Dye && Phase == 2)
	{
		Phase = 1;
	}
	return static_cast<int32>(Subsystem->GetStage()) * 10 + Phase;
}

void SCraftLoopHUD::Refresh()
{
	const UCraftLoopSubsystem* Subsystem = Loop.Get();
	if (!PanelBox.IsValid() || !Subsystem)
	{
		return;
	}
	// Interactive panels read live state through lambdas; rebuilding them mid-drag or mid-hold would break the input.
	const ECraftStage Stage = Subsystem->GetStage();
	const bool bInteractive = Stage == ECraftStage::Arrange || Stage == ECraftStage::Cut || Stage == ECraftStage::Dye || Stage == ECraftStage::Stitch;
	const int32 Key = PanelKey();
	if (bInteractive && Key == BuiltPanelKey)
	{
		return;
	}
	BuiltPanelKey = Key;
	PanelBox->SetContent(BuildPanel());
}

void SCraftLoopHUD::ShowNotice(const FText& Message)
{
	NoticeText = Message;
	NoticeShownAt = FPlatformTime::Seconds();
}

EVisibility SCraftLoopHUD::GetNoticeVisibility() const
{
	return (FPlatformTime::Seconds() - NoticeShownAt) < NoticeSeconds ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

FText SCraftLoopHUD::GetHeaderText() const
{
	const UCraftLoopSubsystem* Subsystem = Loop.Get();
	if (!Subsystem)
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("Header", "{0}    Hour {1}    {2} gold"),
		StageName(Subsystem->GetStage()), FText::AsNumber(Subsystem->GetHour()), FText::AsNumber(Subsystem->GetGold()));
}

TSharedRef<SWidget> SCraftLoopHUD::BuildPanel()
{
	const UCraftLoopSubsystem* Subsystem = Loop.Get();
	if (!Subsystem)
	{
		return SNullWidget::NullWidget;
	}
	switch (Subsystem->GetStage())
	{
	case ECraftStage::Commission: return BuildCommission();
	case ECraftStage::SelectBlueprint: return BuildBlueprintChoice();
	case ECraftStage::SelectCloth: return BuildFabricChoice();
	case ECraftStage::Arrange: return BuildArrange();
	case ECraftStage::Cut: return BuildCut();
	case ECraftStage::Canting: return BuildCanting();
	case ECraftStage::Dye: return BuildDye();
	case ECraftStage::Stitch: return BuildStitch();
	case ECraftStage::Inspect: return BuildInspect();
	case ECraftStage::Deliver: return BuildDeliver();
	case ECraftStage::Expedition: return BuildExpedition();
	case ECraftStage::Return: return BuildReturn();
	default: return BuildFinished();
	}
}

// ---- Stage panels ---------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SCraftLoopHUD::BuildCommission()
{
	FCraftCommission Commission;
	Loop->GetCurrentCommission(Commission);
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;

	return Stack({
		Title(Commission.CustomerName),
		Body(Commission.RequestText),
		SNew(SSpacer).Size(FVector2D(1.0f, 6.0f)),
		Stat(LOCTEXT("Needs", "Needs"), EffectName(Commission.RequiredEffect), Accent),
		Stat(LOCTEXT("Danger", "Danger"), FText::Format(LOCTEXT("DangerValue", "{0}, {1}"), Commission.HazardName, Commission.EnemyName)),
		Stat(LOCTEXT("Pay", "Payment"), FText::Format(LOCTEXT("PayValue", "{0} gold"), FText::AsNumber(Commission.RewardGold))),
		SNew(SSpacer).Size(FVector2D(1.0f, 8.0f)),
		Button(LOCTEXT("Accept", "Accept the commission"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->AcceptCommission(); } })
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildBlueprintChoice()
{
	FCraftCommission Commission;
	Loop->GetCurrentCommission(Commission);
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;

	TSharedRef<SVerticalBox> Box = Stack({
		Title(LOCTEXT("BlueprintTitle", "Which garment?")),
		Body(LOCTEXT("BlueprintBody", "A blueprint fixes the silhouette, the pattern pieces and the seams."), Muted)
	});
	for (const FCraftGarmentBlueprint& Blueprint : UCraftSettings::Get()->Blueprints)
	{
		const bool bAllowed = Commission.AllowedBlueprints.Num() == 0 || Commission.AllowedBlueprints.Contains(Blueprint.Id);
		const FName Id = Blueprint.Id;
		const FText Detail = bAllowed
			? FText::Format(LOCTEXT("BlueprintDetail", "{0}\nWeight {1}, wearable up to {2} · coverage {3} · {4} pieces, {5} seams"),
				Blueprint.Description, Number(Blueprint.BaseWeight), Number(Blueprint.MaxWeight), Percent(Blueprint.BaseCoverage),
				FText::AsNumber(Blueprint.Pieces.Num()), FText::AsNumber(Blueprint.SeamCount))
			: LOCTEXT("BlueprintNotSuitable", "Not what this customer needs.");
		Add(Box, Button(Blueprint.DisplayName, Detail, [Weak, Id]() { if (Weak.IsValid()) { Weak->ChooseBlueprint(Id); } }, bAllowed));
	}
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildFabricChoice()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	TSharedRef<SVerticalBox> Box = Stack({
		Title(LOCTEXT("FabricTitle", "Which cloth?")),
		Body(LOCTEXT("FabricBody", "Heavy cloth stops blows but tires the wearer and holds less wax. Light cloth carries more inscription."), Muted)
	});
	for (const FCraftFabric& Fabric : UCraftSettings::Get()->Fabrics)
	{
		const FName Id = Fabric.Id;
		const FText Detail = FText::Format(LOCTEXT("FabricDetail", "{0}\nWeight ×{1} · durability {2} · wax capacity {3} · bolt {4}×{5}"),
			Fabric.Description, Number(Fabric.Weight), Number(Fabric.Durability, 0), Percent(Fabric.MagicCapacity),
			FText::AsNumber(Fabric.SheetWidth), FText::AsNumber(Fabric.SheetHeight));
		Add(Box, Button(Fabric.DisplayName, Detail, [Weak, Id]() { if (Weak.IsValid()) { Weak->ChooseFabric(Id); } }));
	}
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildArrange()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	auto LayoutText = [Weak]()
	{
		FCraftGarmentBlueprint Blueprint;
		FCraftFabric Fabric;
		if (!Weak.IsValid() || !Weak->GetChosenBlueprint(Blueprint) || !Weak->GetChosenFabric(Fabric))
		{
			return FText::GetEmpty();
		}
		const TArray<FCraftPiecePlacement> Placements = Weak->GetPlacements();
		int32 Placed = 0;
		for (const FCraftPiecePlacement& Placement : Placements)
		{
			Placed += Placement.bPlaced ? 1 : 0;
		}
		float Cross = 0.0f;
		float Waste = 0.0f;
		UCraftRules::EvaluateLayout(Fabric, Blueprint, Placements, Cross, Waste);
		return FText::Format(LOCTEXT("LayoutStats", "{0} of {1} pieces placed · {2} across the grain"),
			FText::AsNumber(Placed), FText::AsNumber(Placements.Num()), Percent(Cross));
	};

	return Stack({
		Title(LOCTEXT("ArrangeTitle", "Lay out the pattern")),
		Body(LOCTEXT("ArrangeBody",
			"Drag each piece onto the cloth. Right-click (or R while dragging) turns it across the grain: "
			"it drapes and covers better, but the weave is weaker that way. Drop a piece outside the cloth to take it back."), Muted),
		SNew(SCraftArrangeBoard).Loop(Loop),
		LiveText(TAttribute<FText>::CreateLambda(LayoutText), CraftStyle::TextColor, BodySize, true),
		Button(LOCTEXT("CommitLayout", "Pin the pattern"), Hours(UCraftSettings::Get()->CostArrange),
			[Weak]() { if (Weak.IsValid()) { Weak->CommitArrangement(); } },
			TAttribute<bool>::CreateLambda([Weak]() { return Weak.IsValid() && Weak->CanCommitArrangement(); }))
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildCut()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	auto CutText = [Weak]()
	{
		FCraftGarmentBlueprint Blueprint;
		FCraftFabric Fabric;
		if (!Weak.IsValid() || !Weak->GetChosenBlueprint(Blueprint) || !Weak->GetChosenFabric(Fabric))
		{
			return FText::GetEmpty();
		}
		const float Length = Weak->GetCutLength();
		const float Weight = Blueprint.BaseWeight * Fabric.Weight * UCraftRules::CutWeightMultiplier(Length);
		return FText::Format(LOCTEXT("CutStats", "Length ×{0} · coverage ×{1} · weight {2} of {3}{4}"),
			Number(Length, 2), Number(UCraftRules::CutCoverageMultiplier(Length), 2), Number(Weight, 2), Number(Blueprint.MaxWeight, 2),
			Weight > Blueprint.MaxWeight ? LOCTEXT("TooHeavy", "\nToo heavy: the wearer will tire.") : FText::GetEmpty());
	};
	auto CutColor = [Weak]()
	{
		FCraftGarmentBlueprint Blueprint;
		FCraftFabric Fabric;
		if (Weak.IsValid() && Weak->GetChosenBlueprint(Blueprint) && Weak->GetChosenFabric(Fabric)
			&& Blueprint.BaseWeight * Fabric.Weight * UCraftRules::CutWeightMultiplier(Weak->GetCutLength()) > Blueprint.MaxWeight)
		{
			return FSlateColor(Warning);
		}
		return FSlateColor(CraftStyle::TextColor);
	};

	return Stack({
		Title(LOCTEXT("CutTitle", "Cut the panels")),
		Body(LOCTEXT("CutBody", "Drag the hem up or down. A longer panel covers more of the body, but it weighs more."), Muted),
		SNew(SCraftCutPanel).Loop(Loop),
		SNew(STextBlock).Text_Lambda(CutText).ColorAndOpacity_Lambda(CutColor).Font(Font(BodySize, true)).AutoWrapText(true),
		Button(LOCTEXT("CommitCut", "Cut along the line"), Hours(UCraftSettings::Get()->CostCut),
			[Weak]() { if (Weak.IsValid()) { Weak->CommitCut(); } })
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildCanting()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const bool bLive = Loop->HasLiveCanvas();

	TSharedRef<SVerticalBox> Box = Stack({
		Title(LOCTEXT("CantingTitle", "Canting")),
		Body(LOCTEXT("CantingBody",
			"Draw on the cloth with the mouse. Closed outlines keep the dye out, so the ward holds. "
			"Fill the cloth for strength, but past its capacity the wax bleeds and the ward turns unstable."), Muted)
	});

	if (bLive)
	{
		auto Live = [Weak]()
		{
			FCraftInscription Inscription;
			if (Weak.IsValid())
			{
				Weak->GetLiveInscription(Inscription);
			}
			return Inscription;
		};
		Add(Box, Body(LOCTEXT("Capacity", "Wax on the cloth (red = over capacity)"), Muted), 8.0f);
		Add(Box, SNew(SBox).HeightOverride(18.0f)
			[
				SNew(SProgressBar)
				.Percent_Lambda([Live]() { return TOptional<float>(FMath::Min(Live().CapacityFill, 1.0f)); })
				.FillColorAndOpacity_Lambda([Live]() { return FSlateColor(Live().CapacityFill > 1.0f ? Warning : Accent); })
			]);
		Add(Box, LiveText(TAttribute<FText>::CreateLambda([Live]()
			{
				const FCraftInscription I = Live();
				return FText::Format(LOCTEXT("LiveStats", "{0} of capacity · {1} strokes · {2} closed shapes\nStrength {3} · Stability {4}"),
					Percent(I.CapacityFill), FText::AsNumber(I.StrokeCount), FText::AsNumber(I.EnclosedShapes),
					Percent(I.Strength), Percent(I.Stability));
			}), CraftStyle::TextColor, BodySize, true));
		Add(Box, Button(LOCTEXT("Undo", "Undo last stroke"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->UndoCantingStroke(); } }), 8.0f);
	}
	else
	{
		Add(Box, Body(LOCTEXT("MeasuredLater", "The wax is measured when you finish."), Muted));
	}

	Add(Box, Button(LOCTEXT("Clear", "Scrape it all off"), LOCTEXT("ClearDetail", "Start the drawing again."),
		[Weak]() { if (Weak.IsValid()) { Weak->ClearCanting(); } }));
	Add(Box, Button(LOCTEXT("FinishCanting", "Finish canting"),
		FText::Format(LOCTEXT("FinishCantingDetail", "Let the wax set. {0}"), Hours(UCraftSettings::Get()->CostCanting)),
		[Weak]() { if (Weak.IsValid()) { Weak->FinishCanting(); } }), 8.0f);
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildDye()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const UCraftSettings* Settings = UCraftSettings::Get();
	const int32 Phase = Loop->GetStagePhase();

	if (Phase >= 3)
	{
		return Stack({ Title(LOCTEXT("RevealTitle", "Revealing…")), Body(LOCTEXT("RevealBody", "The wax is boiled away (nglorod). Where it sealed the cloth, your pattern shows."), Muted) });
	}

	if (Phase == 0)
	{
		const bool bIndigo = Loop->IsIndigoAvailable();
		return Stack({
			Title(LOCTEXT("DyeTitle", "Choose the dye vat")),
			Body(LOCTEXT("DyeBody", "Wherever the wax sealed the cloth, the dye can't reach. That is your pattern."), Muted),
			Button(LOCTEXT("Soga", "Soga brown"), LOCTEXT("SogaDetail", "The workshop's own bark dye."),
				[Weak]() { if (Weak.IsValid()) { Weak->ChooseDye(false); } }),
			Button(LOCTEXT("Indigo", "Indigo"),
				bIndigo
					? FText::Format(LOCTEXT("IndigoDetail", "Bu Sari's gift. Sets the resist firmly: +{0} stability."), Percent(Settings->IndigoStabilityBonus))
					: FText::Format(LOCTEXT("IndigoLocked", "None in the workshop yet. A neighbour may bring some around hour {0}."), FText::AsNumber(Settings->IndigoDeliveryHour)),
				[Weak]() { if (Weak.IsValid()) { Weak->ChooseDye(true); } }, bIndigo)
		});
	}

	// Phases 1-2: hold to dip.
	TArray<FCraftGaugeZone> Zones;
	Zones.Add({ 0.0f, Settings->DipGoodMin, CraftStyle::SRGB(96, 90, 80) });
	Zones.Add({ Settings->DipGoodMin, Settings->DipGoodMax, CraftStyle::SRGB(52, 150, 64) });
	Zones.Add({ Settings->DipGoodMax, 1.0f, CraftStyle::SRGB(176, 56, 44) });

	TSharedRef<SButton> DipButton = MakeButton(LOCTEXT("Dip", "Hold to dip the cloth"), LOCTEXT("DipDetail", "Release to lift it out."), true,
		FSimpleDelegate::CreateLambda([Weak]() { if (Weak.IsValid()) { Weak->StartDip(); } }),
		FSimpleDelegate::CreateLambda([Weak]() { if (Weak.IsValid()) { Weak->StopDip(); } }));

	auto DepthWord = [Weak, Settings]()
	{
		const float Depth = Weak.IsValid() ? Weak->GetDipDepth() : 0.0f;
		if (Depth < Settings->DipGoodMin) return LOCTEXT("DepthPale", "Pale");
		if (Depth <= Settings->DipGoodMax) return LOCTEXT("DepthGood", "Deep and even: lift now");
		return LOCTEXT("DepthCrack", "Too long: the wax is cracking");
	};

	return Stack({
		Title(LOCTEXT("DipTitle", "Dip the cloth")),
		Body(FText::Format(LOCTEXT("DipBody", "Hold the button to lower the cloth into the {0} vat. Lift it out while the marker is in the green band. {1}"),
			Loop->GetGarment().bIndigoDye ? LOCTEXT("VatIndigo", "indigo") : LOCTEXT("VatSoga", "soga"), Hours(Settings->CostDye)), Muted),
		SNew(SCraftGauge)
			.Value_Lambda([Weak]() { return Weak.IsValid() ? Weak->GetDipDepth() : 0.0f; })
			.Zones(Zones)
			.bFill(true),
		LiveText(TAttribute<FText>::CreateLambda(DepthWord), CraftStyle::TextColor, BodySize, true),
		DipButton
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildStitch()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const UCraftSettings* Settings = UCraftSettings::Get();
	const int32 Phase = Loop->GetStagePhase();

	if (Phase >= 2)
	{
		return Stack({ Title(LOCTEXT("AssembleTitle", "Assembling…")), Body(LOCTEXT("AssembleBody", "The panels come together along the blueprint's seams."), Muted) });
	}
	if (Phase == 0)
	{
		FCraftGarmentBlueprint Blueprint;
		Loop->GetChosenBlueprint(Blueprint);
		return Stack({
			Title(LOCTEXT("StitchTitle", "Stitch")),
			Body(FText::Format(LOCTEXT("StitchBody",
				"Join the panels along {0} seams, {1} stitches each. Press when the needle is in the middle of the bar: "
				"neat stitches make the garment tougher. {2}"),
				FText::AsNumber(Blueprint.SeamCount), FText::AsNumber(Settings->StitchesPerSeam), Hours(Settings->CostStitch)), Muted),
			Button(LOCTEXT("Thread", "Thread the needle"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->StartStitching(); } })
		});
	}

	const float Tolerance = Settings->StitchTolerance;
	TArray<FCraftGaugeZone> Zones;
	Zones.Add({ 0.5f - Tolerance, 0.5f + Tolerance, CraftStyle::SRGB(130, 116, 60) });
	Zones.Add({ 0.5f - Tolerance * 0.4f, 0.5f + Tolerance * 0.4f, CraftStyle::SRGB(52, 150, 64) });

	TSharedRef<SButton> StitchButton = MakeButton(LOCTEXT("StitchNow", "Stitch!"), LOCTEXT("StitchNowDetail", "Press when the needle is in the green."), true);
	StitchButton->SetOnClicked(FOnClicked::CreateLambda([Weak]() { if (Weak.IsValid()) { Weak->PressStitch(); } return FReply::Handled(); }));
	StitchButton->SetClickMethod(EButtonClickMethod::MouseDown);

	auto Progress = [Weak]()
	{
		if (!Weak.IsValid())
		{
			return FText::GetEmpty();
		}
		const float Last = Weak->GetLastStitchAccuracy();
		return FText::Format(LOCTEXT("StitchProgress", "Stitch {0} of {1}{2}"),
			FText::AsNumber(FMath::Min(Weak->GetStitchesDone() + 1, Weak->GetStitchesTotal())), FText::AsNumber(Weak->GetStitchesTotal()),
			Last < 0.0f ? FText::GetEmpty() : FText::Format(LOCTEXT("LastStitch", "   ·   last one: {0}"), QualityWord(Last)));
	};

	return Stack({
		Title(LOCTEXT("StitchingTitle", "Stitching")),
		SNew(SCraftGauge)
			.Value_Lambda([Weak]() { return Weak.IsValid() ? Weak->GetStitchMarker() : 0.0f; })
			.Zones(Zones)
			.Height(40.0f),
		LiveText(TAttribute<FText>::CreateLambda(Progress), CraftStyle::TextColor, BodySize, true),
		StitchButton
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildInspect()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const UCraftSettings* Settings = UCraftSettings::Get();
	const FCraftGarment G = Loop->GetGarment();
	const FCraftGarmentBlueprint* Blueprint = Settings->FindBlueprint(G.BlueprintId);
	const FCraftFabric* Fabric = Settings->FindFabric(G.FabricId);
	const bool bHeavy = Blueprint && G.Weight > Blueprint->MaxWeight;

	TSharedRef<SVerticalBox> Box = Stack({
		Title(FText::Format(LOCTEXT("InspectTitle", "{0} in {1}"),
			Blueprint ? Blueprint->DisplayName : FText::GetEmpty(), Fabric ? Fabric->DisplayName : FText::GetEmpty())),
		Body(FText::Format(LOCTEXT("InspectChoices", "Hem ×{0} · {1} across the grain · {2} dye, {3} dip · {4} seams"),
			Number(G.CutLength, 2), Percent(G.CrossGrainFraction),
			G.bIndigoDye ? LOCTEXT("IndigoDye", "indigo") : LOCTEXT("SogaDye", "soga"), QualityWord(G.DyeQuality), QualityWord(G.SeamQuality)), Muted),
		Stat(LOCTEXT("Weight", "Weight"), FText::Format(LOCTEXT("WeightValue", "{0} / {1}"), Number(G.Weight, 2), Number(Blueprint ? Blueprint->MaxWeight : 0.0f, 2)),
			bHeavy ? Warning : CraftStyle::TextColor),
		Stat(LOCTEXT("Coverage", "Coverage"), Percent(G.Coverage)),
		Stat(LOCTEXT("Durability", "Durability"), Number(G.Durability, 0)),
		Stat(LOCTEXT("Effect", "Inscription"), EffectName(G.Effect)),
		Stat(LOCTEXT("Shapes", "Closed shapes"), FText::AsNumber(G.Inscription.EnclosedShapes)),
		Stat(LOCTEXT("Strength", "Strength"), Percent(G.Inscription.Strength)),
		Stat(LOCTEXT("Stability", "Stability"), Percent(G.Inscription.Stability), G.Inscription.CapacityFill > 1.0f ? Warning : CraftStyle::TextColor),
		Stat(LOCTEXT("Protection", "Protection"), Number(G.Protection, 0), Accent)
	});
	if (bHeavy)
	{
		Add(Box, Body(LOCTEXT("HeavyWarning", "It's heavy for this blueprint. The wearer will tire."), Warning));
	}
	if (G.Inscription.CapacityFill > 1.0f)
	{
		Add(Box, Body(LOCTEXT("OverfillWarning", "Too much wax for this cloth; the resist bled and the ward is unstable."), Warning));
	}
	if (G.Inscription.WaxCoverage <= 0.0f)
	{
		Add(Box, Body(LOCTEXT("NoInscription", "There is no inscription on it. It is only cloth."), Warning));
	}
	Add(Box, Button(LOCTEXT("LooksGood", "It's ready"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->ConfirmInspection(); } }), 8.0f);
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildDeliver()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	FCraftCommission Commission;
	Loop->GetCurrentCommission(Commission);
	return Stack({
		Title(LOCTEXT("DeliverTitle", "Deliver")),
		Body(FText::Format(LOCTEXT("DeliverBody", "{0} tries it on, nods, and pays the deposit."), Commission.CustomerName), Muted),
		Button(LOCTEXT("HandOver", "Hand it over and see them off"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->DeliverGarment(); } })
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildExpedition()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const FCraftExpeditionResult Result = Loop->GetExpedition();
	const int32 Shown = FMath::Min(Loop->GetRevealedEventCount(), Result.Events.Num());
	const bool bDone = Shown >= Result.Events.Num();
	const float HP = Shown > 0 ? Result.Events[Shown - 1].HPAfter : 100.0f;

	TSharedRef<SVerticalBox> Box = Stack({
		Title(LOCTEXT("ExpeditionTitle", "The expedition")),
		Stat(LOCTEXT("Health", "Health"), FText::Format(LOCTEXT("HealthValue", "{0} / 100"), Number(HP, 0)), HP >= 60.0f ? Good : Warning),
		SNew(SBox).HeightOverride(16.0f)
		[
			SNew(SProgressBar).Percent(HP / 100.0f).FillColorAndOpacity(HP >= 60.0f ? Good : Warning)
		]
	});
	for (int32 Index = 0; Index < Shown; ++Index)
	{
		const FCraftExpeditionEvent& Event = Result.Events[Index];
		const FLinearColor Color = Event.Type == ECraftEventType::InscriptionActivates ? Accent
			: (Event.Damage > 0.0f ? Warning : CraftStyle::TextColor);
		Add(Box, Body(Event.Text, Color));
	}
	if (bDone)
	{
		Add(Box, Button(LOCTEXT("ContinueExpedition", "Wait for their return"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->FinishExpedition(); } }), 8.0f);
	}
	else
	{
		Add(Box, Button(LOCTEXT("Skip", "Skip ahead"), LOCTEXT("SkipDetail", "The outcome is already decided."),
			[Weak]() { if (Weak.IsValid()) { Weak->SkipExpedition(); } }), 8.0f);
	}
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildReturn()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	FCraftCommission Commission;
	Loop->GetCurrentCommission(Commission);
	const ECraftExpeditionOutcome Outcome = Loop->GetExpedition().Outcome;
	const bool bLast = Loop->GetCommissionIndex() + 1 >= UCraftSettings::Get()->Commissions.Num();

	FText Story;
	FText Heading;
	switch (Outcome)
	{
	case ECraftExpeditionOutcome::Success:
		Heading = LOCTEXT("ReturnSuccess", "They came home");
		Story = Commission.ReturnSuccessText;
		break;
	case ECraftExpeditionOutcome::Injured:
		Heading = LOCTEXT("ReturnInjured", "They came home hurt");
		Story = Commission.ReturnInjuredText;
		break;
	default:
		Heading = LOCTEXT("ReturnRetreat", "They turned back");
		Story = Commission.ReturnRetreatText;
		break;
	}
	return Stack({
		Title(Heading),
		Body(Story),
		Button(bLast ? LOCTEXT("EndDay", "Close the workshop for the day") : LOCTEXT("NextCustomer", "Call in the next customer"),
			FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->NextCommission(); } })
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildFinished()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	TArray<FString> Motifs;
	for (const FName& Motif : Loop->GetKnownMotifs())
	{
		Motifs.Add(Motif.ToString());
	}
	return Stack({
		Title(LOCTEXT("FinishedTitle", "The day's work is done")),
		Stat(LOCTEXT("Earned", "Gold earned"), FText::AsNumber(Loop->GetGold()), Accent),
		Stat(LOCTEXT("Hours", "Hours worked"), FText::AsNumber(Loop->GetHour())),
		Stat(LOCTEXT("Motifs", "Motifs learned"), Motifs.Num() > 0 ? FText::FromString(FString::Join(Motifs, TEXT(", "))) : LOCTEXT("NoMotifs", "none")),
		Button(LOCTEXT("Restart", "Start the day again"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->RestartLoop(); } })
	});
}

#undef LOCTEXT_NAMESPACE
