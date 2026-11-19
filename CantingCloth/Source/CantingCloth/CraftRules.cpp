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
	const FCraftFabric& Fabric, bool bIndigoDye, int32 KnownMotifCount, float DyeQuality)
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
	// A well-timed dip sets the resist; a pale or cracked one weakens it. 0.5 = neutral (not dyed yet).
	Stability += (FMath::Clamp(DyeQuality, 0.0f, 1.0f) - 0.5f) * 2.0f * Settings->DipStabilityRange;
	// An empty cloth has nothing to stabilise.
	Result.Stability = Result.WaxCoverage > 0.0f ? FMath::Clamp(Stability, 0.0f, 1.0f) : 0.0f;
	return Result;
}

float UCraftRules::CutCoverageMultiplier(float CutLength)
{
	return FMath::Clamp(CutLength, MinCutLength, MaxCutLength);
}

float UCraftRules::CutWeightMultiplier(float CutLength)
{
	// Slightly steeper than coverage: a long hem gains less cover than it adds weight.
	return FMath::Pow(FMath::Clamp(CutLength, MinCutLength, MaxCutLength), 1.2f);
}

FIntPoint UCraftRules::PieceFootprint(const FCraftPatternPiece& Piece, bool bRotated)
{
	return bRotated ? FIntPoint(Piece.Height, Piece.Width) : FIntPoint(Piece.Width, Piece.Height);
}

bool UCraftRules::IsPlacementValid(const FCraftFabric& Fabric, const FCraftGarmentBlueprint& Blueprint,
	const TArray<FCraftPiecePlacement>& Placements, int32 PieceIndex, const FCraftPiecePlacement& Candidate)
{
	if (!Blueprint.Pieces.IsValidIndex(PieceIndex))
	{
		return false;
	}
	const FIntPoint Size = PieceFootprint(Blueprint.Pieces[PieceIndex], Candidate.bRotated);
	if (Candidate.X < 0 || Candidate.Y < 0 || Candidate.X + Size.X > Fabric.SheetWidth || Candidate.Y + Size.Y > Fabric.SheetHeight)
	{
		return false;
	}
	for (int32 Other = 0; Other < Placements.Num() && Other < Blueprint.Pieces.Num(); ++Other)
	{
		const FCraftPiecePlacement& Placed = Placements[Other];
		if (Other == PieceIndex || !Placed.bPlaced)
		{
			continue;
		}
		const FIntPoint OtherSize = PieceFootprint(Blueprint.Pieces[Other], Placed.bRotated);
		const bool bOverlap = Candidate.X < Placed.X + OtherSize.X && Placed.X < Candidate.X + Size.X
			&& Candidate.Y < Placed.Y + OtherSize.Y && Placed.Y < Candidate.Y + Size.Y;
		if (bOverlap)
		{
			return false;
		}
	}
	return true;
}

void UCraftRules::EvaluateLayout(const FCraftFabric& Fabric, const FCraftGarmentBlueprint& Blueprint,
	const TArray<FCraftPiecePlacement>& Placements, float& OutCrossGrainFraction, float& OutWaste)
{
	int32 TotalArea = 0;
	int32 CrossArea = 0;
	for (int32 Index = 0; Index < Blueprint.Pieces.Num(); ++Index)
	{
		const int32 Area = Blueprint.Pieces[Index].Width * Blueprint.Pieces[Index].Height;
		TotalArea += Area;
		if (Placements.IsValidIndex(Index) && Placements[Index].bPlaced && Placements[Index].bRotated)
		{
			CrossArea += Area;
		}
	}
	const int32 SheetArea = FMath::Max(Fabric.SheetWidth * Fabric.SheetHeight, 1);
	OutCrossGrainFraction = TotalArea > 0 ? static_cast<float>(CrossArea) / TotalArea : 0.0f;
	OutWaste = FMath::Clamp(1.0f - static_cast<float>(TotalArea) / SheetArea, 0.0f, 1.0f);
}

bool UCraftRules::AutoArrange(const FCraftFabric& Fabric, const FCraftGarmentBlueprint& Blueprint, TArray<FCraftPiecePlacement>& OutPlacements)
{
	OutPlacements.Init(FCraftPiecePlacement(), Blueprint.Pieces.Num());
	bool bAllPlaced = true;
	for (int32 Index = 0; Index < Blueprint.Pieces.Num(); ++Index)
	{
		bool bPlaced = false;
		for (const bool bRotated : { false, true })
		{
			for (int32 Y = 0; Y < Fabric.SheetHeight && !bPlaced; ++Y)
			{
				for (int32 X = 0; X < Fabric.SheetWidth && !bPlaced; ++X)
				{
					FCraftPiecePlacement Candidate;
					Candidate.bPlaced = true;
					Candidate.X = X;
					Candidate.Y = Y;
					Candidate.bRotated = bRotated;
					if (IsPlacementValid(Fabric, Blueprint, OutPlacements, Index, Candidate))
					{
						OutPlacements[Index] = Candidate;
						bPlaced = true;
					}
				}
			}
			if (bPlaced)
			{
				break;
			}
		}
		bAllPlaced &= bPlaced;
	}
	return bAllPlaced;
}

float UCraftRules::DyeQualityFromDepth(float Depth, float GoodMin, float GoodMax)
{
	if (Depth < GoodMin)
	{
		// Pale: the colour barely took.
		return FMath::Lerp(0.0f, 1.0f, FMath::Clamp(Depth / FMath::Max(GoodMin, 0.01f), 0.0f, 1.0f)) * 0.8f;
	}
	if (Depth <= GoodMax)
	{
		return 1.0f;
	}
	// Too long: the wax softens and cracks (remekan), letting dye vein through the resist.
	const float Over = (Depth - GoodMax) / FMath::Max(1.0f - GoodMax, 0.01f);
	return FMath::Lerp(1.0f, 0.0f, FMath::Clamp(Over, 0.0f, 1.0f));
}

float UCraftRules::StitchAccuracy(float MarkerPosition, float Tolerance)
{
	return FMath::Clamp(1.0f - FMath::Abs(MarkerPosition - 0.5f) / FMath::Max(Tolerance, 0.01f), 0.0f, 1.0f);
}

FCraftGarment UCraftRules::BuildGarment(const FCraftGarment& Choices, const FCraftCommission& Commission,
	const FCraftGarmentBlueprint& Blueprint, const FCraftFabric& Fabric)
{
	FCraftGarment Garment = Choices;
	const float CutCoverage = CutCoverageMultiplier(Garment.CutLength);

	Garment.Weight = Blueprint.BaseWeight * Fabric.Weight * CutWeightMultiplier(Garment.CutLength);
	// Pieces laid across the grain drape and cover better, but the weave is weaker that way.
	Garment.Coverage = FMath::Clamp(Blueprint.BaseCoverage * CutCoverage * (1.0f + 0.05f * Garment.CrossGrainFraction), 0.0f, 1.0f);
	// Seam quality: sloppy stitches (0) lose 15%, perfect ones (1) add 10%.
	Garment.Durability = Fabric.Durability * CutCoverage * (1.0f - 0.1f * Garment.CrossGrainFraction)
		* (0.85f + 0.25f * FMath::Clamp(Garment.SeamQuality, 0.0f, 1.0f));
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
