// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SCopyMorphTargetWidget.generated.h"

class FText;

UCLASS()
class UCopyMorphTargetProperties : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Target Skeletal Mesh.
	*/
	UPROPERTY(EditAnywhere, Category = Target, meta = (DisplayName = "Target(s)"))
		TArray<USkeletalMesh*> Targets;

	UPROPERTY()
		bool bMultipleSelection = false;

	/**
	* Target Morph Name. If it exists you can overwrite.
	*/
	UPROPERTY(EditAnywhere, Category = Target, meta = (HideEditConditionToggle, EditConditionHides, EditCondition = "bMultipleSelection"))
		FString NewMorphName = "";

	/**
	* If selected, Morph Targets on target Skeletal Mesh will automatically be overwritten without asking.
	*/
	UPROPERTY(EditAnywhere, Category = Target)
		bool bAutomaticallyOverwrite = false;

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

	UPROPERTY(EditAnywhere, Category = Smoothing, meta = (ClampMin = "0", UIMin = "0"))
		int32 SmoothIterations = 3;

	UPROPERTY(EditAnywhere, Category = Smoothing, meta = (ClampMin = "0", UIMin = "0"))
		double SmoothStrength = 0.6;



	void Initialize()
	{
		Targets.Empty();
		NewMorphName = "";
	}
};


class SCopyMorphTargetWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SCopyMorphTargetWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TArray<TSharedPtr<FString>>, Selection);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsCopyMorphValid() const;
	FReply Close();
	FReply OnCopyMorphTarget();
private:
	USkeletalMesh* Source = nullptr;
	TArray<TSharedPtr<FString>> Selection;
	TSharedPtr<IDetailsView> DetailsPanel;
	UCopyMorphTargetProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};