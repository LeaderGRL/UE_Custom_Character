// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "MeshMorpherSettings.generated.h"

UCLASS(BlueprintType, config = MeshMorpher)
class MESHMORPHERRUNTIME_API UMeshMorpherSettings : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Update")
		bool bCheckForUpdates = true;

	/*Update interval (in seconds).*/
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "bCheckForUpdates"), Category = "Update")
		double CheckInterval = 3600.0;

	/*Connection timeout (in seconds).*/
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "bCheckForUpdates"), Category = "Update")
		double ConnectionTimeout = 15.0;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Tool")
		bool DisplayMaterialSlotsAsSections = true;
	
	/* Remap the morph targets from the base LOD onto the reduce LOD. */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "LOD")
		bool bRemapMorphTargets = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Meta = (Latent, DisplayName = "Use UV Projection for LODs"), Category = "LOD")
		bool bUseUVProjectionForLODs = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "!bUseUVProjectionForLODs"), Category = "LOD")
		double Threshold = 20.0;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "!bUseUVProjectionForLODs"), Category = "LOD")
		int32 SmoothIterations = 3;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "!bUseUVProjectionForLODs"), Category = "LOD")
		double SmoothStrength = 0.6;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Masking")
		FLinearColor MaskSelectionColor = FLinearColor::Yellow;
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Masking")
		FLinearColor MaskSelectionWireframeColor = FLinearColor::Blue;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Masking")
		float SelectionPrimitiveThickness = 2.0f;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Internal Welder")
		double MergeVertexTolerance = FMathd::ZeroTolerance;
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Internal Welder")
		double MergeSearchTolerance = 2.0 * FMathd::ZeroTolerance;
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Internal Welder")
		bool OnlyUniquePairs = false;	

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"), Category = "Indicator")
		float IndicatorThickness = 0.5f;
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Indicator")
		FLinearColor IndicatorColor = FLinearColor::Black;
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Indicator")
		bool bShowIndicatorSphere = true;
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Indicator")
		FLinearColor IndicatorSphereColor = FLinearColor(0.189237,0.968691,1.000000,1.000000);
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Indicator")
		float IndicatorSphereAlpha = 1.0f;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Wireframe")
		FLinearColor WireframeColor = FLinearColor(0, 0.5f, 1.f);

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "OBJ Export")
		bool ExportMaterialSlotsAsSections = true;

	UPROPERTY(config, BlueprintReadWrite, Category = "Camera")
		int32 CameraSpeed = 4;
	UPROPERTY(config, BlueprintReadWrite, Category = "Camera")
		float CameraSpeedScalar = 1.0f;
	UPROPERTY(config, BlueprintReadWrite, Category = "Camera")
		float CameraFOV = 90.0f;
	UPROPERTY(config, BlueprintReadWrite, Category = "Camera")
		FVector CameraPosition = FVector(500, 300, 500);
	UPROPERTY(config, BlueprintReadWrite, Category = "Camera")
		FRotator CameraRotation = UKismetMathLibrary::FindLookAtRotation(CameraPosition, FVector::ZeroVector);
};