// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "IDetailsView.h"
#include "MeshMorpherToolkit.h"
#include "SOpenMaskSelectionWidget.generated.h"

class FText;
class UStandaloneMaskSelection;
class UUserWidget;

UCLASS()
class UOpenMaskSelectionProperties : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = Target, meta = (DisplayName = "Target(s)"))
		UStandaloneMaskSelection* Selection;

	UPROPERTY(EditAnywhere, Category = Options)
		bool bAppendSelection = false;

	void Initialize()
	{
		Selection = nullptr;
	}
};


class SOpenMaskSelectionWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SOpenMaskSelectionWidget)
	{}
	SLATE_ARGUMENT(UUserWidget*, Tool);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	bool IsOpenSelectionValid() const;
	FReply Close();
	FReply OnOpenSelectionTarget();
private:
	UUserWidget* Tool = nullptr;
	TSharedPtr<IDetailsView> DetailsPanel;
	UOpenMaskSelectionProperties* Config = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
};