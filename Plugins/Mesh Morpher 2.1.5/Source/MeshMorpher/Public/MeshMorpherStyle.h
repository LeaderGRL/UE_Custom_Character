// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"

class FMeshMorpherStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** @return The Slate style set for the Shooter game */
	static TSharedPtr< class ISlateStyle > Get();

	static FName GetStyleSetName();

private:

	static TSharedPtr< class FSlateStyleSet > Style;
};