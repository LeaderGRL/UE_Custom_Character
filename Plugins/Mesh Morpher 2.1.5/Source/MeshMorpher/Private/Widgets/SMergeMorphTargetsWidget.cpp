// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SMergeMorphTargetsWidget.h"
#include "SlateOptMacros.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"
#include "MeshOperationsLibraryRT.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/MessageDialog.h"
#include "Misc/FeedbackContext.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SMergeMorphTargetsWidget"

void SMergeMorphTargetsWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
	Selection = InArgs._Selection;
	Toolkit = InArgs._Toolkit;
	check(Source);
	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.MainBackgroundColor"))
			.Padding(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetMargin("MeshMorpher.GenericMargin"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Destination Morph Target Name:")))
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.OptionsLabel"))
				]
				+ SVerticalBox::Slot()
				.Padding(0.f, 5.f)
				.AutoHeight()
				[
					SAssignNew(TextBox, SEditableTextBox)
					.MinDesiredWidth(500.f)
					.OnTextChanged_Raw(this, &SMergeMorphTargetsWidget::HandleNewMorphNameChanged)
					.Text(NewMorphName)
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
				]
				+ SVerticalBox::Slot()
				.Padding(0.f, 5.f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.AutoWidth()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Raw(this, &SMergeMorphTargetsWidget::SetDeleteSources)
						.IsChecked_Raw(this, &SMergeMorphTargetsWidget::IsDeleteSources)
					]
					+ SHorizontalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.AutoWidth()
					.Padding(0.f, 5.f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Delete source Morph Targets on Merge")))
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
							.OnClicked_Raw(this, &SMergeMorphTargetsWidget::Close)
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
							.OnClicked_Raw(this, &SMergeMorphTargetsWidget::OnMergeMorphTarget)
							.IsEnabled_Raw(this, &SMergeMorphTargetsWidget::IsMergeMorphValid)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Ok")))
								.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
							]
						]
					]
				]
			]
		]
	];
}

void SMergeMorphTargetsWidget::SetDeleteSources(ECheckBoxState newState)
{
	if (newState == ECheckBoxState::Checked)
	{
		bDeleteSources = true;
	}
	else {
		bDeleteSources = false;
	}
}

ECheckBoxState SMergeMorphTargetsWidget::IsDeleteSources() const 
{
	if (bDeleteSources)
	{
		return ECheckBoxState::Checked;
	}
	return ECheckBoxState::Unchecked;
}

void SMergeMorphTargetsWidget::HandleNewMorphNameChanged(const FText& NewText)
{
	NewMorphName = FText::FromString(ObjectTools::SanitizeObjectName(NewText.ToString()));
}

bool SMergeMorphTargetsWidget::IsMergeMorphValid() const
{
	if (Source && !NewMorphName.IsEmptyOrWhitespace() && Selection.Num() > 1)
	{
		return true;
	}
	return false;
}

FReply SMergeMorphTargetsWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SMergeMorphTargetsWidget::OnMergeMorphTarget()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	if (IsMergeMorphValid())
	{
		TArray<FString> MorphTargetNames;
		UMeshOperationsLibrary::GetMorphTargetNames(Source, MorphTargetNames);

		bool bCanContinue = false;

		if (MorphTargetNames.Contains(NewMorphName.ToString()))
		{
			auto bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Morph Target: %s already exists. Overwrite?"), *NewMorphName.ToString())));
			ParentWindow->BringToFront();
			if (bDialogResult == EAppReturnType::Yes)
			{
				bCanContinue = true;
			}
		}
		else {
			bCanContinue = true;
		}

		if (bCanContinue)
		{

			TArray<FString> LocalMorphTargets;
			for (auto& MorphTarget : Selection)
			{
				if (MorphTarget.IsValid())
				{
					LocalMorphTargets.Add(*MorphTarget);
				}
			}

			TArray<FMorphTargetDelta> Deltas;
			GWarn->BeginSlowTask(FText::FromString("Merging Morph Targets ..."), true);
			if (!GWarn->GetScopeStack().IsEmpty())
			{
				GWarn->GetScopeStack().Last()->MakeDialog(false, true);
			}
			GWarn->UpdateProgress(0, 3);
			const bool bResult = UMeshOperationsLibrary::MergeMorphTargets(Source, LocalMorphTargets, Deltas);

			if (bDeleteSources)
			{
				GWarn->StatusForceUpdate(2, 3, FText::FromString("Removing selected Morph Targets ..."));
				UMeshOperationsLibrary::RemoveMorphTargetsFromImportData(Source, LocalMorphTargets, true);
			}

			GWarn->StatusForceUpdate(3, 3, FText::FromString("Aplying Morph Target to Skeletal Mesh ..."));
			UMeshOperationsLibrary::ApplyMorphTargetToImportData(Source, NewMorphName.ToString(), Deltas, true);
			GWarn->EndSlowTask();
			if (Toolkit.IsValid())
			{
				Toolkit->RefreshMorphList();
			}

			FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
			return FReply::Handled();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


