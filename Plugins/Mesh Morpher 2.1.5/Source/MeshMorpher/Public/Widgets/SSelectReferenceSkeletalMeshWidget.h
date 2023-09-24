// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "Widgets/SMeshMorpherMorphTargetListRow.h"
#include "SSelectReferenceSkeletalMeshWidget.generated.h"

class FText;
class SSelectReferenceSkeletalMeshWidget;

UCLASS()
class USelectReferenceSkeletalMeshProperties : public UObject
{
	friend class SSelectReferenceSkeletalMeshWidget;
	GENERATED_BODY()

public:

	/**
	* Reference Skeletal Mesh.
	*/
	UPROPERTY(EditAnywhere, Category = Options, meta = (DisplayThumbnail = "true"))
		USkeletalMesh* ReferenceSkeletalMesh = nullptr;

	/**
	* Reference Skeletal Mesh Materials Override.
	*/
	UPROPERTY(EditAnywhere, Category = Options)
		TArray<UMaterialInterface*> Materials;

	/**
	* Transform of the Reference Skeletal Mesh
	*/
	UPROPERTY(EditAnywhere, Category = Options)
		FTransform Transform;

	UPROPERTY(EditAnywhere, Category = Options)
		bool bDisableMasterPose = false;
	
	UPROPERTY(EditAnywhere, meta = (HideEditConditionToggle, EditConditionHides, EditCondition = "!bDisableMasterPose"), Category = Options)
		bool bInvertMasterPose = false;

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
	SSelectReferenceSkeletalMeshWidget* Widget;
};


class SSelectReferenceSkeletalMeshWidget : public SMeshMorpherMorphTargetMainWidget
{
	friend class USelectReferenceSkeletalMeshProperties;
public:
	SLATE_BEGIN_ARGS(SSelectReferenceSkeletalMeshWidget)
	{}
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	FReply Close();
private:
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	TSharedPtr<IDetailsView> DetailsPanel;
	USelectReferenceSkeletalMeshProperties* Config = nullptr;
};