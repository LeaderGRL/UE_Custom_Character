// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Engine/SkeletalMesh.h"
#include "MeshMorpherToolkit.h"
#include "Widgets/SMeshMorpherMorphTargetListRow.h"

class FText;
class USkeletalMeshComponent;

class SBakeSkeletalMeshWidget : public SMeshMorpherMorphTargetMainWidget
{

public:
	SLATE_BEGIN_ARGS(SBakeSkeletalMeshWidget)
	{}
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TSharedPtr<FMeshMorpherToolkit>, Toolkit);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
private:
	FReply Close();
	bool IsBakeValid() const;
	FReply OnBake();
private:
	USkeletalMesh* Source = nullptr;
	TSharedPtr<FMeshMorpherToolkit> Toolkit;
	USkeletalMesh* OriginalSource = nullptr;
};