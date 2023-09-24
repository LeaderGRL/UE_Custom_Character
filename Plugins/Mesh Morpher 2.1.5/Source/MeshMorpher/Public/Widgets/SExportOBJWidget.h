// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SExportOBJWidget.generated.h"

class FText;

UCLASS()
class UExportOBJProperties : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Additional Skeletal Meshes to Append.
	*/
	UPROPERTY(EditAnywhere, Category = Append, meta = (DisplayName = "Additional Meshes"))
		TArray<USkeletalMesh*> AdditionalSkeletalMeshes;

	/**
	* Save as a welded mesh
	*/
	UPROPERTY(EditAnywhere, Category = Welding)
		bool bExportWeldedMesh = false;

	/**
	* Edges are coincident if both pairs of endpoint vertices are closer than this distance
	*/
	UPROPERTY(EditAnywhere, Category = Welding, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "bExportWeldedMesh"))
		double MergeVertexTolerance = 0.001;

	/**
	* Edges are considered as potentially the same if their midpoints are within this distance.
	* Due to floating-point roundoff this should be larger than MergeVertexTolerance.
	*/
	UPROPERTY(EditAnywhere, Category = Welding, meta = (ClampMin = "0", UIMin = "0", HideEditConditionToggle, EditConditionHides, EditCondition = "bExportWeldedMesh"))
		double MergeSearchTolerance = 2.0 * FMathd::ZeroTolerance;
	
	/**
	* Only merge unambiguous pairs that have unique duplicate-edge matches 
	*/
	UPROPERTY(EditAnywhere, Category = Welding, meta = (HideEditConditionToggle, EditConditionHides, EditCondition = "bExportWeldedMesh"))
		bool bMergeOnlyUniquePairs = false;
		
    /**
    * Each mesh section in additional meshes are created in a separate group
	*/
    UPROPERTY(EditAnywhere, Category = Options)
        bool bCreateAdditionalMeshesGroups = false;
	
	void Initialize()
	{
		AdditionalSkeletalMeshes.Empty();
	}
};


class SExportOBJWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SExportOBJWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsExportOBJValid() const;
	FReply Close();
	FReply OnExportOBJ();
private:
	USkeletalMesh* Source = nullptr;
	TSharedPtr<IDetailsView> DetailsPanel;
	UExportOBJProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};