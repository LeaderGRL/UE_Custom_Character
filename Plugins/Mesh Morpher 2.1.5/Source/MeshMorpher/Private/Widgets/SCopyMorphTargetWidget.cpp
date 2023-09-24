// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SCopyMorphTargetWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "MeshOperationsLibrary.h"
#include "MeshOperationsLibraryRT.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/MessageDialog.h"
#include "MeshMorpherToolkit.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SCopyMorphTargetWidget"

void SCopyMorphTargetWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
	Selection = InArgs._Selection;
	Toolkit = InArgs._Toolkit;
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
	Config = GetMutableDefault<UCopyMorphTargetProperties>();
	Config->Initialize();
	Config->bMultipleSelection = Selection.Num() > 1 ? false : true;
	DetailsPanel->SetObject(Config);
	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SScrollBox)
		.Orientation(EOrientation::Orient_Vertical)
		+ SScrollBox::Slot()
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
							.OnClicked_Raw(this, &SCopyMorphTargetWidget::Close)
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
							.OnClicked_Raw(this, &SCopyMorphTargetWidget::OnCopyMorphTarget)
							.IsEnabled_Raw(this, &SCopyMorphTargetWidget::IsCopyMorphValid)
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

	if (Selection.Num() == 1 && Config->NewMorphName.IsEmpty())
	{
		if (Selection[0].IsValid())
		{
			Config->NewMorphName = *Selection[0];
		}
	}
}

bool SCopyMorphTargetWidget::IsCopyMorphValid() const
{

	if (Source && Config->Targets.Num() > 0)
	{
		return true;
	}
	return false;
}

FReply SCopyMorphTargetWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SCopyMorphTargetWidget::OnCopyMorphTarget()
{
	const FString NewMorphName = ObjectTools::SanitizeObjectName(Config->NewMorphName);

	if (IsCopyMorphValid())
	{

		if (Selection.Num() == 1 && NewMorphName.IsEmpty())
		{
			return FReply::Handled();
		}
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
		GWarn->BeginSlowTask(FText::FromString("Copying Morph Target(s) ..."), true);
		if (!GWarn->GetScopeStack().IsEmpty())
		{
			GWarn->GetScopeStack().Last()->MakeDialog(false, true);
		}
		GWarn->UpdateProgress(0, 3);
		for (USkeletalMesh* Target : Config->Targets)
		{
			if (Target && Target != Source)
			{
				TArray<FString> MorphTargetNames;
				UMeshOperationsLibrary::GetMorphTargetNames(Target, MorphTargetNames);


				TArray<FString> LocalMorphTargets;
				for (auto& MorphTarget : Selection)
				{
					if (MorphTarget.IsValid())
					{
						LocalMorphTargets.Add(*MorphTarget);
					}
				}

				TArray<TArray<FMorphTargetDelta>> Deltas;
				const bool bResult = UMeshOperationsLibrary::CopyMorphTarget(Source, LocalMorphTargets, Target, Deltas, Config->Threshold, Config->NormalIncompatibilityThreshold, Config->Multiplier, Config->SmoothIterations, Config->SmoothStrength);
				if (bResult)
				{
					bool bNeedsRebuild = false;
					for (int32 MorphTargetIndex = 0; MorphTargetIndex < LocalMorphTargets.Num(); MorphTargetIndex++)
					{
						FString MorphName = LocalMorphTargets.Num() == 1 ? NewMorphName : LocalMorphTargets[MorphTargetIndex];
						if (Deltas[MorphTargetIndex].Num() > 0)
						{
							bool bCanContinue = false;

							if (MorphTargetNames.Contains(MorphName))
							{
								if (Config->bAutomaticallyOverwrite)
								{
									bCanContinue = true;
								}
								else {
									auto bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Morph Target: %s already exists. Overwrite?"), *MorphName)));
									ParentWindow->BringToFront();
									if (bDialogResult == EAppReturnType::Yes)
									{
										bCanContinue = true;
									}
								}
							}
							else {
								bCanContinue = true;
							}

							if (bCanContinue)
							{
								GWarn->StatusForceUpdate(3, 3, FText::FromString("Aplying Morph Target to Skeletal Mesh ..."));
								UMeshOperationsLibrary::ApplyMorphTargetToImportData(Target, MorphName, Deltas[MorphTargetIndex], false);
								bNeedsRebuild = true;
							}
						}
					}

					if (bNeedsRebuild)
					{

						if (Toolkit.IsValid())
						{
							Toolkit->RefreshMorphList();
						}

						Target->InitMorphTargetsAndRebuildRenderData();
						if (LocalMorphTargets.Num() == 1 && Config->Targets.Num() == 1)
						{
							UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Copying: %s to: %s was successful."), *LocalMorphTargets[0], *Target->GetName()));
							ParentWindow->BringToFront();
						}
					}
					else {
						if (LocalMorphTargets.Num() == 1 && Config->Targets.Num() == 1)
						{
							UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Morph Target: %s didn't generate valid data."), *LocalMorphTargets[0]));
							ParentWindow->BringToFront();
						}
					}
				}
				else {
					if (Config->Targets.Num() == 1)
					{
						UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Morph Target(s) could not be copied.")));
						ParentWindow->BringToFront();
					}
				}
			}
		}

		GWarn->EndSlowTask();

		if (Config->Targets.Num() > 1)
		{
			UMeshOperationsLibrary::NotifyMessage(FString("Finished copying selected Morph Targets."));
			ParentWindow->BringToFront();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
