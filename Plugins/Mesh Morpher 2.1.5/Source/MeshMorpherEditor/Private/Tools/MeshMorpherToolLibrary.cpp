// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Tools/MeshMorpherToolLibrary.h"
#include "Components/MeshMorpherMeshComponent.h"
#include "InteractiveToolManager.h"
#include "MeshMorpherSettings.h"
#include "StandaloneMaskSelection.h"
#include "FileHelpers.h"



void UMeshMorpherToolLibrary::SaveMaskSelection(UStandaloneMaskSelection* MaskSelection, bool bForce)
{
	if (MaskSelection)
	{
		if (bForce)
		{
			MarkAsModified(MaskSelection);
		}

		TArray< UPackage*> Packages;

		UPackage* SkeletalMeshPkg = Cast<UPackage>(MaskSelection->GetOuter());
		if (SkeletalMeshPkg)
		{
			Packages.Add(SkeletalMeshPkg);
		}
		FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, true, nullptr, false, true);
	}
}


void UMeshMorpherToolLibrary::MarkAsModified(UObject* Object)
{
	if (Object)
	{
		Object->MarkPackageDirty();
	}
}

UMeshMorpherSettings* UMeshMorpherToolLibrary::GetSettings()
{
	return GetMutableDefault<UMeshMorpherSettings>();
}