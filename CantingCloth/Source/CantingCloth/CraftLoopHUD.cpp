#include "CraftLoopHUD.h"

#include "CraftLoopSubsystem.h"
#include "CraftRules.h"
#include "CraftSettings.h"
#include "Styling/CoreStyle.h"
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
	constexpr float PanelWidth = 430.0f;
	constexpr double NoticeSeconds = 7.0;

	const FLinearColor TextColor(0.93f, 0.9f, 0.84f);
	const FLinearColor MutedColor(0.68f, 0.65f, 0.6f);
	const FLinearColor AccentColor(0.95f, 0.72f, 0.35f);
	const FLinearColor WarningColor(0.95f, 0.45f, 0.35f);

	FSlateFontInfo Font(int32 Size, bool bBold = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
	}

	TSharedRef<SWidget> Title(const FText& Text)
	{
		return SNew(STextBlock).Text(Text).Font(Font(18, true)).ColorAndOpacity(AccentColor).AutoWrapText(true);
	}

	TSharedRef<SWidget> Body(const FText& Text, const FLinearColor& Color = TextColor)
	{
		return SNew(STextBlock).Text(Text).Font(Font(12)).ColorAndOpacity(Color).AutoWrapText(true);
	}

	TSharedRef<SWidget> Stat(const FText& Label, const FText& Value, const FLinearColor& Color = TextColor)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(Label).Font(Font(12)).ColorAndOpacity(MutedColor)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text(Value).Font(Font(12, true)).ColorAndOpacity(Color)
			];
	}

	/** A choice button: a bold label and an optional line explaining the trade-off. */
	TSharedRef<SWidget> Button(const FText& Label, const FText& Detail, TFunction<void()> OnClick, bool bEnabled = true)
	{
		TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(Font(13, true)).ColorAndOpacity(FLinearColor(0.08f, 0.06f, 0.04f))
			];
		if (!Detail.IsEmpty())
		{
			Content->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Detail).Font(Font(11)).ColorAndOpacity(FLinearColor(0.2f, 0.17f, 0.13f)).AutoWrapText(true)
			];
		}
		return SNew(SButton)
			.IsEnabled(bEnabled)
			.ButtonColorAndOpacity(FLinearColor(0.93f, 0.84f, 0.68f))
			.ContentPadding(FMargin(10.0f, 6.0f))
			.OnClicked_Lambda([OnClick]()
			{
				OnClick();
				return FReply::Handled();
			})
			[
				Content
			];
	}

	FText EffectName(ECraftEffectFamily Effect)
	{
		switch (Effect)
		{
		case ECraftEffectFamily::WardPhysical: return LOCTEXT("WardPhysical", "Ward against blows");
		default: return LOCTEXT("WardPoison", "Ward against miasma");
		}
	}

	FText CutName(ECraftCutStyle Cut)
	{
		switch (Cut)
		{
		case ECraftCutStyle::Short: return LOCTEXT("CutShort", "Short");
		case ECraftCutStyle::Long: return LOCTEXT("CutLong", "Long");
		default: return LOCTEXT("CutStandard", "Standard");
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

	/** Stacks widgets vertically with even spacing. */
	TSharedRef<SVerticalBox> Stack(std::initializer_list<TSharedRef<SWidget>> Widgets)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		for (const TSharedRef<SWidget>& Widget : Widgets)
		{
			Box->AddSlot().AutoHeight().Padding(0.0f, 4.0f)[Widget];
		}
		return Box;
	}

	void Add(const TSharedRef<SVerticalBox>& Box, const TSharedRef<SWidget>& Widget, float Top = 4.0f)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, Top, 0.0f, 4.0f)[Widget];
	}
}

using namespace CraftHUD;

SCraftLoopHUD::SCraftLoopHUD()
	: PanelBrush(FLinearColor(0.035f, 0.03f, 0.03f, 0.86f))
	, NoticeBrush(FLinearColor(0.12f, 0.08f, 0.04f, 0.92f))
{
}

