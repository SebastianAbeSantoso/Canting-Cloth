#include "CraftRules.h"

#include "CraftSettings.h"

#define LOCTEXT_NAMESPACE "CraftRules"

namespace
{
	constexpr float StartingHP = 100.0f;
	constexpr float SuccessHP = 60.0f;
	constexpr float RetreatHP = 25.0f;
	constexpr float SlowedDamage = 10.0f;
	constexpr float MaxDurabilityBlock = 0.8f;
	/** Below this protection the ward is too weak to visibly activate. */
	constexpr float ActivationThreshold = 5.0f;

	void AddEvent(FCraftExpeditionResult& Result, ECraftEventType Type, const FText& Text, float Damage, float HP)
	{
		FCraftExpeditionEvent& Event = Result.Events.AddDefaulted_GetRef();
		Event.Type = Type;
		Event.Text = Text;
		Event.Damage = Damage;
		Event.HPAfter = HP;
	}
}

FCraftInscription UCraftRules::InterpretInscription(int32 StrokeCount, float WaxCoverage, int32 EnclosedShapes,
	const FCraftFabric& Fabric, bool bIndigoDye, int32 KnownMotifCount)
{
	const UCraftSettings* Settings = UCraftSettings::Get();

	FCraftInscription Result;
	Result.StrokeCount = StrokeCount;
	Result.WaxCoverage = FMath::Max(WaxCoverage, 0.0f);
	Result.EnclosedShapes = FMath::Max(EnclosedShapes, 0);
	Result.CapacityFill = Result.WaxCoverage / FMath::Max(Fabric.MagicCapacity, 0.01f);

	Result.Strength = FMath::Clamp(Result.CapacityFill, 0.0f, 1.0f);
	const float Overfill = FMath::Max(Result.CapacityFill - 1.0f, 0.0f);

	const int32 CountedShapes = FMath::Min(Result.EnclosedShapes, Settings->MaxCountedShapes);
	float Stability = Settings->BaseStability
		+ Settings->MotifStabilityBonus * KnownMotifCount
		+ Settings->StabilityPerShape * CountedShapes
		- Settings->OverfillPenalty * Overfill;
	if (bIndigoDye)
	{
		Stability += Settings->IndigoStabilityBonus;
	}
	// An empty cloth has nothing to stabilise.
	Result.Stability = Result.WaxCoverage > 0.0f ? FMath::Clamp(Stability, 0.0f, 1.0f) : 0.0f;
	return Result;
}

float UCraftRules::CutCoverageMultiplier(ECraftCutStyle Cut)
{
	switch (Cut)
	{
	case ECraftCutStyle::Short: return 0.8f;
	case ECraftCutStyle::Long: return 1.2f;
	default: return 1.0f;
	}
}

float UCraftRules::CutWeightMultiplier(ECraftCutStyle Cut)
{
	switch (Cut)
	{
	case ECraftCutStyle::Short: return 0.8f;
	case ECraftCutStyle::Long: return 1.25f;
	default: return 1.0f;
	}
}

FCraftGarment UCraftRules::BuildGarment(const FCraftGarment& Choices, const FCraftCommission& Commission,
	const FCraftGarmentBlueprint& Blueprint, const FCraftFabric& Fabric)
{
	FCraftGarment Garment = Choices;
	const float CutCoverage = CutCoverageMultiplier(Garment.CutStyle);

	Garment.Weight = Blueprint.BaseWeight * Fabric.Weight * CutWeightMultiplier(Garment.CutStyle);
	// Cut on the bias: drapes and covers better, but the stretched weave is weaker.
	Garment.Coverage = FMath::Clamp(Blueprint.BaseCoverage * CutCoverage * (Garment.bOnBias ? 1.05f : 1.0f), 0.0f, 1.0f);
	Garment.Durability = Fabric.Durability * CutCoverage * (Garment.bOnBias ? 0.9f : 1.0f);
	// Slice rule: the commission names the effect family; the drawing decides how well it works (GDD open question 3).
	Garment.Effect = Commission.RequiredEffect;
	Garment.Protection = 100.0f * Garment.Inscription.Strength * Garment.Inscription.Stability * Garment.Coverage;
	return Garment;
}

