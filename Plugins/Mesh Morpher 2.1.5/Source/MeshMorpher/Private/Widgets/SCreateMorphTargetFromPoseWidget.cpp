// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SCreateMorphTargetFromPoseWidget.h"
#include "SlateOptMacros.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/MessageDialog.h"
#include "MeshMorpherEdMode.h"
#include "Preview/SPreviewViewport.h"
#include "Misc/FeedbackContext.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SCreateMorphTargetFromPoseWidget"

void SCreateMorphTargetFromPoseWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
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
					.Text(FText::FromString(TEXT("Morph Target Name:")))
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.OptionsLabel"))
				]
				+ SVerticalBox::Slot()
				.Padding(0.f, 5.f)
				.AutoHeight()
				[
					SNew(SEditableTextBox)
					.MinDesiredWidth(500.f)
					.OnTextChanged_Raw(this, &SCreateMorphTargetFromPoseWidget::HandleNewMorphNameChanged)
					.Text(NewMorphName)
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
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
							.OnClicked_Raw(this, &SCreateMorphTargetFromPoseWidget::Close)
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
							.OnClicked_Raw(this, &SCreateMorphTargetFromPoseWidget::OnCreateMorphTarget)
							.IsEnabled_Raw(this, &SCreateMorphTargetFromPoseWidget::IsCreateMorphValid)
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

void SCreateMorphTargetFromPoseWidget::HandleNewMorphNameChanged(const FText& NewText)
{
	NewMorphName = FText::FromString(ObjectTools::SanitizeObjectName(NewText.ToString()));
}

bool SCreateMorphTargetFromPoseWidget::IsCreateMorphValid() const
{
	if (Source && !NewMorphName.IsEmptyOrWhitespace())
	{
		return true;
	}
	return false;
}

FReply SCreateMorphTargetFromPoseWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SCreateMorphTargetFromPoseWidget::OnCreateMorphTarget()
{
	const FString LocalNewMorphName = ObjectTools::SanitizeObjectName(NewMorphName.ToString());
	if (IsCreateMorphValid() && !LocalNewMorphName.IsEmpty())
	{
		TArray<FString> MorphTargetNames;
		UMeshOperationsLibrary::GetMorphTargetNames(Source, MorphTargetNames);
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();

		bool bCanContinue = false;

		if (MorphTargetNames.Contains(LocalNewMorphName))
		{
			auto bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Morph Target: %s already exists. Overwrite?"), *LocalNewMorphName)));
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
			if (Toolkit->SelectedMorphTarget.IsValid() && LocalNewMorphName.Equals(*Toolkit->SelectedMorphTarget, ESearchCase::IgnoreCase))
			{
				Toolkit->CancelTool();
				Toolkit->SelectedMorphTarget = nullptr;
			}

			TArray<FMorphTargetDelta> Deltas;
			GWarn->BeginSlowTask(FText::FromString("Create Morph Target from Pose ..."), true);
			if (!GWarn->GetScopeStack().IsEmpty())
			{
				GWarn->GetScopeStack().Last()->MakeDialog(false, true);
			}
			GWarn->UpdateProgress(0, 3);
			const bool bResult = UMeshOperationsLibrary::CreateMorphTargetFromPose(Source, Toolkit->PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, Deltas);

			if (bResult)
			{
				GWarn->StatusForceUpdate(3, 3, FText::FromString("Aplying Morph Target to Skeletal Mesh ..."));
				UMeshOperationsLibrary::ApplyMorphTargetToImportData(Source, LocalNewMorphName, Deltas);
				GWarn->EndSlowTask();
				if (Toolkit.IsValid())
				{
					Toolkit->RefreshMorphList();
					Toolkit->OnPoseAssetSelected(NULL);
				}

				UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Creating: %s to: %s was successful."), *LocalNewMorphName, *Source->GetName()));
				FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
			}
			else {
				GWarn->EndSlowTask();
				UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Morph Target: %s has no valid data."), *LocalNewMorphName));
				ParentWindow->BringToFront();
			}

		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
