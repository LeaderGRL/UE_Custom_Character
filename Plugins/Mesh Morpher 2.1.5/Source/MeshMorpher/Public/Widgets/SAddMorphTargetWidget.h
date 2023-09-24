// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/SkeletalMesh.h"
#include "MeshMorpherToolkit.h"
class FText;
class SEditableTextBox;
class SAddMorphTargetWidget : public SCompoundWidget
{
	friend class FMeshMorpherToolkit;
public:
	SLATE_BEGIN_ARGS(SAddMorphTargetWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh* , Source);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	void HandleNewMorphNameChanged(const FText& NewText);
	bool IsCreateMorphValid() const;
	FReply Close();
	FReply OnCreateMorphTarget();
private:
	USkeletalMesh* Source = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	TSharedPtr<SEditableTextBox> TextBox;
	FText NewMorphName;
};