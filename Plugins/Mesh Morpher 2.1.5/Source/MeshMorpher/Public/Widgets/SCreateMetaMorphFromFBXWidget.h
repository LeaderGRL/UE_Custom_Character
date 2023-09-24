// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "Components/DynamicMeshComponent.h"
#include "UObject/GCObject.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Widgets/SCreateMorphTargetFromFBXWidget.h"
#include "SCreateMetaMorphFromFBXWidget.generated.h"

class FText;
class FText;
class STextBlock;
class UStandaloneMaskSelection;

using namespace UE::Geometry;

UCLASS()
class UCreateMetaMorphFromFBXProperties : public UObject
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

	UPROPERTY(EditAnywhere, Category = Masks)
		TArray< UStandaloneMaskSelection*> IgnoreMasks;
	UPROPERTY(EditAnywhere, Category = Masks)
		TArray< UStandaloneMaskSelection*> MoveMasks;

	/**
	* Whether FBX files are rendered in the scene
	*/
	UPROPERTY(EditAnywhere, Category = Rendering)
		bool RenderMeshes = true;

public:
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
	
		IgnoreMasks.Empty();
		MoveMasks.Empty();

		BaseFile = FFilePath();
		BaseTransform = FFBXTransform();

		MorphedFile = FFilePath();
		MorphedTransform = FFBXTransform();
		LockTransform = true;
	}

protected:
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};



class SCreateMetaMorphFromFBXWidget : public SCompoundWidget, public FGCObject
{

public:
	SLATE_BEGIN_ARGS(SCreateMetaMorphFromFBXWidget)
	{}
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetReferencerName() const override
	{
		return "MeshMorpherMeta" + FString::FromInt(FMath::Rand());
	}

public:
	UDynamicMeshComponent* BaseComponent = nullptr;
	UDynamicMeshComponent* MorphedComponent = nullptr;
private:
	FText GetMessage() const;
	bool IsCreateMorphValid() const;
	FReply Close();
	FReply OnCreateMorphTarget();
private:
	TSharedPtr<IDetailsView> DetailsPanel;
	UCreateMetaMorphFromFBXProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	TSharedPtr<STextBlock> ToolMessageArea;
};