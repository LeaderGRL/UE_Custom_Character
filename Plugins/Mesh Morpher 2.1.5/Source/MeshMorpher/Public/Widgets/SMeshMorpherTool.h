// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "IDetailsView.h"
#include "MeshMorpherEdMode.h"

class FText;
class STextBlock;
class SBorder;

class SMeshMorpherToolWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SMeshMorpherToolWidget)
	{}
	SLATE_ARGUMENT(UMeshMorpherEdMode*, EdMode);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	~SMeshMorpherToolWidget();
private:
	void PostNotification(const FText& Message);
	void ClearNotification();
	void PostWarning(const FText& Message);
	void ClearWarning();
private:
	UMeshMorpherEdMode* EdMode = nullptr;
	TSharedPtr<IDetailsView> DetailsPanel;
	TSharedPtr<SBorder> Content;
	TSharedPtr<STextBlock> ToolHeaderLabel;
	TSharedPtr<STextBlock> ToolMessageArea;
	TSharedPtr<STextBlock> ToolWarningArea;
};