FCraftExpeditionResult UCraftRules::ResolveExpedition(const FCraftCommission& Commission, const FCraftGarment& Garment,
	const FCraftGarmentBlueprint& Blueprint)
{
	FCraftExpeditionResult Result;
	float HP = StartingHP;
	const bool bWardActive = Garment.Protection > ActivationThreshold;
	const float WardFactor = 1.0f - FMath::Clamp(Garment.Protection / 100.0f, 0.0f, 1.0f);

	FFormatNamedArguments Args;
	Args.Add(TEXT("Name"), Commission.CustomerName);
	Args.Add(TEXT("Hazard"), Commission.HazardName);
	Args.Add(TEXT("Enemy"), Commission.EnemyName);

	AddEvent(Result, ECraftEventType::Travel,
		FText::Format(LOCTEXT("SetOut", "{Name} sets out at dawn wearing your work."), Args), 0.0f, HP);

	if (Garment.Weight > Blueprint.MaxWeight)
	{
		HP -= SlowedDamage;
		AddEvent(Result, ECraftEventType::Travel,
			FText::Format(LOCTEXT("Slowed", "The garment is heavy. {Name} tires early and falls behind."), Args), SlowedDamage, HP);
	}

	// Physical encounter: the cloth itself blocks some of the blow; a physical ward blocks more.
	{
		float Damage = Commission.EnemyAttack * (1.0f - FMath::Clamp(Garment.Durability / 100.0f, 0.0f, MaxDurabilityBlock));
		if (Garment.Effect == ECraftEffectFamily::WardPhysical && bWardActive)
		{
			AddEvent(Result, ECraftEventType::InscriptionActivates,
				FText::Format(LOCTEXT("WardPhysical", "{Enemy} strikes. Your inscription flares and the blow skids off the cloth."), Args), 0.0f, HP);
			Damage *= WardFactor;
		}
		HP -= Damage;
		FFormatNamedArguments DamageArgs = Args;
		DamageArgs.Add(TEXT("Damage"), FText::AsNumber(FMath::RoundToInt(Damage)));
		AddEvent(Result, ECraftEventType::Encounter,
			FText::Format(LOCTEXT("Encounter", "{Name} fights off {Enemy}. (-{Damage} HP)"), DamageArgs), Damage, HP);
	}

	// Environmental hazard: only a poison ward helps.
	{
		float Damage = Commission.HazardStrength;
		if (Garment.Effect == ECraftEffectFamily::WardPoison && bWardActive)
		{
			AddEvent(Result, ECraftEventType::InscriptionActivates,
				FText::Format(LOCTEXT("WardPoison", "Your inscription glows. {Hazard} thins around {Name}."), Args), 0.0f, HP);
			Damage *= WardFactor;
		}
		HP -= Damage;
		FFormatNamedArguments DamageArgs = Args;
		DamageArgs.Add(TEXT("Damage"), FText::AsNumber(FMath::RoundToInt(Damage)));
		AddEvent(Result, ECraftEventType::Hazard,
			FText::Format(LOCTEXT("Hazard", "{Name} pushes through {Hazard}. (-{Damage} HP)"), DamageArgs), Damage, HP);
	}

	HP = FMath::Max(HP, 0.0f);
	Result.FinalHP = HP;
	if (HP >= SuccessHP)
	{
		Result.Outcome = ECraftExpeditionOutcome::Success;
		AddEvent(Result, ECraftEventType::Result, FText::Format(LOCTEXT("Success", "{Name} completes the journey and turns for home."), Args), 0.0f, HP);
	}
	else if (HP > RetreatHP)
	{
		Result.Outcome = ECraftExpeditionOutcome::Injured;
		AddEvent(Result, ECraftEventType::Result, FText::Format(LOCTEXT("Injured", "{Name} makes it through, hurt, and heads home slowly."), Args), 0.0f, HP);
	}
	else
	{
		Result.Outcome = ECraftExpeditionOutcome::Retreat;
		AddEvent(Result, ECraftEventType::Result, FText::Format(LOCTEXT("Retreat", "{Name} can't go on and retreats."), Args), 0.0f, HP);
	}
	return Result;
}

