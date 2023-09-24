// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshMorpherToolLibrary.generated.h"

class UMeshMorpherMeshComponent;
class UMeshMorpherSettings;
class UStandaloneMaskSelection;

UCLASS()
class MESHMORPHEREDITOR_API UMeshMorpherToolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Misc")
		static void SaveMaskSelection(UStandaloneMaskSelection* MaskSelection, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Misc")
		static void MarkAsModified(UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Settings")
		static UMeshMorpherSettings* GetSettings();

};