// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SStitchMorphTargetWidget.generated.h"

class FText;

UCLASS()
class UStitchMorphTargetProperties : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Target Skeletal Mesh.
	*/
	UPROPERTY(EditAnywhere, Category = Target, meta = (DisplayName = "Target(s)"))
		USkeletalMesh* Target;

	/**
	* Target Morph Name. If it exists you can overwrite.
	*/
	UPROPERTY(EditAnywhere, Category = Target)
		FString TargetMorphTarget = "";


	/**
	* Maximum Distance threshold between a vertex in the target mesh and a morphed vertex, at which it is considered to be compatible.
	*/
	UPROPERTY(EditAnywhere, Category = Options, meta = (ClampMin = "0", UIMin = "0"))
		double Threshold = 0.1;

	void Initialize()
	{
		Target = nullptr;
		TargetMorphTarget = "";
	}
};


class SStitchMorphTargetWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SStitchMorphTargetWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TSharedPtr<FString>, MorphTarget);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsStitchMorphValid() const;
	FReply Close();
	FReply OnStitchMorphTarget();
private:
	USkeletalMesh* Source = nullptr;
	TSharedPtr<FString> MorphTarget;
	TSharedPtr<IDetailsView> DetailsPanel;
	UStitchMorphTargetProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};