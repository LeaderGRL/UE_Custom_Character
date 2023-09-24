// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "Components/DynamicMeshComponent.h"
#include "UObject/GCObject.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshMorpherFBXImport.h"
#include "SCreateMorphTargetFromFBXWidget.generated.h"

class FText;
class STextBlock;
class UMeshMorpherMeshComponent;
using namespace UE::Geometry;

USTRUCT(BlueprintType)
struct FFBXTransform
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Transform)
		FVector Position = FVector(0, 0, 0);	
	UPROPERTY(EditAnywhere, Category = Transform)
		FVector Rotation = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, Category = Transform)
		FVector Scale = FVector::OneVector;
};


UCLASS()
class UCreateMorphTargetFromFBXProperties : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Path to the Base File.
	*/
	UPROPERTY(EditAnywhere, meta = (FilePathFilter = "Mesh files (*.OBJ; *.FBX)|*.OBJ; *.FBX"), Category = Base)
	FFilePath BaseFile;

	/**
	* Base T0 Pose
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Base Use T0 Pose"), Category = Base)
	bool bBaseUseT0 = false;
	
	/**
	* Base Coordinate System
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Base CoordinateSystem"), Category = Base)
	EMeshMorpherFBXCoordinate BaseFrontCoordinateSystem = EMeshMorpherFBXCoordinate::RightHanded;
	
	/**
	* Base Front Axis
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Base Front Axis"), Category = Base)
	EMeshMorpherFBXAxis BaseFrontAxis = EMeshMorpherFBXAxis::X;

	/**
	* Base Up Axis
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Base Up Axis"), Category = Base)
	EMeshMorpherFBXAxis BaseUpAxis = EMeshMorpherFBXAxis::Z;	

	/**
	* Base Transform
	*/
	UPROPERTY(EditAnywhere, Category = Base)
	FFBXTransform BaseTransform;

	/**
	* Path to the Morphed File.
	*/
	UPROPERTY(EditAnywhere, meta = (FilePathFilter = "Mesh files (*.OBJ; *.FBX)|*.OBJ; *.FBX"), Category = Morphed)
	FFilePath MorphedFile;

	/**
	* Morphed T0 Pose
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Morphed Use T0 Pose"), Category = Morphed)
	bool bMorphedUseT0 = false;
	
	/**
	* Morphed Coordinate System
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Morphed CoordinateSystem"), Category = Morphed)
	EMeshMorpherFBXCoordinate MorphedFrontCoordinateSystem = EMeshMorpherFBXCoordinate::RightHanded;
	
	/**
	* Morphed Front Axis
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Morphed Front Axis"), Category = Morphed)
	EMeshMorpherFBXAxis MorphedFrontAxis = EMeshMorpherFBXAxis::X;

	/**
	* Morphed Up Axis
	*/
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Morphed Up Axis"), Category = Morphed)
	EMeshMorpherFBXAxis MorphedUpAxis = EMeshMorpherFBXAxis::Z;	
	
	/**
	* Morphed Transform
	*/
	UPROPERTY(EditAnywhere, Category = Morphed)
		FFBXTransform MorphedTransform;

	/**
	* Lock Morphed Transform to Base Transform
	*/
	UPROPERTY(EditAnywhere, Category = Morphed)
		bool LockTransform = true;

	/**
	* Target Morph Name. If it exists you can overwrite.
	*/
	UPROPERTY(EditAnywhere, Category = Target)
		FString NewMorphName = "";

	
	/**
	* Maximum Distance threshold between a vertex in the target mesh and a morphed vertex, at which it is considered to be compatible.
	*/
	UPROPERTY(EditAnywhere, Category = Options, meta = (ClampMin = "0", UIMin = "0"))
		double Threshold = 20.0;

	UPROPERTY(EditAnywhere, Category = Options)
		double NormalIncompatibilityThreshold = 0.5;	
	
	/**
	* Multiplier
	*/
	UPROPERTY(EditAnywhere, Category = Options)
		double Multiplier = 1.0;

	UPROPERTY(EditAnywhere, Category = "Options|Smoothing", meta = (ClampMin = "0", UIMin = "0"))
		int32 SmoothIterations = 3;

	UPROPERTY(EditAnywhere, Category = "Options|Smoothing", meta = (ClampMin = "0", UIMin = "0"))
		double SmoothStrength = 0.6;

	/**
	* Whether files are rendered in the scene
	*/
	UPROPERTY(EditAnywhere, Category = Rendering)
		bool RenderMeshes = true;

public:
	UMeshMorpherMeshComponent* MeshComponent = nullptr;
	UDynamicMeshComponent* BaseComponent = nullptr;
	UDynamicMeshComponent* MorphedComponent = nullptr;

public:
	FDynamicMesh3 BaseMesh;
	FDynamicMesh3 MorphedMesh;

	bool bApplyBaseTransform = false;
	bool bApplyMorphedTransform = false;

public:
	void Initialize()
	{
		BaseFile = FFilePath();
		BaseTransform = FFBXTransform();

		MorphedFile = FFilePath();
		MorphedTransform = FFBXTransform();
		LockTransform = true;

		NewMorphName = "";

		Threshold = 20.0;
		Multiplier = 1.0;
		SmoothIterations = 3;
		SmoothStrength = 0.6;
	}

protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};



class SCreateMorphTargetFromFBXWidget : public SCompoundWidget, public FGCObject
{

public:
	SLATE_BEGIN_ARGS(SCreateMorphTargetFromFBXWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Target);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetReferencerName() const override
	{
		return "MeshMorpherFBX" + FString::FromInt(FMath::Rand());
	}

public:
	UDynamicMeshComponent* BaseComponent = nullptr;
	UDynamicMeshComponent* MorphedComponent = nullptr;
	UMeshMorpherMeshComponent* MeshComponent = nullptr;
private:
	FText GetMessage() const;
	bool IsCreateMorphValid() const;
	FReply Close();
	FReply OnCreateMorphTarget();
private:
	USkeletalMesh* Target = nullptr;
	TSharedPtr<IDetailsView> DetailsPanel;
	UCreateMorphTargetFromFBXProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	TSharedPtr<STextBlock> ToolMessageArea;
};