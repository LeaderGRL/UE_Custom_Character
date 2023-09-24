// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "PreviewMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkeletalMesh.h"
#include "MeshMorpherToolHelper.generated.h"

using namespace UE::Geometry;

class UMeshMorpherMeshComponent;

USTRUCT(BlueprintType)
struct MESHMORPHERRUNTIME_API FBrushPosition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Brush Position")
		FVector Location;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Brush Position")
		FVector Normal;
public:
	FBrushPosition()
	{
		Location = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
	}

	FBrushPosition(const FVector& InLocation, const FVector& InNormal)
	{
		Location = InLocation;
		Normal = InNormal;
	}
};

UCLASS(BlueprintType)
class MESHMORPHERRUNTIME_API UIndicatorMesh : public UPreviewMesh
{
	GENERATED_BODY()
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Indicator")
		void Update(float Size, const FVector& Position, const FVector& Normal);
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Indicator")
		void UpdateMaterial(UMaterialInterface* Material);
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Indicator")
		void UpdateVisibility(const bool Visibility);
	void BeginDestroy() override;
};

UCLASS()
class MESHMORPHERRUNTIME_API UMeshMorpherToolHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Brush")
		static double CalculateBrushFalloff(double Distance, double BrushSize, double FalloffAmount);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Brush")
		static void GetBrushPositionOnPlane(const FVector& RayOrigin, const FVector& RayDirection, const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static bool FindTriangleIntersectionPoint(const FVector& RayOrigin, const FVector& RayDirection, const FVector& A, const FVector& B, const FVector& C, double& Distance);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void FindBestAxisVectors(const FVector& Normal, FVector& Axis1, FVector& Axis2);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static FVector ProjectOnPlane(const FVector& PlaneOrigin, const FVector& PlaneNormal, const FVector& Position, int32 Axis);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void GetPlaneX(const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& Out);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void GetPlaneY(const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& Out);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void GetPlaneZ(const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& Out);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static double GetNormalizedLength(const FVector& Vector);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void LerpVector(const FVector& A, const FVector& B, double Alpha, FVector& ReturnValue);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (BrushPosition)", CompactNodeTitle = "=="), Category = "Mesh Morpher|Brush Position")
		static bool BrushPositionEqualsBrushPosition(const FBrushPosition& A, const FBrushPosition& B);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Materials"), Category = "Mesh Morpher|Tools")
		static void ExportMeshComponentToOBJFile(UMeshMorpherMeshComponent* MeshComponent, const TSet<int32>& SelectedTriangles, FString FilePath, bool bInvert, const TArray<FSkeletalMaterial>& Materials);

	static void ExportMeshToOBJFile(FDynamicMesh3* Mesh, const TSet<int32>& SelectedTriangles, FString FilePath, bool bInvert, const TArray<FSkeletalMaterial>& Materials = TArray<FSkeletalMaterial>());

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Misc")
		static void GetBoundaryVertices(UMeshMorpherMeshComponent* MeshComponent, TSet<int32>& Vertices);
	
	UFUNCTION(BlueprintCallable, Meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"), Category = "Mesh Morpher|Gizmo")
		static bool MakeSphereBrushGizmo(UObject* WorldContextObject, UMaterialInterface* Material, UIndicatorMesh*& Gizmo, int32 Resolution);
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Gizmo")
		static void DestroyBrushGizmo(UIndicatorMesh* Gizmo);
};