void SCraftLoopHUD::Construct(const FArguments& InArgs)
{
	Loop = InArgs._Loop;

	ChildSlot
	[
		SNew(SOverlay)
		// Header: stage, clock, purse.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(16.0f)
		[
			SNew(SBorder)
			.BorderImage(&PanelBrush)
			.Padding(FMargin(12.0f, 8.0f))
			[
				SNew(STextBlock)
				.Text(this, &SCraftLoopHUD::GetHeaderText)
				.Font(Font(13, true))
				.ColorAndOpacity(TextColor)
			]
		]
		// Stage panel on the right, leaving the cloth visible.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(16.0f)
		[
			SNew(SBox)
			.WidthOverride(PanelWidth)
			.MaxDesiredHeight(760.0f)
			[
				SNew(SBorder)
				.BorderImage(&PanelBrush)
				.Padding(16.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(PanelBox, SBox)
					]
				]
			]
		]
		// Notices from scheduled events.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(16.0f, 16.0f, 16.0f, 40.0f)
		[
			SNew(SBox)
			.MaxDesiredWidth(640.0f)
			.Visibility(this, &SCraftLoopHUD::GetNoticeVisibility)
			[
				SNew(SBorder)
				.BorderImage(&NoticeBrush)
				.Padding(FMargin(14.0f, 10.0f))
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return NoticeText; })
					.Font(Font(13))
					.ColorAndOpacity(AccentColor)
					.AutoWrapText(true)
				]
			]
		]
	];

	Refresh();
}

