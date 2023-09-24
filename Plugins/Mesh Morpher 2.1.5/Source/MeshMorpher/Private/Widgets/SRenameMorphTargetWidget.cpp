// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SRenameMorphTargetWidget.h"
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

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SRenameMorphTargetWidget"

void SRenameMorphTargetWidget::Construct(const FArguments& InArgs)
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
					.OnTextChanged_Raw(this, &SRenameMorphTargetWidget::HandleNewMorphNameChanged)
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
							.OnClicked_Raw(this, &SRenameMorphTargetWidget::Close)
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
							.OnClicked_Raw(this, &SRenameMorphTargetWidget::OnRenameMorphTarget)
							.IsEnabled_Raw(this, &SRenameMorphTargetWidget::IsRenameMorphValid)
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

void SRenameMorphTargetWidget::HandleNewMorphNameChanged(const FText& NewText)
{
	NewMorphName = FText::FromString(ObjectTools::SanitizeObjectName(NewText.ToString()));
}

bool SRenameMorphTargetWidget::IsRenameMorphValid() const
{
	if (Source && MorphTarget.IsValid() && !NewMorphName.IsEmptyOrWhitespace())
	{
		return true;
	}
	return false;
}

FReply SRenameMorphTargetWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SRenameMorphTargetWidget::OnRenameMorphTarget()
{
	if (IsRenameMorphValid())
	{
		TArray<FString> MorphTargetNames;
		UMeshOperationsLibrary::GetMorphTargetNames(Source, MorphTargetNames);

		if (!(NewMorphName.ToString().Equals(*MorphTarget)))
		{
			TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
			if (MorphTargetNames.Contains(NewMorphName.ToString()))
			{
				UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("A Morph Target with name: %s already exists."), *NewMorphName.ToString()));
				ParentWindow->BringToFront();
				return FReply::Handled();
			}
			else {
				FString OriginalName = *MorphTarget;
				UMeshOperationsLibrary::RenameMorphTargetInImportData(Source, *NewMorphName.ToString(), *MorphTarget);
				if (Toolkit.IsValid())
				{
					Toolkit->RefreshMorphList();
				}
				FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
				return FReply::Handled();
			}
		}
		else {
			Close();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION