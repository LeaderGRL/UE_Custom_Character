// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Components/MeshMorpherMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshMorpherRuntimeLibrary.generated.h"

class USkeletalMesh;
class USceneComponent;
class USkeletalMeshComponent;
using namespace UE::Geometry;

UCLASS()
class MESHMORPHERRUNTIME_API UMeshMorpherRuntimeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
		UMeshMorpherRuntimeLibrary();
	~UMeshMorpherRuntimeLibrary();
	
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Misc")
		static void InvalidateCachedBounds(USkeletalMeshComponent* SkeletalMeshComponent);
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Misc")
		static USceneComponent* CreateComponent(UObject* Outer, TSubclassOf<USceneComponent> Class);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Misc")
		static UMeshMorpherMeshComponent* CreateSphereComponent(UObject* Outer, const int32 Resolution = 32);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Misc")
		static void DestroyComponent(USceneComponent* Component);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Utilities")
		static UMorphTarget* FindMorphTarget(USkeletalMesh* SkeletalMesh, FName Name);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Utilities")
		static void GetMorphTargetsNames(USkeletalMesh* SkeletalMesh, TArray<FName>& Names);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Utilities")
		static bool RemoveMorphTarget(USkeletalMesh* SkeletalMesh, FName Name);
private:
	static void OnEndPIE(const bool bIsSimulating);
public:
	static FDelegateHandle EndPIEHandle;
	static TMap<USkeletalMesh*, int32> Tracker;
};