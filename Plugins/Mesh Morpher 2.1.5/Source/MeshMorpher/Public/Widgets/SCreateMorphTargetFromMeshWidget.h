// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SCreateMorphTargetFromMeshWidget.generated.h"

class FText;

UCLASS()
class UCreateMorphTargetFromMeshProperties : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Source Skeletal Mesh.
	*/
	UPROPERTY(EditAnywhere, Category = Source, meta = (DisplayThumbnail = "true"))
		USkeletalMesh* Source = nullptr;

	/**
	* Target Morph Name. If it exists you can overwrite.
	*/
	UPROPERTY(EditAnywhere, Category = Target)
		FString NewMorphName = "";

	
	/**
	* Maximum Distance threshold between a vertex in the target mesh and a morphed vertex, at which it is considered to be compatible.
	* This is used only when the LOD used to generate the deltas is higher than 0.
	*/
	UPROPERTY(EditAnywhere, Category = Options, meta = (ClampMin = "0", UIMin = "0"))
		double Threshold = 20.0;

	UPROPERTY(EditAnywhere, Category = Options)
		double NormalIncompatibilityThreshold = 0.5;
	
	/**
	* This is used only when the LOD used to generate the deltas is higher than 0.
	*/
	UPROPERTY(EditAnywhere, Category = Smoothing, meta = (ClampMin = "0", UIMin = "0"))
		int32 SmoothIterations = 3;

	/**
	* This is used only when the LOD used to generate the deltas is higher than 0.
	*/
	UPROPERTY(EditAnywhere, Category = Smoothing, meta = (ClampMin = "0", UIMin = "0"))
		double SmoothStrength = 0.6;


	void Initialize()
	{
		NewMorphName = "";
	}
};



class SCreateMorphTargetFromMeshWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SCreateMorphTargetFromMeshWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Target);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsCreateMorphValid() const;
	FReply Close();
	FReply OnCreateMorphTarget();
private:
	USkeletalMesh* Target = nullptr;
	TSharedPtr<IDetailsView> DetailsPanel;
	UCreateMorphTargetFromMeshProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};