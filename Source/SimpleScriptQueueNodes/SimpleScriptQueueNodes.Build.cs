// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SimpleScriptQueueNodes : ModuleRules
{
	public SimpleScriptQueueNodes(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateIncludePaths.AddRange( new string[] { "SimpleScriptQueueEditor/Private" });
		
		PublicIncludePaths.AddRange( new string[] { "SimpleScriptQueueEditor/Public" });
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "PropertyEditor" });

       PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd", "EditorStyle", "GraphEditor", "KismetCompiler", "BlueprintGraph", "SimpleScriptQueue" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
