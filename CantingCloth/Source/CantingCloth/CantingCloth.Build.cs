// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class CantingCloth : ModuleRules
{
	public CantingCloth(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore" });

		// Slate/SlateCore: built-in craft loop UI. DeveloperSettings: Project Settings page for the loop's content and tuning.
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "DeveloperSettings" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
