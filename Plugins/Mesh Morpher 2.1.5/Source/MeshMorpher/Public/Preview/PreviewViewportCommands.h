// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

class FMeshMorpherPreviewSceneCommands : public TCommands<FMeshMorpherPreviewSceneCommands>
{

public:
	FMeshMorpherPreviewSceneCommands() : TCommands<FMeshMorpherPreviewSceneCommands>
		(
			"AdvancedPreviewScene",
			NSLOCTEXT("Contexts", "AdvancedPreviewScene", "Advanced Preview Scene"),
			NAME_None,
			FEditorStyle::Get().GetStyleSetName()
		)
	{}

	/** Toggles environment (sky sphere) visibility */
	TSharedPtr< FUICommandInfo > ToggleEnvironment;

	/** Toggles floor visibility */
	TSharedPtr< FUICommandInfo > ToggleFloor;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;
};