void SCraftLoopHUD::Refresh()
{
	if (PanelBox.IsValid())
	{
		PanelBox->SetContent(BuildPanel());
	}
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
	return FText::Format(LOCTEXT("Header", "{0}   ·   Hour {1}   ·   {2} gold"),
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
		Stat(LOCTEXT("Needs", "Needs"), EffectName(Commission.RequiredEffect)),
		Stat(LOCTEXT("Danger", "Expected danger"), FText::Format(LOCTEXT("DangerValue", "{0}, {1}"), Commission.HazardName, Commission.EnemyName)),
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
		Body(LOCTEXT("BlueprintBody", "Blueprints fix the silhouette and the seams. Pick one that suits the request."), MutedColor)
	});
	for (const FCraftGarmentBlueprint& Blueprint : UCraftSettings::Get()->Blueprints)
	{
		const bool bAllowed = Commission.AllowedBlueprints.Num() == 0 || Commission.AllowedBlueprints.Contains(Blueprint.Id);
		const FName Id = Blueprint.Id;
		const FText Detail = bAllowed
			? FText::Format(LOCTEXT("BlueprintDetail", "{0}\nWeight {1} · wearable up to {2} · coverage {3}"),
				Blueprint.Description, Number(Blueprint.BaseWeight), Number(Blueprint.MaxWeight), Percent(Blueprint.BaseCoverage))
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
		Body(LOCTEXT("FabricBody", "Heavy cloth stops blows but tires the wearer and holds less wax. Light cloth carries more inscription."), MutedColor)
	});
	for (const FCraftFabric& Fabric : UCraftSettings::Get()->Fabrics)
	{
		const FName Id = Fabric.Id;
		const FText Detail = FText::Format(LOCTEXT("FabricDetail", "{0}\nWeight ×{1} · durability {2} · wax capacity {3}"),
			Fabric.Description, Number(Fabric.Weight), Number(Fabric.Durability, 0), Percent(Fabric.MagicCapacity));
		Add(Box, Button(Fabric.DisplayName, Detail, [Weak, Id]() { if (Weak.IsValid()) { Weak->ChooseFabric(Id); } }));
	}
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildArrange()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const FText Cost = FText::Format(LOCTEXT("CostHours", "Takes {0} h."), FText::AsNumber(UCraftSettings::Get()->CostArrange));
	return Stack({
		Title(LOCTEXT("ArrangeTitle", "Lay out the pattern")),
		Body(LOCTEXT("ArrangeBody", "Place the blueprint pieces on the cloth. Along the grain the weave stays strong; on the bias the panels drape and cover more."), MutedColor),
		Button(LOCTEXT("Grain", "Along the grain"), FText::Format(LOCTEXT("GrainDetail", "Full durability. {0}"), Cost),
			[Weak]() { if (Weak.IsValid()) { Weak->CommitArrangement(false); } }),
		Button(LOCTEXT("Bias", "On the bias"), FText::Format(LOCTEXT("BiasDetail", "+5% coverage, −10% durability. {0}"), Cost),
			[Weak]() { if (Weak.IsValid()) { Weak->CommitArrangement(true); } })
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildCut()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const UCraftSettings* Settings = UCraftSettings::Get();
	const FCraftGarment Garment = Loop->GetGarment();
	const FCraftGarmentBlueprint* Blueprint = Settings->FindBlueprint(Garment.BlueprintId);
	const FCraftFabric* Fabric = Settings->FindFabric(Garment.FabricId);

	TSharedRef<SVerticalBox> Box = Stack({
		Title(LOCTEXT("CutTitle", "Cut the panels")),
		Body(FText::Format(LOCTEXT("CutBody", "Longer panels cover more of the body but weigh more. Takes {0} h."), FText::AsNumber(Settings->CostCut)), MutedColor)
	});
	for (const ECraftCutStyle Cut : { ECraftCutStyle::Short, ECraftCutStyle::Standard, ECraftCutStyle::Long })
	{
		FText Detail;
		if (Blueprint && Fabric)
		{
			const float Weight = Blueprint->BaseWeight * Fabric->Weight * UCraftRules::CutWeightMultiplier(Cut);
			Detail = FText::Format(LOCTEXT("CutDetail", "Coverage ×{0} · weight {1}{2}"),
				Number(UCraftRules::CutCoverageMultiplier(Cut), 2), Number(Weight, 2),
				Weight > Blueprint->MaxWeight ? LOCTEXT("TooHeavy", "  (too heavy, will slow them)") : FText::GetEmpty());
		}
		Add(Box, Button(CutName(Cut), Detail, [Weak, Cut]() { if (Weak.IsValid()) { Weak->CommitCut(Cut); } }));
	}
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildCanting()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const bool bLive = Loop->HasLiveCanvas();

	TSharedRef<SVerticalBox> Box = Stack({
		Title(LOCTEXT("CantingTitle", "Canting")),
		Body(LOCTEXT("CantingBody",
			"Draw on the cloth with the mouse. Closed outlines keep the dye out, so the ward holds. "
			"Fill the cloth for strength, but past its capacity the wax bleeds and the ward turns unstable."), MutedColor)
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
		Add(Box, SNew(STextBlock).Text(LOCTEXT("Capacity", "Wax on the cloth")).Font(Font(12)).ColorAndOpacity(MutedColor), 8.0f);
		Add(Box, SNew(SBox).HeightOverride(14.0f)
			[
				SNew(SProgressBar)
				.Percent_Lambda([Live]() { return TOptional<float>(FMath::Min(Live().CapacityFill, 1.0f)); })
				.FillColorAndOpacity_Lambda([Live]() { return FSlateColor(Live().CapacityFill > 1.0f ? WarningColor : AccentColor); })
			]);
		Add(Box, SNew(STextBlock)
			.Font(Font(12))
			.ColorAndOpacity(TextColor)
			.AutoWrapText(true)
			.Text_Lambda([Live]()
			{
				const FCraftInscription I = Live();
				return FText::Format(LOCTEXT("LiveStats", "{0} of capacity · {1} strokes · {2} closed shapes\nStrength {3} · Stability {4}"),
					Percent(I.CapacityFill), FText::AsNumber(I.StrokeCount), FText::AsNumber(I.EnclosedShapes),
					Percent(I.Strength), Percent(I.Stability));
			}));
		Add(Box, Button(LOCTEXT("Undo", "Undo last stroke"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->UndoCantingStroke(); } }), 8.0f);
	}
	else
	{
		Add(Box, Body(LOCTEXT("MeasuredLater", "The wax is measured when you finish."), MutedColor));
	}

	Add(Box, Button(LOCTEXT("Clear", "Scrape it all off"), LOCTEXT("ClearDetail", "Start the drawing again."),
		[Weak]() { if (Weak.IsValid()) { Weak->ClearCanting(); } }));
	Add(Box, Button(LOCTEXT("FinishCanting", "Finish canting"),
		FText::Format(LOCTEXT("FinishCantingDetail", "Let the wax set. Takes {0} h."), FText::AsNumber(UCraftSettings::Get()->CostCanting)),
		[Weak]() { if (Weak.IsValid()) { Weak->FinishCanting(); } }), 8.0f);
	return Box;
}

TSharedRef<SWidget> SCraftLoopHUD::BuildDye()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	const UCraftSettings* Settings = UCraftSettings::Get();
	if (Loop->IsBusy())
	{
		return Stack({ Title(LOCTEXT("DyeingTitle", "Dyeing…")), Body(LOCTEXT("DyeingBody", "The cloth soaks, then the wax is boiled away (nglorod)."), MutedColor) });
	}
	const bool bIndigo = Loop->IsIndigoAvailable();
	const FText Cost = FText::Format(LOCTEXT("DyeCost", "Takes {0} h."), FText::AsNumber(Settings->CostDye));
	return Stack({
		Title(LOCTEXT("DyeTitle", "Dye & reveal")),
		Body(LOCTEXT("DyeBody", "Wherever the wax sealed the cloth, the dye can't reach. That is your pattern."), MutedColor),
		Button(LOCTEXT("Soga", "Soga brown"), FText::Format(LOCTEXT("SogaDetail", "The workshop's own bark dye. {0}"), Cost),
			[Weak]() { if (Weak.IsValid()) { Weak->ApplyDye(false); } }),
		Button(LOCTEXT("Indigo", "Indigo"),
			bIndigo
				? FText::Format(LOCTEXT("IndigoDetail", "Bu Sari's gift. Sets the resist firmly: +{0} stability. {1}"), Percent(Settings->IndigoStabilityBonus), Cost)
				: FText::Format(LOCTEXT("IndigoLocked", "None in the workshop yet. A neighbour may bring some around hour {0}."), FText::AsNumber(Settings->IndigoDeliveryHour)),
			[Weak]() { if (Weak.IsValid()) { Weak->ApplyDye(true); } }, bIndigo)
	});
}

