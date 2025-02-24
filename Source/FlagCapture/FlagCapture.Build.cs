// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlagCapture : ModuleRules
{
	public FlagCapture(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GameplayTags", "InputCore", "NetCore", "UMG", "AIModule", "NavigationSystem", "GameplayAbilities", "DeveloperSettings" });

        PrivateDependencyModuleNames.AddRange(new string[] { "CoreOnline", "EnhancedInput", "GameplayTasks", "AIModule", "MotionWarping", "Niagara", "PhysicsCore", "Slate", "SlateCore", "EngineSettings" });

    }
}
