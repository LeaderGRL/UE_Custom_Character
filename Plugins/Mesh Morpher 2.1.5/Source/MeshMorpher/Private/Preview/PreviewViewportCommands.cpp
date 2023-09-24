// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Preview/PreviewViewportCommands.h"

#define LOCTEXT_NAMESPACE "VoxelPreviewSceneCommands"

void FMeshMorpherPreviewSceneCommands::RegisterCommands()
{
	UI_COMMAND(ToggleEnvironment, "Toggle Environment", "Toggles Environment visibility", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::I));
	UI_COMMAND(ToggleFloor, "Toggle Floor", "Toggles floor visibility", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::O));
}

#undef LOCTEXT_NAMESPACE