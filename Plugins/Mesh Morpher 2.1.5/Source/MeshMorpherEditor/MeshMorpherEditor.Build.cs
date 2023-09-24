// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

using UnrealBuildTool;
using System.IO;


public class MeshMorpherEditor : ModuleRules
{

    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../ThirdParty/")); }
    }

    public MeshMorpherEditor(ReadOnlyTargetRules Target) : base(Target)
    {

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ModulePath, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModulePath, "Private"));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                 "Core"
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "RenderCore",
                "RHI",
                "MeshConversion",
                "InteractiveToolsFramework",
                "EditorInteractiveToolsFramework",
                "MeshModelingTools",
                "MeshDescription",
                "MeshMorpherRuntime",
                "ModelingComponents",
                "AssetTools",
                "AssetRegistry",
                "UMG",
				// ... add private dependencies that you statically link with here ...	
			}
            );

        BuildVersion Version;
        var File = BuildVersion.GetDefaultFileName();
        if(BuildVersion.TryRead(File, out Version))
        {
            if(Version.MajorVersion == 5)
            {
                PrivateDependencyModuleNames.AddRange(
                    new string[]
                    {
                        "EditorFramework",
                        "GeometryCore",
                        "GeometryFramework",
                        "ModelingComponents",
                        "MeshConversion",
                    }
                    );
            }
        } else
        {
            throw new BuildException("Couldn't read engine version.");
        }


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
