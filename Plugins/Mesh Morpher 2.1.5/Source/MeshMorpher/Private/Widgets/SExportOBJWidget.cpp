// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SExportOBJWidget.h"
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
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "MeshMorpherToolHelper.h"
#include "MeshMorpherSettings.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SExportOBJWidget"


void SExportOBJWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
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
	Config = GetMutableDefault<UExportOBJProperties>();
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
							.OnClicked_Raw(this, &SExportOBJWidget::Close)
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
							.OnClicked_Raw(this, &SExportOBJWidget::OnExportOBJ)
							.IsEnabled_Raw(this, &SExportOBJWidget::IsExportOBJValid)
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

	UMeshMorpherSettings* Settings = GetMutableDefault<UMeshMorpherSettings>();
	if(Settings)
	{
		Config->MergeVertexTolerance = Settings->MergeVertexTolerance;
		Config->MergeSearchTolerance = Settings->MergeSearchTolerance;
		Config->bMergeOnlyUniquePairs = Settings->OnlyUniquePairs;
	} 
	
}

bool SExportOBJWidget::IsExportOBJValid() const
{

	if (Source)
	{
		return true;
	}
	return false;
}

FReply SExportOBJWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SExportOBJWidget::OnExportOBJ()
{
	if (IsExportOBJValid())
	{
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
		FDynamicMesh3 Output;

		GWarn->BeginSlowTask(FText::FromString("Exporting Mesh Data(s) ..."), true);
		if (!GWarn->GetScopeStack().IsEmpty())
		{
			GWarn->GetScopeStack().Last()->MakeDialog(false, true);
		}
		GWarn->UpdateProgress(0, 1);
		GWarn->StatusForceUpdate(1, 1, FText::FromString("Preparing Mesh for Export ..."));
		const double VertexTolerance = Config->MergeVertexTolerance == 0.0 ? FMathd::ZeroTolerance : Config->MergeVertexTolerance;
		const bool bResult =  UMeshOperationsLibrary::AppenedMeshes(Source, Config->AdditionalSkeletalMeshes, Config->bExportWeldedMesh, VertexTolerance, Config->MergeSearchTolerance, Config->bMergeOnlyUniquePairs, Config->bCreateAdditionalMeshesGroups, Output);
		GWarn->EndSlowTask();
		
		if(bResult)
		{
			const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			if (DesktopPlatform && ParentWindowHandle)
			{
				//Opening the file picker!
				uint32 SelectionFlag = 0; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
				TArray<FString> OutFileNames;
				const FString Title = "Export Skeletal Mesh to OBJ...";
				const bool bFinalResult = DesktopPlatform->SaveFileDialog(ParentWindowHandle, Title, "", Source->GetName(), "OBJ Files|*.OBJ", SelectionFlag, OutFileNames);
				if (bFinalResult && OutFileNames.Num() > 0)
				{
					TArray<FSkeletalMaterial> Materials;
					UMeshMorpherSettings* Settings = GetMutableDefault<UMeshMorpherSettings>();
					if(Settings)
					{
						if(Settings->ExportMaterialSlotsAsSections)
						{
							UMeshOperationsLibraryRT::GetUsedMaterials(Source, 0, Materials);
						}
					}
					
					UMeshMorpherToolHelper::ExportMeshToOBJFile(&Output, TSet<int32>(), OutFileNames[0], false, Materials);
					UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Mesh: %s export completed."), *Source->GetName()));
					ParentWindow->BringToFront();
				}
			}
		} else
		{
			UMeshOperationsLibrary::NotifyMessage(FString::Printf(TEXT("Mesh: %s export failed."), *Source->GetName()));
			ParentWindow->BringToFront();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
