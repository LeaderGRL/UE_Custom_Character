// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SMorphTargetListWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SSearchBox.h"
#include "MeshOperationsLibrary.h"
#include "MeshOperationsLibraryRT.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SMorphTargetListWidget"

namespace MeshMorpherMorphTargetListColumns
{
	static const FName MorphTargetNameLabel("MorphTargetName");
}

void SMorphTargetListWidget::Construct(const FArguments& InArgs)
{
	FilterText = FText::FromString("");
	Source = InArgs._Source;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	check(InArgs._Commands.IsValid());
	UICommands = InArgs._Commands;

	ChildSlot
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SSearchBox)
					.SelectAllTextWhenFocused(true)
					.OnTextChanged(this, &SMorphTargetListWidget::OnFilterTextChanged)
					.OnTextCommitted(this, &SMorphTargetListWidget::OnFilterTextCommitted)
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(MorphListView, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&MorphTargetList)
					.ItemHeight(24)
					.SelectionMode(ESelectionMode::Multi)
					.OnGenerateRow_Raw(this, &SMorphTargetListWidget::OnGenerateRowForList)
					.OnSelectionChanged_Raw(this, &SMorphTargetListWidget::MorphTargetSelectionChanged)
					.OnContextMenuOpening_Raw(this, &SMorphTargetListWidget::OnGetContextMenuContent)
					.OnMouseButtonDoubleClick_Raw(this, &SMorphTargetListWidget::ListDoubleClick)
					.HeaderRow
					(
						SNew( SHeaderRow )
						+ SHeaderRow::Column(MeshMorpherMorphTargetListColumns::MorphTargetNameLabel )
						.DefaultLabel( LOCTEXT( "MorphTargetNameLabel", "Name" ) )
					)
				]
			]
		];

	SetSource(Source);
	if (InArgs._InitialSelection.IsValid())
	{
		SetSelection(InArgs._InitialSelection);
	}
}

void SMorphTargetListWidget::Refresh()
{
	CreateMorphTargetList(FilterText.ToString());
}

void SMorphTargetListWidget::SetSelection(TSharedPtr<FString> Item)
{
	if (MorphTargetList.Contains(Item))
	{
		MorphListView->SetSelection(Item, ESelectInfo::Direct);
	}
}

void SMorphTargetListWidget::SetSource(USkeletalMesh* InSource)
{
	Source = InSource;
	CreateMorphTargetList(FilterText.ToString());
}

TSharedRef<ITableRow> SMorphTargetListWidget::OnGenerateRowForList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	//Create the row
	if (Item.IsValid())
	{
		return
			SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(*Item))
			    .Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.ListItem"))
			];
	}
	else {
		return
			SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
			[
				SNew(SBox)
			];
	}
}


void SMorphTargetListWidget::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;

	CreateMorphTargetList(SearchText.ToString());
}

void SMorphTargetListWidget::OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo)
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged(SearchText);
}

void SMorphTargetListWidget::MorphTargetSelectionChanged(TSharedPtr<FString> InItem, ESelectInfo::Type SelectInfo)
{
	TArray<TSharedPtr<FString>> Items;
	MorphListView->GetSelectedItems(Items);
	OnSelectionChanged.ExecuteIfBound(Items, true, false, false);
}

void SMorphTargetListWidget::ListDoubleClick(TSharedPtr<FString> InItem)
{
	TArray<TSharedPtr<FString>> Items;
	Items.Add(InItem);
	OnSelectionChanged.ExecuteIfBound(Items, false, true, true);
}

TSharedPtr<SWidget> SMorphTargetListWidget::OnGetContextMenuContent() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder Builder(bShouldCloseWindowAfterMenuSelection, UICommands);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().OpenMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().DeselectMorphButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().AddMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().DeleteMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().RenameMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().DuplicateMorphButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CopyMorphButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().MergeMorphsButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ExportMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ExportMorphTargetOBJMenuButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ApplyMorphTargetToLODsButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().MorphMagnitudeButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMetaMorphFromMorphTargetButton);
	return Builder.MakeWidget();
}

void SMorphTargetListWidget::CreateMorphTargetList(const FString& SearchText)
{
	MorphTargetList.Empty();
	TArray<FString> MorphTargets;
	UMeshOperationsLibrary::GetMorphTargetNames(Source, MorphTargets);

	bool bDoFiltering = !SearchText.IsEmpty();
	for (const FString& MorphTarget : MorphTargets)
	{
		if (bDoFiltering && !MorphTarget.Contains(SearchText))
		{
			continue; // Skip items that don't match our filter
		}
		MorphTargetList.Add(MakeShareable(new FString(MorphTarget)));
	}

	MorphListView->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
