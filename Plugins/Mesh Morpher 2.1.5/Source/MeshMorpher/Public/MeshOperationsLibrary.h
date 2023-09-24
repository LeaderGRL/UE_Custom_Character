// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshDescription.h"
#include "Animation/MorphTarget.h"
#include "Engine/StaticMesh.h"
#include "SkeletalRenderPublic.h"
#include "Widgets/SBakeSkeletalMeshWidget.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/EngineTypes.h"
#include "MeshOperationsLibrary.generated.h"

using namespace UE::Geometry;

class USkeletalMesh;
struct FMeshDescription;
class UStandaloneMaskSelection;
class UMetaMorph;

USTRUCT(BlueprintType)
struct MESHMORPHER_API FMeshMorpherSolidifyOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = Options)
	int GridResolution = 128;

	UPROPERTY(BlueprintReadWrite, Category = Options)
	float WindingThreshold = 0.5;

	UPROPERTY(BlueprintReadWrite, Category = Options)
	bool bSolidAtBoundaries = true;

	UPROPERTY(BlueprintReadWrite, Category = Options)
	float ExtendBounds = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = Options)
	int SurfaceSearchSteps = 3;
};


UCLASS()
class MESHMORPHER_API UMeshOperationsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static void NotifyMessage(const FString& Message);
	static bool CopySkeletalMeshMaterialsToStaticMesh(USkeletalMesh* SkeletalMesh, UStaticMesh* StaticMesh);
	static bool CopySkeletalMeshMaterials(USkeletalMesh* SkeletalMesh, TArray<FStaticMaterial>& StaticMaterials);
	static bool CopySkeletalMeshMaterials(USkeletalMesh* SkeletalMesh, TMap<int32, FName>& StaticMaterials);
	static bool CopySkeletalMeshMaterials(USkeletalMesh* SkeletalMesh, TMap<FName, int32>& StaticMaterials);
	static bool SetStaticMesh(UStaticMesh* StaticMesh, const FMeshDescription& MeshDescription);
	static bool SkeletalMeshToDynamicMesh(USkeletalMesh* SkeletalMesh, FDynamicMesh3& IdenticalDynamicMesh, FDynamicMesh3* WeldedDynamicMesh = NULL, const TArray<FFinalSkinVertex>& FinalVertices = TArray<FFinalSkinVertex>(), int32 LOD = 0, bool bUseRenderData = true);
	static void ApplyDeltasToDynamicMesh(const TArray<FMorphTargetDelta>& Deltas, FDynamicMesh3& DynamicMesh);
	static void ApplyDeltasToDynamicMesh(FDynamicMesh3& SourceDynamicMesh, const TArray<FMorphTargetDelta>& Deltas, FDynamicMesh3& DynamicMesh);
	static void ApplyChangesToMorphTarget(USkeletalMesh* Mesh, const FDynamicMesh3& DynamicMesh, FString MorphTargetName, const FDynamicMesh3& Original, const FDynamicMesh3& Changed);
	static void GetMorphTargetNames(USkeletalMesh* Mesh, TArray<FString>& MorphTargets);
	static void RenameMorphTargetInImportData(USkeletalMesh* Mesh, FString NewName, FString OriginalName, bool bInvalidateRenderData = true);
	static void RemoveMorphTargetsFromImportData(USkeletalMesh* Mesh, const TArray<FString>& MorphTargets, bool bInvalidateRenderData = true);
	static void ApplyMorphTargetToImportData(USkeletalMesh* Mesh, FString MorphTargetName, const TArray<FMorphTargetDelta>& Deltas, bool bInvalidateRenderData = true);
	static void ApplyMorphTargetToImportData(USkeletalMesh* Mesh, FString MorphTargetName, const TArray<FMorphTargetDelta>& Deltas, int32 LOD);
	static void GetMorphTargetDeltas(USkeletalMesh* Mesh, FString MorphTargetName, TArray<FMorphTargetDelta>& Deltas, int32 LOD = 0);
	static void SetEnableBuildData(USkeletalMesh* Mesh, bool NewValue);
	static void ApplyMorphTargetToLODs(USkeletalMesh* Mesh, FString MorphTargetName, const TArray<FMorphTargetDelta>& Deltas);
	static bool ApplyMorphTargetsToSkeletalMesh(USkeletalMesh* SkeletalMesh, const TArray< TSharedPtr<FMeshMorpherMorphTargetInfo> >& MorphTargets);
	static void SaveSkeletalMesh(USkeletalMesh* Mesh);
	static void ExportMorphTargetToStaticMesh(FString MorphTargetName, const FMeshDescription& MeshDescription, const TArray<FStaticMaterial>& StaticMaterials);
	static bool ApplyMorphTargetToLOD(USkeletalMesh* SkeletalMesh, const TArray<FMorphTargetDelta>& Deltas, int32 SourceLOD, int32 DestinationLOD, TArray<FMorphTargetDelta>& OutDeltas);
	static void CreateMorphTarget(USkeletalMesh* Mesh, FString MorphTargetName);
	static bool MergeMorphTargets(USkeletalMesh* SkeletalMesh, const TArray<FString>& MorphTargets, TArray<FMorphTargetDelta>& OutDeltas);
	static bool SetMorphTargetMagnitude(USkeletalMesh* SkeletalMesh, const FString& MorphTarget, const double& Magnitude, TArray<FMorphTargetDelta>& OutDeltas);
	static bool CopyMorphTarget(USkeletalMesh* SkeletalMesh, const TArray<FString>& MorphTargets, USkeletalMesh* TargetSkeletalMesh, TArray<TArray<FMorphTargetDelta>>& OutDeltas, double Threshold = 20.0, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 0.8);
	static void ApplySourceDeltasToDynamicMesh(const FDynamicMesh3& SourceDynamicMesh, const FDynamicMesh3& DynamicMesh, const TArray<FMorphTargetDelta>& SourceDeltas, const TSet<int32>& IgnoreVertices, TArray<FMorphTargetDelta>& OutDeltas, double Threshold = 0.0001, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 1.0, bool bCheckIdentical = true);
	static bool CreateMorphTargetFromPose(USkeletalMesh* SkeletalMesh, const FDynamicMesh3& PoseDynamicMesh, TArray<FMorphTargetDelta>& OutDeltas);
	static bool CreateMorphTargetFromMesh(USkeletalMesh* SkeletalMesh, USkeletalMesh* SourceSkeletalMesh, TArray<FMorphTargetDelta>& OutDeltas, double Threshold = 20.0, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 0.8);
	static int32 CreateMorphTargetFromDynamicMeshes(USkeletalMesh* SkeletalMesh, const FDynamicMesh3& BaseMesh, const FDynamicMesh3& MorphedMesh, TArray<FMorphTargetDelta>& OutDeltas, double Threshold = 1.0, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 1.0);
	static int32 CreateMorphTargetFromDynamicMeshes(const FDynamicMesh3& DynamicMesh, const FDynamicMesh3& BaseMesh, const FDynamicMesh3& MorphedMesh, TArray<FMorphTargetDelta>& OutDeltas, double Threshold = 1.0, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 1.0);
	static bool CreateMorphTargetsFromMetaMorph(USkeletalMesh* SkeletalMesh, const TArray<UMetaMorph*>& MetaMorphs, TMap<FName, TMap<int32, FMorphTargetDelta>>& OutDeltas, double Threshold = 1.0, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 1.0, bool bMergeMoveDeltasToMorphTargets = true);
	static bool CreateMorphTargetsFromMetaMorph(const FDynamicMesh3& DynamicMesh, const FDynamicMesh3& WeldedDynamicMesh, const TArray<UMetaMorph*>& MetaMorphs, TMap<FName, TMap<int32, FMorphTargetDelta>>& OutDeltas, double Threshold = 1.0, double NormalIncompatibilityThreshold = 0.5, double Multiplier = 1.0, int32 SmoothIterations = 0, double SmoothStrength = 1.0, bool bMergeMoveDeltasToMorphTargets = true);
	static bool CreateMetaMorphAssetFromMorphTargets(USkeletalMesh* SkeletalMesh, const TArray<FString>& MorphTargets, const TArray<UStandaloneMaskSelection*>& IgnoreMasks, const TArray<UStandaloneMaskSelection*>& MoveMasks);
	static bool CreateMetaMorphAssetFromDynamicMeshes(const FDynamicMesh3& BaseMesh, const FDynamicMesh3& MorphedMesh, FString MorphName, const TArray< UStandaloneMaskSelection*>& IgnoreMasks = TArray< UStandaloneMaskSelection*>(), const TArray< UStandaloneMaskSelection*>& MoveMasks = TArray< UStandaloneMaskSelection*>());
	static bool AppenedMeshes(USkeletalMesh* SkeletalMesh, TArray<USkeletalMesh*> AdditionalSkeletalMeshes, const bool bWeldMesh, double MergeVertexTolerance, double MergeSearchTolerance, bool OnlyUniquePairs, bool bCreateAdditionalMeshesGroups, FDynamicMesh3& Output);
	static bool SkeletalMeshToSolidDynamicMesh(USkeletalMesh* SkeletalMesh, const FMeshMorpherSolidifyOptions& Options, FDynamicMesh3& OutMesh, const uint32 SubdivisionSteps = 0, const bool bWeldMesh = false, const double MergeVertexTolerance = 0.0, const double MergeSearchTolerance = 0.0, const bool OnlyUniquePairs = false);
	static bool ConvertDynamicMeshToSolidDynamicMesh(FDynamicMesh3& DynamicMesh, const FMeshMorpherSolidifyOptions& Options, const uint32 SubdivisionSteps = 0, const bool bWeldMesh = false, const double MergeVertexTolerance = 0.0, const double MergeSearchTolerance = 0.0, const bool OnlyUniquePairs = false);
	static bool MorphMesh(USkeletalMesh* SourceSkeletalMesh, USkeletalMesh* TargetSkeletalMesh, TArray<FMorphTargetDelta>& OutDeltas);
};