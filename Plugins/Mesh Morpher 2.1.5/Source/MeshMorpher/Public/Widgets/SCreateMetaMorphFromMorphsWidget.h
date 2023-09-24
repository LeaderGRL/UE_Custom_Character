// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SCreateMetaMorphFromMorphsWidget.generated.h"

class FText;
class UStandaloneMaskSelection;


UCLASS()
class UCreateMetaMorphFromMorphsProperties : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = Masks)
		TArray< UStandaloneMaskSelection*> IgnoreMasks;
	UPROPERTY(EditAnywhere, Category = Masks)
		TArray< UStandaloneMaskSelection*> MoveMasks;

	void Initialize()
	{
		IgnoreMasks.Empty();
		MoveMasks.Empty();
	}
};



class SCreateMetaMorphFromMorphsWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SCreateMetaMorphFromMorphsWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TArray<TSharedPtr<FString>>, Selection);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsCreateMorphValid() const;
	FReply Close();
	FReply OnCreateMorphTarget();
private:
	TSharedPtr<IDetailsView> DetailsPanel;
	USkeletalMesh* Source = nullptr;
	TArray<TSharedPtr<FString>> Selection;
	UCreateMetaMorphFromMorphsProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};