// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MeshMorpherRuntime : ModuleRules
{
	public MeshMorpherRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"InputCore",
				"SlateCore",
				"RenderCore",
				"RHI",
				"MeshDescription",
				"StaticMeshDescription",
				"GeometryCore",
				"GeometryFramework",
				"ModelingComponents",
				"MeshConversion",
				"InteractiveToolsFramework",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		if (Target.Type == TargetRules.TargetType.Editor)
			PrivateDependencyModuleNames.AddRange(
				new[]
				{
					"UnrealEd",
					"DesktopPlatform"
				}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
