// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SCreateMorphTargetFromMetaWidget.h"
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
#include "Preview/SPreviewViewport.h"
#include "MeshMorpherEdMode.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SCreateMorphTargetFromMetaWidget"

void SCreateMorphTargetFromMetaWidget::Construct(const FArguments& InArgs)
{
	Toolkit = InArgs._Toolkit;
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
	Config = GetMutableDefault<UCreateMorphTargetFromMetaProperties>();
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
							.OnClicked_Raw(this, &SCreateMorphTargetFromMetaWidget::Close)
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
							.OnClicked_Raw(this, &SCreateMorphTargetFromMetaWidget::OnCreateMorphTarget)
							.IsEnabled_Raw(this, &SCreateMorphTargetFromMetaWidget::IsCreateMorphValid)
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

	Config->bNoTarget = InArgs._NoTarget;
	Config->Targets = InArgs._Targets;
	Config->Sources = InArgs._Sources;
}

bool SCreateMorphTargetFromMetaWidget::IsCreateMorphValid() const
{
	if (Config->Targets.Num() > 0 && Config->Sources.Num() > 0)
	{
		return true;
	}
	return false;
}

FReply SCreateMorphTargetFromMetaWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SCreateMorphTargetFromMetaWidget::OnCreateMorphTarget()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	if (IsCreateMorphValid())
	{
		for (USkeletalMesh* Mesh : Config->Targets)
		{
			TMap<FName, TMap<int32, FMorphTargetDelta>> Deltas;
			if(Config->bNoTarget)
			{
				const bool bResult = UMeshOperationsLibrary::CreateMorphTargetsFromMetaMorph(Toolkit->PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, Toolkit->PreviewViewport->GetEditorMode()->WeldedDynamicMesh, Config->Sources.Array(), Deltas, Config->Threshold, Config->NormalIncompatibilityThreshold, 1.0, Config->SmoothIterations, Config->SmoothStrength, Config->bMergeMoveDeltasToMorphTargets);

			} else
			{
				const bool bResult = UMeshOperationsLibrary::CreateMorphTargetsFromMetaMorph(Mesh, Config->Sources.Array(), Deltas, Config->Threshold, Config->NormalIncompatibilityThreshold, 1.0, Config->SmoothIterations, Config->SmoothStrength, Config->bMergeMoveDeltasToMorphTargets);
			}

			TArray<FString> MorphTargetNames;
			UMeshOperationsLibrary::GetMorphTargetNames(Mesh, MorphTargetNames);

			bool bNeedsRebuild = false;

			for (auto& Delta : Deltas)
			{
				bool bCanContinue = false;
				if (Delta.Value.Num() > 0)
				{
					if (MorphTargetNames.Contains(Delta.Key.ToString()))
					{
						auto bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Morph Target: %s already exists. Overwrite?"), *Delta.Key.ToString())));
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
						TArray<FMorphTargetDelta> LocalDeltas;
						Delta.Value.GenerateValueArray(LocalDeltas);
						UMeshOperationsLibrary::ApplyMorphTargetToImportData(Mesh, Delta.Key.ToString(), LocalDeltas, false);
						bNeedsRebuild = true;
					}
				}
			}

			if (bNeedsRebuild)
			{
				Mesh->InitMorphTargetsAndRebuildRenderData();
				if (Toolkit.IsValid())
				{
					USkeletalMesh* LocalSource = Cast<USkeletalMesh>(Toolkit->SourceFile.GetAsset());
					if (LocalSource && (LocalSource == Mesh))
					{
						Toolkit->RefreshMorphList();
					}
				}
			}
		}

		Close();
	}
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
