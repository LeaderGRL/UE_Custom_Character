// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SDuplicateMorphTargetWidget.h"
#include "SlateOptMacros.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"
#include "MeshOperationsLibraryRT.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/MessageDialog.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SDuplicateMorphTargetWidget"

void SDuplicateMorphTargetWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
	MorphTarget = InArgs._MorphTarget;
	Toolkit = InArgs._Toolkit;
	check(Source);
	check(MorphTarget);
	//NewMorphName = FText::FromString(MorphTarget->GetName());
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
					.Text(FText::FromString(TEXT("New Morph Target Name:")))
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.OptionsLabel"))
				]
				+ SVerticalBox::Slot()
				.Padding(0.f, 5.f)
				.AutoHeight()
				[
					SAssignNew(TextBox, SEditableTextBox)
					.MinDesiredWidth(500.f)
					.OnTextChanged_Raw(this, &SDuplicateMorphTargetWidget::HandleNewMorphNameChanged)
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
							.OnClicked_Raw(this, &SDuplicateMorphTargetWidget::Close)
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
							.OnClicked_Raw(this, &SDuplicateMorphTargetWidget::OnDuplicateMorphTarget)
							.IsEnabled_Raw(this, &SDuplicateMorphTargetWidget::IsDuplicateMorphValid)
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

void SDuplicateMorphTargetWidget::HandleNewMorphNameChanged(const FText& NewText)
{
	NewMorphName = FText::FromString(ObjectTools::SanitizeObjectName(NewText.ToString()));
}

bool SDuplicateMorphTargetWidget::IsDuplicateMorphValid() const
{
	if (Source && MorphTarget.IsValid() && !NewMorphName.IsEmptyOrWhitespace() && !(NewMorphName.ToString().Equals(*MorphTarget, ESearchCase::IgnoreCase)))
	{
		return true;
	}
	return false;
}

FReply SDuplicateMorphTargetWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SDuplicateMorphTargetWidget::OnDuplicateMorphTarget()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	if (IsDuplicateMorphValid())
	{
		TArray<FString> MorphTargetNames;
		UMeshOperationsLibrary::GetMorphTargetNames(Source, MorphTargetNames);

		if (!(NewMorphName.ToString().Equals(*MorphTarget)))
		{

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
				TArray<FMorphTargetDelta> Deltas;
				UMeshOperationsLibrary::GetMorphTargetDeltas(Source, *MorphTarget, Deltas);

				UMeshOperationsLibrary::ApplyMorphTargetToImportData(Source, NewMorphName.ToString(), Deltas);

				if (Toolkit.IsValid())
				{
					Toolkit->RefreshMorphList();
				}
				FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
				return FReply::Handled();
			}
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION