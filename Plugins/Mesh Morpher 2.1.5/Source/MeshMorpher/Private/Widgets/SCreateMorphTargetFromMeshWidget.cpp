// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SCreateMorphTargetFromMeshWidget.h"
#include "SlateOptMacros.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"
#include "MeshOperationsLibraryRT.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/MessageDialog.h"
#include "MeshMorpherToolkit.h"
#include "Modules/ModuleManager.h"
#include "Misc/FeedbackContext.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SCreateMorphTargetFromMeshWidget"

void SCreateMorphTargetFromMeshWidget::Construct(const FArguments& InArgs)
{
	Target = InArgs._Target;
	Toolkit = InArgs._Toolkit;
	check(Target);
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
	Config = GetMutableDefault<UCreateMorphTargetFromMeshProperties>();
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
							.OnClicked_Raw(this, &SCreateMorphTargetFromMeshWidget::Close)
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
							.OnClicked_Raw(this, &SCreateMorphTargetFromMeshWidget::OnCreateMorphTarget)
							.IsEnabled_Raw(this, &SCreateMorphTargetFromMeshWidget::IsCreateMorphValid)
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

bool SCreateMorphTargetFromMeshWidget::IsCreateMorphValid() const
{
	if (Target && Config->Source && (Target != Config->Source))
	{
		return true;
	}
	return false;
}

FReply SCreateMorphTargetFromMeshWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SCreateMorphTargetFromMeshWidget::OnCreateMorphTarget()
{
	const FString NewMorphName = ObjectTools::SanitizeObjectName(Config->NewMorphName);
	if (IsCreateMorphValid() && !NewMorphName.IsEmpty())
	{
		TArray<FString> MorphTargetNames;
		UMeshOperationsLibrary::GetMorphTargetNames(Target, MorphTargetNames);
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();

		bool bCanContinue = false;

		if (MorphTargetNames.Contains(NewMorphName))
		{
			auto bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Morph Target: %s already exists. Overwrite?"), *NewMorphName)));
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

			if (Toolkit->SelectedMorphTarget.IsValid() && NewMorphName.Equals(*Toolkit->SelectedMorphTarget, ESearchCase::IgnoreCase))
			{
				Toolkit->CancelTool();
				Toolkit->SelectedMorphTarget = nullptr;
			}

			TArray<FMorphTargetDelta> Deltas;
			GWarn->BeginSlowTask(FText::FromString("Create Morph Target from Mesh ..."), true);
			if (!GWarn->GetScopeStack().IsEmpty())
			{
				GWarn->GetScopeStack().Last()->MakeDialog(false, true);
			}
			GWarn->UpdateProgress(0, 3);
			const bool bResult = UMeshOperationsLibrary::CreateMorphTargetFromMesh(Target, Config->Source, Deltas, Config->Threshold, Config->NormalIncompatibilityThreshold, 1.0, Config->SmoothIterations, Config->SmoothStrength);

			if (bResult)
			{
				GWarn->StatusForceUpdate(3, 3, FText::FromString("Aplying Morph Target to Skeletal Mesh ..."));
				UMeshOperationsLibrary::ApplyMorphTargetToImportData(Target, NewMorphName, Deltas);
				GWarn->EndSlowTask();
				if (Toolkit.IsValid())
				{
					Toolkit->RefreshMorphList();
				}
				UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Creating: %s to: %s was successful."), *NewMorphName, *Target->GetName()));
				FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
			}
			else {
				GWarn->EndSlowTask();
				UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Morph Target: %s has no valid data."), *NewMorphName));
				ParentWindow->BringToFront();
			}

		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