TSharedRef<SWidget> SCraftLoopHUD::BuildStitch()
{
	TWeakObjectPtr<UCraftLoopSubsystem> Weak = Loop;
	if (Loop->IsBusy())
	{
		return Stack({ Title(LOCTEXT("StitchingTitle", "Stitching…")), Body(LOCTEXT("StitchingBody", "Joining the panels along the blueprint's seams."), MutedColor) });
	}
	return Stack({
		Title(LOCTEXT("StitchTitle", "Stitch")),
		Body(LOCTEXT("StitchBody", "Join the dyed panels along the blueprint's seams to make the garment."), MutedColor),
		Button(LOCTEXT("StitchButton", "Stitch the garment"),
			FText::Format(LOCTEXT("StitchDetail", "Takes {0} h."), FText::AsNumber(UCraftSettings::Get()->CostStitch)),
			[Weak]() { if (Weak.IsValid()) { Weak->Stitch(); } })
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
		Body(FText::Format(LOCTEXT("InspectChoices", "{0} cut · {1} · {2} dye"),
			CutName(G.CutStyle),
			G.bOnBias ? LOCTEXT("OnBias", "on the bias") : LOCTEXT("OnGrain", "along the grain"),
			G.bIndigoDye ? LOCTEXT("IndigoDye", "indigo") : LOCTEXT("SogaDye", "soga")), MutedColor),
		Stat(LOCTEXT("Weight", "Weight"), FText::Format(LOCTEXT("WeightValue", "{0} / {1}"), Number(G.Weight, 2), Number(Blueprint ? Blueprint->MaxWeight : 0.0f, 2)),
			bHeavy ? WarningColor : TextColor),
		Stat(LOCTEXT("Coverage", "Coverage"), Percent(G.Coverage)),
		Stat(LOCTEXT("Durability", "Durability"), Number(G.Durability, 0)),
		Stat(LOCTEXT("Effect", "Inscription"), EffectName(G.Effect)),
		Stat(LOCTEXT("Shapes", "Closed shapes"), FText::AsNumber(G.Inscription.EnclosedShapes)),
		Stat(LOCTEXT("Strength", "Strength"), Percent(G.Inscription.Strength)),
		Stat(LOCTEXT("Stability", "Stability"), Percent(G.Inscription.Stability), G.Inscription.CapacityFill > 1.0f ? WarningColor : TextColor),
		Stat(LOCTEXT("Protection", "Protection"), Number(G.Protection, 0), AccentColor)
	});
	if (bHeavy)
	{
		Add(Box, Body(LOCTEXT("HeavyWarning", "It's heavy for this blueprint. The wearer will tire."), WarningColor));
	}
	if (G.Inscription.CapacityFill > 1.0f)
	{
		Add(Box, Body(LOCTEXT("OverfillWarning", "Too much wax for this cloth; the resist bled and the ward is unstable."), WarningColor));
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
		Body(FText::Format(LOCTEXT("DeliverBody", "{0} tries it on, nods, and pays the deposit."), Commission.CustomerName), MutedColor),
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
		SNew(SBox).HeightOverride(12.0f)
		[
			SNew(SProgressBar).Percent(HP / 100.0f).FillColorAndOpacity(HP > 60.0f ? AccentColor : WarningColor)
		]
	});
	for (int32 Index = 0; Index < Shown; ++Index)
	{
		const FCraftExpeditionEvent& Event = Result.Events[Index];
		const FLinearColor Color = Event.Type == ECraftEventType::InscriptionActivates ? AccentColor
			: (Event.Damage > 0.0f ? WarningColor : TextColor);
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
		Stat(LOCTEXT("Earned", "Gold earned"), FText::AsNumber(Loop->GetGold())),
		Stat(LOCTEXT("Hours", "Hours worked"), FText::AsNumber(Loop->GetHour())),
		Stat(LOCTEXT("Motifs", "Motifs learned"), Motifs.Num() > 0 ? FText::FromString(FString::Join(Motifs, TEXT(", "))) : LOCTEXT("NoMotifs", "none")),
		Button(LOCTEXT("Restart", "Start the day again"), FText::GetEmpty(), [Weak]() { if (Weak.IsValid()) { Weak->RestartLoop(); } })
	});
}

#undef LOCTEXT_NAMESPACE
