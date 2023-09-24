// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SAddMorphTargetWidget.h"
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

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SAddMorphTargetWidget"

void SAddMorphTargetWidget::Construct(const FArguments& InArgs)
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
					SAssignNew(TextBox, SEditableTextBox)
					.MinDesiredWidth(500.f)
					.OnTextChanged_Raw(this, &SAddMorphTargetWidget::HandleNewMorphNameChanged)
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
							.OnClicked_Raw(this, &SAddMorphTargetWidget::Close)
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
							.OnClicked_Raw(this, &SAddMorphTargetWidget::OnCreateMorphTarget)
							.IsEnabled_Raw(this, &SAddMorphTargetWidget::IsCreateMorphValid)
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

void SAddMorphTargetWidget::HandleNewMorphNameChanged(const FText& NewText)
{
	NewMorphName = FText::FromString(ObjectTools::SanitizeObjectName(NewText.ToString()));
}

bool SAddMorphTargetWidget::IsCreateMorphValid() const
{
	if (Source && !NewMorphName.IsEmptyOrWhitespace())
	{
		return true;
	}
	return false;
}

FReply SAddMorphTargetWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SAddMorphTargetWidget::OnCreateMorphTarget()
{
	if (IsCreateMorphValid())
	{
		TArray<FString> MorphTargetNames;
		UMeshOperationsLibrary::GetMorphTargetNames(Source, MorphTargetNames);
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
		if (MorphTargetNames.Contains(NewMorphName.ToString()))
		{
			auto result = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString("Morph Name already exists. Overwrite?"));
			ParentWindow->BringToFront();
			if (result == EAppReturnType::Yes)
			{
				UMeshOperationsLibrary::CreateMorphTarget(Source, NewMorphName.ToString());
				if (Toolkit.IsValid())
				{
					Toolkit->RefreshMorphList();
				}

				FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
				return FReply::Handled();
			}
		}
		else {
			UMeshOperationsLibrary::CreateMorphTarget(Source, NewMorphName.ToString());
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
