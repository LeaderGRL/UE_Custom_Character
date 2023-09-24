// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherCommands.h"

#define LOCTEXT_NAMESPACE "FMeshMorpherModule"

void FMeshMorpherCommands::RegisterCommands()
{
	UI_COMMAND(OpenToolkitAction, "Mesh Morpher", "Open Mesh Morpher", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SaveButton, "Save", "Save Skeletal Mesh", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::S, false, true, false, false));

	UI_COMMAND(OpenMorphButton, "Open Morph Target", "Opens the selected Morph Target.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::O, false, true, false, false));
	UI_COMMAND(DeselectMorphButton, "Close Morph Target", "Closes the Morph Target currently open.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::D, false, true, false, false));
	UI_COMMAND(AddMorphButton, "Add New", "Adds a Morph Target to the skeletal mesh.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::A, false, true, false, false));
	UI_COMMAND(DeleteMorphButton, "Delete", "Removes the selected morph target from the skeletal mesh.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::Delete, false, false, false, false));
	UI_COMMAND(RenameMorphButton, "Rename", "Renames the selected morph target.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::R, false, true, false, false));
	UI_COMMAND(DuplicateMorphButton, "Duplicate", "Duplicates the selected morph target.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::D, false, true, true, false));
	UI_COMMAND(EnableBuildDataButton, "Enable Build Mesh Data", "Enables re/building of Mesh Data.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());
	UI_COMMAND(DisableBuildDataButton, "Disable Build Mesh Data", "Disables re/building of Mesh Data.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());
	UI_COMMAND(SettingsButton, "Settings", "Open Settings Window.", EUserInterfaceActionType::Button, FInputChord(), FInputChord());

	UI_COMMAND(CopyMorphButton, "Copy", "Copy a Morph Target to another skeletal mesh.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::C, false, true, false, false));
	UI_COMMAND(SelectReferenceMeshButton, "Select a Reference Skeletal Mesh", "Select a Reference Skeletal Mesh that allows you to easily follow a model while sculpting.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::L, true, true, false, false));
	UI_COMMAND(MergeMorphsButton, "Merge", "Merge multiple Morph Targets into single Morph Target.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::M, false, true, false, false));
	UI_COMMAND(ExportMorphButton, "Export Morph Target As Static Mesh", "Export a Morph target to a Static Mesh.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::E, false, true, true, false));
	UI_COMMAND(ExportOBJButton, "Export Mesh As OBJ", "Export a Skeletal Mesh to OBJ file.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(BakeButton, "Bake", "Bake Morph Targets into a Skeletal Mesh.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::B, false, true, false, false));

	UI_COMMAND(CreateMorphFromPoseButton, "Create From Pose", "Creates a Morph Target from selected Pose", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::P, true, true, false, false));
	UI_COMMAND(CreateMorphFromMeshButton, "Create From Mesh", "Creates a Morph Target from selected Mesh", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::I, true, true, false, false));
	UI_COMMAND(CreateMorphFromFBXButton, "Create From Mesh Files", "Creates a Morph Target from selected Mesh Files", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::F, true, true, false, false));
	UI_COMMAND(ApplyMorphTargetToLODsButton, "Apply to LODs", "Manually applies selected Morph Target(s) to LODs", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::F1, false, false, false, false));
	UI_COMMAND(MorphMagnitudeButton, "Set Magnitude", "Sets the Magnitude of the selected Morph Target(s)", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::M, false, false, true, false));

	UI_COMMAND(CreateMetaMorphFromFBXButton, "Create Meta Morph from Mesh Files", "Creates a Meta Morph asset from selected Mesh Files", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::G, true, true, false, false));
	UI_COMMAND(CreateMetaMorphFromMorphTargetButton, "Create Meta Morph from Morph Target(s)", "Creates a Meta Morph asset from selected Morph Target(s)", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::H, true, true, false, false));
	UI_COMMAND(CreateMorphFromMetaButton, "Create From Meta Morph", "Creates Morph Target(s) from selected Meta Morph assets", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::J, true, true, false, false));
	UI_COMMAND(CreateMorphFromMetaCurrentButton, "Create From Meta Morph (Current Mesh)", "Creates Morph Target(s) on Current Mesh from selected Meta Morph assets", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::K, true, true, false, false));

	UI_COMMAND(ToggleMaskSelectionButton, "Toggle Mask Selection", "Toggles the ability to select masking.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::V, true, true, false, false));
	UI_COMMAND(ClearMaskSelectiontButton, "Clear Mask Selection", "Clears current mask selection.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::C, true, true, false, false));
	UI_COMMAND(SaveSelectionButton, "Save Selection", "Saves a Mask Selection Asset", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SaveSelectionToAssetButton, "Save Selection As...", "Saves a Mask Selection to UE Asset.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenMaskSelectionButton, "Open Mask Selection", "Opens a Mask Selection from UE Asset.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ExportMaskSelectionOBJButton, "Export Mask Selection to OBJ", "Export current Mask Selection to OBJ file.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ExportMorphTargetOBJButton, "Export Morph Target As OBJ", "Export a Morph Target to OBJ file.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ExportMorphTargetOBJMenuButton, "Export Morph Target As OBJ", "Export a Morph Target to OBJ file.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(FocusCameraButton, "Focus Camera", "Focus Camera at Cursor.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::F, false, false, false, false));

	UI_COMMAND(IncreaseBrushSizeButton, "Increase Brush Size", "Increase Brush Size", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::RightBracket, false, false, false, false));
	UI_COMMAND(DecreaseBrushSizeButton, "Decrease Brush Size", "Decrease Brush Size", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::LeftBracket, false, false, false, false));


	UI_COMMAND(StitchMorphButton, "Stitch Morph Target", "Stitch Morph Target", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(New, "New", "Open a new Mesh Morpher tab.", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::N, false, true, false, false));
	UI_COMMAND(Exit, "Exit", "Exit Mesh Morpher", EUserInterfaceActionType::Button, FInputChord(), FInputChord(EKeys::X, false, true, false, false));
	UI_COMMAND(About, "About", "Show About Information", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(ToolbarTab, "Toolbar", "Toggle Toolbar Tab", EUserInterfaceActionType::Check, FInputChord());
	UI_COMMAND(PoseTab, "Pose", "Toggle Pose Tab", EUserInterfaceActionType::Check, FInputChord());
	UI_COMMAND(PreviewTab, "Preview", "Toggle Preview Tab", EUserInterfaceActionType::Check, FInputChord());
	UI_COMMAND(ToolTab, "Tool", "Toggle Tool Tab", EUserInterfaceActionType::Check, FInputChord());
	UI_COMMAND(MorphTargetsTab, "Morph Targets", "Toggle Morph Targets Tab", EUserInterfaceActionType::Check, FInputChord());
}

#undef LOCTEXT_NAMESPACE