FCraftMaskStats UCraftRules::AnalyseWaxMask(const TArray<uint8>& Mask, int32 Width, int32 Height, float MinEnclosedFraction)
{
	FCraftMaskStats Stats;
	const int32 Count = Width * Height;
	if (Width <= 0 || Height <= 0 || Mask.Num() != Count)
	{
		return Stats;
	}

	int32 WaxCount = 0;
	// -2 = wax, -1 = unvisited background, 0+ = visited
	TArray<int32> Label;
	Label.SetNumUninitialized(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const bool bWax = Mask[Index] != 0;
		Label[Index] = bWax ? -2 : -1;
		WaxCount += bWax ? 1 : 0;
	}
	Stats.WaxCoverage = static_cast<float>(WaxCount) / static_cast<float>(Count);

	const int32 MinArea = FMath::Max(1, FMath::CeilToInt(MinEnclosedFraction * Count));
	TArray<int32> Stack;
	int32 RegionId = 0;
	for (int32 Start = 0; Start < Count; ++Start)
	{
		if (Label[Start] != -1)
		{
			continue;
		}
		int32 Area = 0;
		bool bTouchesBorder = false;
		Label[Start] = RegionId;
		Stack.Reset();
		Stack.Add(Start);
		while (Stack.Num() > 0)
		{
			const int32 Cell = Stack.Pop(EAllowShrinking::No);
			const int32 X = Cell % Width;
			const int32 Y = Cell / Width;
			++Area;
			bTouchesBorder |= (X == 0 || Y == 0 || X == Width - 1 || Y == Height - 1);

			const int32 Neighbours[4] = {
				X > 0 ? Cell - 1 : -1,
				X < Width - 1 ? Cell + 1 : -1,
				Y > 0 ? Cell - Width : -1,
				Y < Height - 1 ? Cell + Width : -1 };
			for (const int32 N : Neighbours)
			{
				if (N >= 0 && Label[N] == -1)
				{
					Label[N] = RegionId;
					Stack.Add(N);
				}
			}
		}
		if (!bTouchesBorder && Area >= MinArea)
		{
			++Stats.EnclosedShapes;
		}
		++RegionId;
	}
	return Stats;
}

TArray<uint8> UCraftRules::MaskFromPixels(const TArray<FColor>& Pixels, int32 Width, int32 Height, int32 MaxSize,
	int32& OutWidth, int32& OutHeight)
{
	TArray<uint8> Mask;
	OutWidth = 0;
	OutHeight = 0;
	if (Width <= 0 || Height <= 0 || Pixels.Num() != Width * Height)
	{
		return Mask;
	}

	const int32 Step = FMath::Max(1, FMath::DivideAndRoundUp(FMath::Max(Width, Height), FMath::Max(MaxSize, 16)));
	OutWidth = FMath::DivideAndRoundUp(Width, Step);
	OutHeight = FMath::DivideAndRoundUp(Height, Step);

	auto Brightness = [](const FColor& C) { return FMath::Max3(C.R, C.G, C.B); };

	// The bare cloth is whatever brightness is most common.
	int32 Histogram[32] = {};
	for (int32 Y = 0; Y < OutHeight; ++Y)
	{
		for (int32 X = 0; X < OutWidth; ++X)
		{
			++Histogram[Brightness(Pixels[(Y * Step) * Width + X * Step]) >> 3];
		}
	}
	int32 BackgroundBin = 0;
	for (int32 Bin = 1; Bin < 32; ++Bin)
	{
		if (Histogram[Bin] > Histogram[BackgroundBin])
		{
			BackgroundBin = Bin;
		}
	}
	const int32 Background = BackgroundBin * 8 + 4;
	constexpr int32 WaxDifference = 40;

	Mask.SetNumZeroed(OutWidth * OutHeight);
	for (int32 Y = 0; Y < OutHeight; ++Y)
	{
		for (int32 X = 0; X < OutWidth; ++X)
		{
			const int32 Value = Brightness(Pixels[(Y * Step) * Width + X * Step]);
			Mask[Y * OutWidth + X] = FMath::Abs(Value - Background) > WaxDifference ? 1 : 0;
		}
	}
	return Mask;
}

#undef LOCTEXT_NAMESPACE
