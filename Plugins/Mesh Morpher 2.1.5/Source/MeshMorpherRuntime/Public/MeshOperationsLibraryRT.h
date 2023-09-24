// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshDescription.h"
#include "SkeletalRenderPublic.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Animation/MorphTarget.h"
#include "MeshOperationsLibraryRT.generated.h"

class USkeletalMesh;
class UMeshMorpherMeshComponent;

using namespace UE::Geometry;


UCLASS()
class MESHMORPHERRUNTIME_API UMeshOperationsLibraryRT : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void GetUsedMaterials(USkeletalMesh* SkeletalMesh, int32 LOD, TArray<FSkeletalMaterial>& OutMaterials);
	
	static bool SkeletalMeshToDynamicMesh_RenderData(USkeletalMesh* SkeletalMesh, FDynamicMesh3& IdenticalDynamicMesh, FDynamicMesh3* WeldedDynamicMesh = NULL, const TArray<FFinalSkinVertex>& FinalVertices = TArray<FFinalSkinVertex>(), int32 LOD = 0);
	static void CreateEmptyLODModel(FMorphTargetLODModel& LODModel);
	static UMorphTarget* FindMorphTarget(USkeletalMesh* Mesh, FString MorphTargetName);
	static void CreateMorphTargetObj(USkeletalMesh* Mesh, FString MorphTargetName, bool bInvalidateRenderData = true);
	static void GetMorphTargetDeltas(USkeletalMesh* Mesh, UMorphTarget* MorphTarget, TArray<FMorphTargetDelta>& Deltas, int32 LOD = 0);
	static void GetMorphTargetNames(USkeletalMesh* Mesh, TArray<FString>& MorphTargets);
	static void GetMorphDeltas(const FDynamicMesh3& Original, const FDynamicMesh3& Changed, TArray<FMorphTargetDelta>& Deltas);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Operations")
		static bool CopyMeshComponent(UMeshMorpherMeshComponent* Source, UMeshMorpherMeshComponent* Target);
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Operations")
		static bool SkeletalMeshToDynamicMesh(USkeletalMesh* SkeletalMesh, int32 LOD, UMeshMorpherMeshComponent* IdenticalMeshComponent, UMeshMorpherMeshComponent* WeldedMeshComponent);
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Operations")
		static bool SkeletalMeshComponentToDynamicMesh(USkeletalMeshComponent* SkeletalMeshComponent, int32 LOD, bool bUseSkinnedVertices, UMeshMorpherMeshComponent* IdenticalMeshComponent, UMeshMorpherMeshComponent* WeldedMeshComponent);
};