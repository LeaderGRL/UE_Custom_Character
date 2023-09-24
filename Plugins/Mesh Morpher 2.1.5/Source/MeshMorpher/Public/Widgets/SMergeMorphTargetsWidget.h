// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "MeshMorpherToolkit.h"
class FText;

class SMergeMorphTargetsWidget : public SCompoundWidget
{
	friend class FMeshMorpherToolkit;
public:
	SLATE_BEGIN_ARGS(SMergeMorphTargetsWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TArray<TSharedPtr<FString>>, Selection);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	void SetDeleteSources(ECheckBoxState newState);
	ECheckBoxState IsDeleteSources() const;
	void HandleNewMorphNameChanged(const FText& NewText);
	bool IsMergeMorphValid() const;
	FReply Close();
	FReply OnMergeMorphTarget();
private:
	USkeletalMesh* Source = nullptr;
	TArray<TSharedPtr<FString>> Selection;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	TSharedPtr<SEditableTextBox> TextBox;
	FText NewMorphName;
	bool bDeleteSources = true;
};