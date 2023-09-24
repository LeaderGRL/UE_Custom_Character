// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "MeshMorpherStyle.h"
#include "Tools/InteractiveToolsCommands.h"

class FMeshMorpherCommands : public TCommands<FMeshMorpherCommands>
{
public:

	FMeshMorpherCommands()
		: TCommands<FMeshMorpherCommands>(TEXT("MeshMorpher"), NSLOCTEXT("Contexts", "MeshMorpher", "MeshMorpher Plugin"), NAME_None, FMeshMorpherStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenToolkitAction;
	TSharedPtr<FUICommandInfo> SaveButton;

	TSharedPtr<FUICommandInfo> OpenMorphButton;
	TSharedPtr<FUICommandInfo> DeselectMorphButton;
	TSharedPtr<FUICommandInfo> AddMorphButton;
	TSharedPtr<FUICommandInfo> DeleteMorphButton;
	TSharedPtr<FUICommandInfo> RenameMorphButton;
	TSharedPtr<FUICommandInfo> DuplicateMorphButton;
	TSharedPtr<FUICommandInfo> EnableBuildDataButton;
	TSharedPtr<FUICommandInfo> DisableBuildDataButton;
	TSharedPtr<FUICommandInfo> SettingsButton;

	TSharedPtr<FUICommandInfo> CopyMorphButton;
	TSharedPtr<FUICommandInfo> SelectReferenceMeshButton;
	TSharedPtr<FUICommandInfo> MergeMorphsButton;
	TSharedPtr<FUICommandInfo> ExportMorphButton;
	TSharedPtr<FUICommandInfo> ExportOBJButton;
	TSharedPtr<FUICommandInfo> BakeButton;
	TSharedPtr<FUICommandInfo> CreateMorphFromPoseButton;
	TSharedPtr<FUICommandInfo> CreateMorphFromMeshButton;
	TSharedPtr<FUICommandInfo> CreateMorphFromFBXButton;
	TSharedPtr<FUICommandInfo> ApplyMorphTargetToLODsButton;
	TSharedPtr<FUICommandInfo> MorphMagnitudeButton;
	TSharedPtr<FUICommandInfo> CreateMetaMorphFromMorphTargetButton;
	TSharedPtr<FUICommandInfo> CreateMetaMorphFromFBXButton;
	
	TSharedPtr<FUICommandInfo> CreateMorphFromMetaButton;
	TSharedPtr<FUICommandInfo> CreateMorphFromMetaCurrentButton;

	TSharedPtr<FUICommandInfo> ToggleMaskSelectionButton;
	TSharedPtr<FUICommandInfo> ClearMaskSelectiontButton;
	TSharedPtr<FUICommandInfo> SaveSelectionButton;
	TSharedPtr<FUICommandInfo> SaveSelectionToAssetButton;
	TSharedPtr<FUICommandInfo> OpenMaskSelectionButton;
	TSharedPtr<FUICommandInfo> ExportMaskSelectionOBJButton;
	TSharedPtr<FUICommandInfo> ExportMorphTargetOBJButton;
	TSharedPtr<FUICommandInfo> ExportMorphTargetOBJMenuButton;
	TSharedPtr<FUICommandInfo> FocusCameraButton;

	TSharedPtr<FUICommandInfo> IncreaseBrushSizeButton;
	TSharedPtr<FUICommandInfo> DecreaseBrushSizeButton;


	TSharedPtr<FUICommandInfo> StitchMorphButton;

	TSharedPtr<FUICommandInfo> New;
	TSharedPtr<FUICommandInfo> Exit;
	TSharedPtr<FUICommandInfo> About;

	TSharedPtr<FUICommandInfo> ToolbarTab;
	TSharedPtr<FUICommandInfo> PoseTab;
	TSharedPtr<FUICommandInfo> PreviewTab;
	TSharedPtr<FUICommandInfo> ToolTab;
	TSharedPtr<FUICommandInfo> MorphTargetsTab;
};