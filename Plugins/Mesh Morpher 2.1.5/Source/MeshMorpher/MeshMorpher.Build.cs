// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MeshMorpher : ModuleRules
{

    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public MeshMorpher(ReadOnlyTargetRules Target) : base(Target)
    {

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ModulePath, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModulePath, "Private"));

        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "EditorStyle",
                "RenderCore",
                "RHI",
                "MeshConversion",
                "ToolMenus",
                "InputCore",
                "LevelEditor",
                "ApplicationCore",
                "Projects",
                "MeshDescription",
                "StaticMeshDescription",
                "SkeletalMeshDescription",
                "MeshDescriptionOperations",
                "ViewportInteraction",
                "UMG",
                "AssetTools",
                "AssetRegistry",
                "PropertyEditor",
                "Settings",
                "InteractiveToolsFramework",
                "EditorInteractiveToolsFramework",
                "MeshModelingTools",
                "MeshModelingToolsEditorOnly",
                "ViewportInteraction",
                "AdvancedPreviewScene",
                "ModelingComponents",
                "MeshMorpherRuntime",
                "MeshMorpherEditor",
                "MeshUtilities",
                "ContentBrowser",
                "Settings",
                "HTTP",
                "Json",
                "FBX",
                "DynamicMesh",
                "EditorFramework",
                "GeometryCore",
                "GeometryFramework"
				// ... add private dependencies that you statically link with here ...	
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
