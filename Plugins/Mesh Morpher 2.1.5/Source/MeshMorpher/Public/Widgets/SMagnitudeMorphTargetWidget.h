// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "MeshMorpherToolkit.h"
#include "Slate.h"

class FText;

class SMagnitudeMorphTargetWidget : public SCompoundWidget
{
	friend class FMeshMorpherToolkit;
public:
	SLATE_BEGIN_ARGS(SMagnitudeMorphTargetWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TArray<TSharedPtr<FString>>, Selection);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	TOptional<float> GetMagnitudeValue() const;
	void SetMagnitudeValue(float NewValue);
	bool IsUpdateMorphValid() const;
	FReply Close();
	FReply OnUpdateMagnitudeMorphTarget();
private:
	USkeletalMesh* Source = nullptr;
	TArray<TSharedPtr<FString>> Selection;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	TSharedPtr<SNumericEntryBox<float>> NumericBox;
	float Value = 1.0f;
};