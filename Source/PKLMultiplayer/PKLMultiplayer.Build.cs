// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PKLMultiplayer : ModuleRules
{
	public PKLMultiplayer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// Online session layer (transport-agnostic: LAN or Steam via config/flag).
		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils" });

		// OnlineSubsystemSteam is enabled in PKLMultiplayer.uproject (Plugins section).
	}
}
