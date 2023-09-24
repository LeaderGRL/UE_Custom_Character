// Copyright 2019-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/InputChord.h"
#include "MeshMorpherToolInterface.generated.h"

class USkeletalMesh;
class UMeshMorpherMeshComponent;
class UMeshMorpherSettings;
class UStandaloneMaskSelection;
class USkeletalMeshComponent;

UINTERFACE(BlueprintType, Blueprintable)
class MESHMORPHERRUNTIME_API UMeshMorpherToolInterface : public UInterface
{
	GENERATED_BODY()
};

class MESHMORPHERRUNTIME_API IMeshMorpherToolInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnInitialize(UMeshMorpherSettings* Settings, USkeletalMeshComponent* SkeletalMeshComponent, UMeshMorpherMeshComponent* WeldedMeshComponent, UMeshMorpherMeshComponent* OriginalMeshComponent);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnBeginDrag(const FVector& Origin, const FVector& Direction);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnUpdateDrag(const FVector& Origin, const FVector& Direction);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnEndDrag(const FVector& Origin, const FVector& Direction);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		bool OnUpdateHover(const FVector& Origin, const FVector& Direction);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnUpdate(float DeltaTime, const FTransform& CameraTransform);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		bool OnApplyTool(const FVector& Origin, const FVector& Direction);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnMeshChanged();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		bool OnHitTest(const FVector& Origin, const FVector& Direction, FHitResult& OutHit);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnUninitialize();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnKeyPressed(const FInputChord& Key);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void OnKeyUnPressed(const FInputChord& Key);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		UMeshMorpherMeshComponent* GetMeshComponent() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		UMeshMorpherMeshComponent* GetOriginalMeshComponent() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface|Selection")
		void ToggleMaskSelection();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface|Selection")
		void ClearMaskSelection();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface|Selection")
		void LoadMaskSelectionAsset(UStandaloneMaskSelection* MaskSelection, bool bAppend);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface|Selection")
		void SaveMaskSelection(UStandaloneMaskSelection* Selection);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface|Selection")
		UStandaloneMaskSelection* GetMaskSelectionAsset() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface|Selection")
		void ExportToFile(const FString& FilePath);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void IncreaseBrushSize();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mesh Morpher|Interface")
		void DecreaseBrushSize();

};