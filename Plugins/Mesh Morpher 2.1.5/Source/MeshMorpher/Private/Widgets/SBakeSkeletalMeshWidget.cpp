// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SBakeSkeletalMeshWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"

#include "Preview/SPreviewViewport.h"
#include "AdvancedPreviewScene.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/RendererSettings.h"
#include "Animation/AnimInstance.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SBakeSkeletalMeshWidget"

//////////////////////////////////////////////////////////////////////////
// SBakeMorphTargetListRow

typedef TSharedPtr< FMeshMorpherMorphTargetInfo > FMeshMorpherMorphTargetInfoPtr;

void SBakeSkeletalMeshWidget::Construct(const FArguments& InArgs)
{
	check(InArgs._Source);
	OriginalSource = InArgs._Source;
	Toolkit = InArgs._Toolkit;
	TSharedPtr<class SMeshMorpherPreviewViewport> PreviewViewport;
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
			.FillHeight(1.f)
			[
				SNew(SSplitter)
				.Orientation(EOrientation::Orient_Horizontal)
				+ SSplitter::Slot()
				[
					SNew(SBorder)
					.BorderImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.MainBackgroundColor"))
					.Padding(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetMargin("MeshMorpher.GenericMargin"))
					[

						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 2)
						[
							SNew(SSearchBox)
							.SelectAllTextWhenFocused(true)
							.OnTextChanged(this, &SBakeSkeletalMeshWidget::OnFilterTextChanged)
							.OnTextCommitted(this, &SBakeSkeletalMeshWidget::OnFilterTextCommitted)
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SAssignNew(MorphListView, SMeshMorpherMorphTargetListType)
							.ListItemsSource(&MorphTargetList)
							.ItemHeight(24)
							.SelectionMode(ESelectionMode::None)
							.OnGenerateRow_Raw(this, &SBakeSkeletalMeshWidget::OnGenerateRowForList)
							.HeaderRow
							(
								SNew( SHeaderRow )
								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetNameLabel )
								.DefaultLabel( LOCTEXT( "MorphTargetNameLabel", "Name" ) )

								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetWeightLabel )
								.DefaultLabel( LOCTEXT( "MorphTargetWeightLabel", "Weight" ) )

								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetRemoveLabel)
								.DefaultLabel(LOCTEXT("MorphTargetRemoveLabel", "Remove On Bake"))

								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetVertCountLabel )
								.DefaultLabel( LOCTEXT("MorphTargetVertCountLabel", "Vert Count") )
							)
						]
					]
				]
				+ SSplitter::Slot()
				[
					SNew(SBorder)
					.BorderImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.MainBackgroundColor"))
					.Padding(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetMargin("MeshMorpher.GenericMargin"))
					[
						SAssignNew(PreviewViewport, SMeshMorpherPreviewViewport)
					]
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(EHorizontalAlignment::HAlign_Right)
			.Padding(0.f, 20.f, 0.f, 0.f)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
					.WidthOverride(90.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Raw(this, &SBakeSkeletalMeshWidget::Close)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
							.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
						]
					]
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
					.WidthOverride(70.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Raw(this, &SBakeSkeletalMeshWidget::OnBake)
						.IsEnabled_Raw(this, &SBakeSkeletalMeshWidget::IsBakeValid)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Bake")))
							.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
						]
					]
				]
			]
		]
	];

	SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(PreviewViewport->PreviewScene->GetWorld(), NAME_None, RF_Transient);
	ReferencedObjects.Add(SkeletalMeshComponent);
	Source = DuplicateObject(OriginalSource, SkeletalMeshComponent);
	ReferencedObjects.Add(Source);
	check(Source);
	SkeletalMeshComponent->SetSkeletalMesh(Source);

	PreviewViewport->AddComponent(SkeletalMeshComponent);
	InitializeMorphTargetList();
	MorphTargetList = FullMorphTargetList;
}

FReply SBakeSkeletalMeshWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

bool SBakeSkeletalMeshWidget::IsBakeValid() const
{
	if (Source && OriginalSource)
	{
		return true;
	}
	return false;
}

FReply SBakeSkeletalMeshWidget::OnBake()
{
	if (IsBakeValid())
	{
		const bool bBakeResult = UMeshOperationsLibrary::ApplyMorphTargetsToSkeletalMesh(OriginalSource, FullMorphTargetList);
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
		if (bBakeResult)
		{
			if (Toolkit.IsValid())
			{
				Toolkit->RefreshMorphList();
				Toolkit->OnAssetChanged(OriginalSource);
			}

			for (const TSharedPtr<FMeshMorpherMorphTargetInfo>& MorphTarget : FullMorphTargetList)
			{
				if (MorphTarget.IsValid() && MorphTarget->bRemove)
				{
					if (Toolkit->SelectedMorphTarget.IsValid() && MorphTarget->Name.ToString().Equals(*Toolkit->SelectedMorphTarget, ESearchCase::IgnoreCase))
					{
						Toolkit->CancelTool();
						Toolkit->SelectedMorphTarget = nullptr;
					}
				}
			}

			UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Baking: %s was successful."), *OriginalSource->GetName()));
			ParentWindow->BringToFront();
			return Close();
		}
		else {
			UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Baking: %s failed."), *OriginalSource->GetName()));
			ParentWindow->BringToFront();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION