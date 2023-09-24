// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SStitchMorphTargetWidget.h"
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
#include "MeshMorpherToolkit.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SStitchMorphTargetWidget"

void SStitchMorphTargetWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
	MorphTarget = InArgs._MorphTarget;
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
	Config = GetMutableDefault<UStitchMorphTargetProperties>();
	Config->Initialize();
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
							.OnClicked_Raw(this, &SStitchMorphTargetWidget::Close)
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
							.OnClicked_Raw(this, &SStitchMorphTargetWidget::OnStitchMorphTarget)
							.IsEnabled_Raw(this, &SStitchMorphTargetWidget::IsStitchMorphValid)
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

bool SStitchMorphTargetWidget::IsStitchMorphValid() const
{

	if (Source && Config->Target && !FText::FromString(Config->TargetMorphTarget).IsEmptyOrWhitespace())
	{
		return true;
	}
	return false;
}

FReply SStitchMorphTargetWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SStitchMorphTargetWidget::OnStitchMorphTarget()
{
	if (IsStitchMorphValid())
	{
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
		GWarn->BeginSlowTask(FText::FromString("Stitching Morph Target(s) ..."), true);
		if (!GWarn->GetScopeStack().IsEmpty())
		{
			GWarn->GetScopeStack().Last()->MakeDialog(false, true);
		}
		GWarn->UpdateProgress(0, 3);

		if (Config->Target && Config->Target != Source)
		{
			TArray<FMorphTargetDelta> Deltas;
			TArray<FMorphTargetDelta> NeighborDeltas;
			bool bResult = false;
			//const bool bResult = UMeshOperationsLibrary::CreateStitchDeltas(Source, *MorphTarget, Config->Target, Config->TargetMorphTarget, Deltas, NeighborDeltas, Config->Threshold);
			if (bResult)
			{
				bool bNeedsRebuild = false;
				bool bNeighborNeedsRebuild = false;
				if (Deltas.Num() > 0)
				{
					GWarn->StatusForceUpdate(3, 3, FText::FromString("Applying Morph Target to Skeletal Mesh ..."));
					UMeshOperationsLibrary::ApplyMorphTargetToImportData(Source, *MorphTarget, Deltas, false);
					bNeedsRebuild = true;
				}

				if (NeighborDeltas.Num() > 0)
				{
					GWarn->StatusForceUpdate(3, 3, FText::FromString("Applying Morph Target to Neighbor Skeletal Mesh ..."));
					UMeshOperationsLibrary::ApplyMorphTargetToImportData(Config->Target, Config->TargetMorphTarget, NeighborDeltas, false);
					bNeighborNeedsRebuild = true;
				}			

				if (bNeedsRebuild)
				{

					if (Toolkit.IsValid())
					{
						Toolkit->RefreshMorphList();
					}

					Source->InitMorphTargetsAndRebuildRenderData();

					if(bNeighborNeedsRebuild)
					{
						Config->Target->InitMorphTargetsAndRebuildRenderData();
					}
				
					UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Stitching: %s was successful."), *(*MorphTarget)));
					ParentWindow->BringToFront();
				}
				else {
					UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Stitching: %s didn't generate valid data."), *(*MorphTarget)));
					ParentWindow->BringToFront();
				}
			}
			else {
				UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Stitching: %s didn't generate valid data."), *(*MorphTarget)));
				ParentWindow->BringToFront();
			}
		}

		GWarn->EndSlowTask();
		Close();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
