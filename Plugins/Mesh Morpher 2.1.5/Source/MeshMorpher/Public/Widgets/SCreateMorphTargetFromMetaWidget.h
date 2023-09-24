// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SCreateMorphTargetFromMetaWidget.generated.h"

class FText;
class UMetaMorph;

UCLASS()
class UCreateMorphTargetFromMetaProperties : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Target Skeletal Mesh.
	*/
	UPROPERTY(EditAnywhere, Category = Source, meta = (DisplayThumbnail = "true", HideEditConditionToggle, EditConditionHides, EditCondition = "!bNoTarget"))
		TSet<USkeletalMesh*> Targets;

	/**
	* Source Meta Morph ASssets.
	*/
	UPROPERTY(EditAnywhere, Category = Source, meta = (DisplayThumbnail = "true"))
		TSet<UMetaMorph*> Sources;


	/**
	* Merges Moved Mask Selection with created Morph Targets.
	*/
	UPROPERTY(EditAnywhere, Category = Source, meta = (ClampMin = "0", UIMin = "0"))
		bool bMergeMoveDeltasToMorphTargets = true;

	UPROPERTY(EditAnywhere, Category = Options, meta = (ClampMin = "0", UIMin = "0"))
		double Threshold = 20.0;

	UPROPERTY(EditAnywhere, Category = Options)
		double NormalIncompatibilityThreshold = 0.5;		

	UPROPERTY(EditAnywhere, Category = Smoothing, meta = (ClampMin = "0", UIMin = "0"))
		int32 SmoothIterations = 3;

	UPROPERTY(EditAnywhere, Category = Smoothing, meta = (ClampMin = "0", UIMin = "0"))
		double SmoothStrength = 0.6;

	UPROPERTY(transient)
		bool bNoTarget = false;


	void Initialize()
	{
		Targets.Empty();
		Sources.Empty();
		bNoTarget = false;
	}
};



class SCreateMorphTargetFromMetaWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SCreateMorphTargetFromMetaWidget)
	{}
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_ARGUMENT(bool, NoTarget);
	SLATE_ARGUMENT(TSet<USkeletalMesh*>, Targets);
	SLATE_ARGUMENT(TSet<UMetaMorph*>, Sources);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsCreateMorphValid() const;
	FReply Close();
	FReply OnCreateMorphTarget();
private:
	TSharedPtr<IDetailsView> DetailsPanel;
	UCreateMorphTargetFromMetaProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};