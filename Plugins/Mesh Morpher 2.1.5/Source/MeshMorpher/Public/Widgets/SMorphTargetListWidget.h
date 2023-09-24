// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Engine/SkeletalMesh.h"
#include "MeshMorpherCommands.h"

DECLARE_DELEGATE_FourParams(FOnMorphTargetSelectionChanged, const TArray<TSharedPtr<FString>> &, bool, bool, bool);

class SMorphTargetListWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SMorphTargetListWidget)
	{}
	SLATE_EVENT(FOnMorphTargetSelectionChanged, OnSelectionChanged);
	SLATE_ARGUMENT(USkeletalMesh*, Source);
	SLATE_ARGUMENT(TSharedPtr<FString>, InitialSelection);
	SLATE_ARGUMENT(TSharedPtr<class FUICommandList>, Commands);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	void Refresh();
	void SetSelection(TSharedPtr<FString> Item);
	void SetSource(USkeletalMesh* InSource);
private:
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnFilterTextChanged(const FText& SearchText);
	void OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo);
	void MorphTargetSelectionChanged(TSharedPtr<FString> InItem, ESelectInfo::Type SelectInfo);
	void ListDoubleClick(TSharedPtr<FString> InItem);
	TSharedPtr<SWidget> OnGetContextMenuContent() const;
	void CreateMorphTargetList(const FString& SearchText);

public:
	TSharedPtr< SListView< TSharedPtr<FString> > > MorphListView;

private:
	USkeletalMesh* Source = nullptr;
	FOnMorphTargetSelectionChanged OnSelectionChanged;
	TArray<TSharedPtr<FString>> MorphTargetList;
	FText FilterText;
	TSharedPtr<class FUICommandList> UICommands;
};