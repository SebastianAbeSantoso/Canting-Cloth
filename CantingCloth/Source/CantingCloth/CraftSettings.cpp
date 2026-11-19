#include "CraftSettings.h"

#include "Engine/TextureRenderTarget2D.h"

#define LOCTEXT_NAMESPACE "CraftSettings"

DEFINE_LOG_CATEGORY(LogCraftLoop);

UCraftSettings::UCraftSettings()
{
	ClothActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Blueprints/Canting/BP_ClothActor.BP_ClothActor_C")));
	LegacyWaxRenderTarget = TSoftObjectPtr<UTextureRenderTarget2D>(FSoftObjectPath(TEXT("/Game/Materials/RT_CantingWax.RT_CantingWax")));

	// ---- Fabrics ----
	{
		FCraftFabric& Mori = Fabrics.AddDefaulted_GetRef();
		Mori.Id = TEXT("Mori_Cotton");
		Mori.DisplayName = LOCTEXT("MoriName", "Mori Cotton");
		Mori.Description = LOCTEXT("MoriDesc", "Fine, light cotton. Takes wax deeply and holds a lot of inscription.");
		Mori.Weight = 1.0f;
		Mori.Durability = 40.0f;
		Mori.MagicCapacity = 0.12f;
		Mori.SheetWidth = 10;
		Mori.SheetHeight = 14;

		FCraftFabric& Blacu = Fabrics.AddDefaulted_GetRef();
		Blacu.Id = TEXT("Blacu_Canvas");
		Blacu.DisplayName = LOCTEXT("BlacuName", "Blacu Canvas");
		Blacu.Description = LOCTEXT("BlacuDesc", "Coarse, heavy weave. Tough against blows, but wax sits on the surface and overloads quickly.");
		Blacu.Weight = 1.5f;
		Blacu.Durability = 70.0f;
		Blacu.MagicCapacity = 0.08f;
		// A narrow bolt: the cloak's two body panels can't sit side by side along the grain.
		Blacu.SheetWidth = 9;
		Blacu.SheetHeight = 15;
	}

	auto Piece = [](const FText& Name, int32 Width, int32 Height)
	{
		FCraftPatternPiece Result;
		Result.Name = Name;
		Result.Width = Width;
		Result.Height = Height;
		return Result;
	};

	// ---- Garment blueprints ----
	{
		FCraftGarmentBlueprint& Cloak = Blueprints.AddDefaulted_GetRef();
		Cloak.Id = TEXT("Cloak_Light");
		Cloak.DisplayName = LOCTEXT("CloakName", "Light Cloak");
		Cloak.Description = LOCTEXT("CloakDesc", "Two panels and a hood. Wide surface for inscriptions, little armour.");
		Cloak.BaseWeight = 1.0f;
		Cloak.MaxWeight = 1.6f;
		Cloak.BaseCoverage = 0.7f;
		Cloak.Pieces = {
			Piece(LOCTEXT("CloakBack", "Back"), 5, 8),
			Piece(LOCTEXT("CloakFront", "Front"), 5, 8),
			Piece(LOCTEXT("CloakHood", "Hood"), 4, 4) };
		Cloak.SeamCount = 3;

		FCraftGarmentBlueprint& Vest = Blueprints.AddDefaulted_GetRef();
		Vest.Id = TEXT("Vest_Guardian");
		Vest.DisplayName = LOCTEXT("VestName", "Guardian's Vest");
		Vest.Description = LOCTEXT("VestDesc", "Layered torso panels. Covers the vitals; heavier to wear.");
		Vest.BaseWeight = 1.6f;
		Vest.MaxWeight = 3.0f;
		Vest.BaseCoverage = 0.9f;
		Vest.Pieces = {
			Piece(LOCTEXT("VestFront", "Front"), 6, 6),
			Piece(LOCTEXT("VestBack", "Back"), 6, 6),
			Piece(LOCTEXT("VestSideL", "Left side"), 3, 5),
			Piece(LOCTEXT("VestSideR", "Right side"), 3, 5) };
		Vest.SeamCount = 4;
	}

	// ---- Customers ----
	{
		FCraftCommission& Scout = Commissions.AddDefaulted_GetRef();
		Scout.Id = TEXT("Scout");
		Scout.CustomerName = LOCTEXT("ScoutName", "Ranti the Scout");
		Scout.RequestText = LOCTEXT("ScoutRequest",
			"\"I cross the Rawa Getih marsh in three days. The air there rots the lungs, and the boars don't care who you are. "
			"I need something light that keeps the miasma off me.\"");
		Scout.AllowedBlueprints = { TEXT("Cloak_Light") };
		Scout.RequiredEffect = ECraftEffectFamily::WardPoison;
		Scout.HazardName = LOCTEXT("ScoutHazard", "marsh miasma");
		Scout.HazardStrength = 60.0f;
		Scout.EnemyName = LOCTEXT("ScoutEnemy", "a marsh boar");
		Scout.EnemyAttack = 20.0f;
		Scout.RewardGold = 30;
		Scout.RewardMotif = TEXT("Kawung");
		Scout.ReturnSuccessText = LOCTEXT("ScoutSuccess",
			"Ranti comes back muddy to the knees but breathing easy. In the river village she copied a kawung motif from an old pembatik, and she gives you the sketch.");
		Scout.ReturnInjuredText = LOCTEXT("ScoutInjured",
			"Ranti limps in, coughing, the hem of the cloak torn. She made it through. She still hands you the kawung sketch from the river village.");
		Scout.ReturnRetreatText = LOCTEXT("ScoutRetreat",
			"Ranti turned back at the edge of the marsh. \"It wouldn't have held,\" she says, and pays only for the cloth.");

		FCraftCommission& Guardian = Commissions.AddDefaulted_GetRef();
		Guardian.Id = TEXT("Guardian");
		Guardian.CustomerName = LOCTEXT("GuardianName", "Pak Darma the Guardian");
		Guardian.RequestText = LOCTEXT("GuardianRequest",
			"\"Bandits have been raiding the caravan road. I stand in front of the carts, so whatever hits, hits me. "
			"Make me something that turns a blade.\"");
		Guardian.AllowedBlueprints = { TEXT("Vest_Guardian") };
		Guardian.RequiredEffect = ECraftEffectFamily::WardPhysical;
		Guardian.HazardName = LOCTEXT("GuardianHazard", "the dust storm on the pass");
		Guardian.HazardStrength = 15.0f;
		Guardian.EnemyName = LOCTEXT("GuardianEnemy", "a band of road raiders");
		Guardian.EnemyAttack = 65.0f;
		Guardian.RewardGold = 50;
		Guardian.RewardMotif = TEXT("Parang");
		Guardian.ReturnSuccessText = LOCTEXT("GuardianSuccess",
			"Pak Darma returns with the caravan intact. The merchants send a bolt of cloth with him, and he teaches you the parang line his mother wove.");
		Guardian.ReturnInjuredText = LOCTEXT("GuardianInjured",
			"Pak Darma returns with a bandaged arm, but the carts got through. He shows you the parang line his mother wove.");
		Guardian.ReturnRetreatText = LOCTEXT("GuardianRetreat",
			"The caravan turned back after the first ambush. Pak Darma says nothing, sets down the torn vest, and leaves.");
	}
}

const FCraftCommission* UCraftSettings::FindCommission(FName Id) const
{
	return Commissions.FindByPredicate([Id](const FCraftCommission& Item) { return Item.Id == Id; });
}

const FCraftGarmentBlueprint* UCraftSettings::FindBlueprint(FName Id) const
{
	return Blueprints.FindByPredicate([Id](const FCraftGarmentBlueprint& Item) { return Item.Id == Id; });
}

const FCraftFabric* UCraftSettings::FindFabric(FName Id) const
{
	return Fabrics.FindByPredicate([Id](const FCraftFabric& Item) { return Item.Id == Id; });
}

#undef LOCTEXT_NAMESPACE
