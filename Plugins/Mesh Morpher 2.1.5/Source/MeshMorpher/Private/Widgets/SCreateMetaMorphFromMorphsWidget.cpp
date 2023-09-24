// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SCreateMetaMorphFromMorphsWidget.h"
#include "SlateOptMacros.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "MeshMorpherToolkit.h"
#include "Modules/ModuleManager.h"
#include "Misc/FeedbackContext.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SCreateMetaMorphFromMorphsWidget"

void SCreateMetaMorphFromMorphsWidget::Construct(const FArguments& InArgs)
{
	Toolkit = InArgs._Toolkit;
	Source = InArgs._Source;
	Selection = InArgs._Selection;
	check(Source);
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NotifyHook = nullptr;
	DetailsViewArgs.bSearchInitialKeyFocus = false;
	DetailsViewArgs.ViewIdentifier = NAME_None;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	// add customization that hides header labels
	TSharedPtr<FMeshMorpherDetailRootObjectCustomization> RootObjectCustomization
		= MakeShared<FMeshMorpherDetailRootObjectCustomization>();
	DetailsPanel->SetRootObjectCustomizationInstance(RootObjectCustomization);
	Config = GetMutableDefault<UCreateMetaMorphFromMorphsProperties>();
	Config->Initialize();
	DetailsPanel->SetObject(Config);
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
					DetailsPanel->AsShared()
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
							.OnClicked_Raw(this, &SCreateMetaMorphFromMorphsWidget::Close)
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
							.OnClicked_Raw(this, &SCreateMetaMorphFromMorphsWidget::OnCreateMorphTarget)
							.IsEnabled_Raw(this, &SCreateMetaMorphFromMorphsWidget::IsCreateMorphValid)
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

bool SCreateMetaMorphFromMorphsWidget::IsCreateMorphValid() const
{

	if (Source && Selection.Num() > 0)
	{
		return true;
	}
	return false;
}

FReply SCreateMetaMorphFromMorphsWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SCreateMetaMorphFromMorphsWidget::OnCreateMorphTarget()
{
	if (IsCreateMorphValid())
	{

		TArray<FString> LocalMorphTargets;
		for (auto& MorphTarget : Selection)
		{
			if (MorphTarget.IsValid())
			{
				LocalMorphTargets.Add(*MorphTarget);
			}
		}

		const bool bResult = UMeshOperationsLibrary::CreateMetaMorphAssetFromMorphTargets(Source, LocalMorphTargets, Config->IgnoreMasks, Config->MoveMasks);
		Close();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
