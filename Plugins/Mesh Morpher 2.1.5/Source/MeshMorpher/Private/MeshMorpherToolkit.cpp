// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherToolkit.h"
#include "UnrealEdMisc.h"
#include "MeshMorpher.h"

#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"

#include "Widgets/SAddMorphTargetWidget.h"
#include "Widgets/SCopyMorphTargetWidget.h"
#include "Widgets/SExportOBJWidget.h"
#include "Widgets/SSelectReferenceSkeletalMeshWidget.h"
#include "Widgets/SMergeMorphTargetsWidget.h"
#include "Widgets/SRenameMorphTargetWidget.h"
#include "Widgets/SDuplicateMorphTargetWidget.h"
#include "Widgets/SCreateMorphTargetFromPoseWidget.h"
#include "Widgets/SCreateMorphTargetFromMeshWidget.h"
#include "Widgets/SCreateMorphTargetFromMetaWidget.h"
#include "Widgets/SCreateMetaMorphFromFbxWidget.h"
#include "Widgets/SCreateMetaMorphFromMorphsWidget.h"
#include "Widgets/SStitchMorphTargetWidget.h"
#include "Widgets/SOpenMaskSelectionWidget.h"
#include "Widgets/SMeshMorpherTool.h"
#include "Widgets/SMorphTargetListWidget.h"
#include "Widgets/SAboutWindow.h"
#include "Widgets/SBakeSkeletalMeshWidget.h"
#include "Widgets/SMagnitudeMorphTargetWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SSlider.h"
#include "PropertyCustomizationHelpers.h"

#include "Preview/SPreviewViewport.h"
#include "AdvancedPreviewScene.h"

#include "AssetThumbnail.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/LayoutService.h"
#include "EditorModeManager.h"
#include "MeshMorpherEdMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshMorpherMeshComponent.h"
#include "Animation/AnimSequence.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

#include "MeshOperationsLibrary.h"
#include "MeshOperationsLibraryRT.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"

#include "Engine/Engine.h"

#include "MeshMorpherCommands.h"
#include "Tools/MeshMorpherTool.h"
#include "DynamicMeshToMeshDescription.h"
#include "AssetRegistryModule.h"
#include "ISettingsModule.h"

#include "Misc/FeedbackContext.h"
#include "Tools/MeshMorpherToolInterface.h"
#include "Internationalization/Text.h"

//File picker includes
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"

#include "StandaloneMaskSelection.h"
#include "AssetToolsModule.h"
#include "Dialogs/DlgPickAssetPath.h"

#include "FileHelpers.h"
#include "MeshMorpherSettings.h"
#include "Widgets/SCreateMorphTargetFromFBXWidget.h"

#define LOCTEXT_NAMESPACE "MeshMorpherToolkit"


namespace MeshMorpher
{
	static const FName ToolbarID("MeshMorpherToolbar");
	static const FName PreviewTabID("MeshMorpherPreviewTab");
	static const FName PoseTabID("MeshMorpherPoseTab");
	static const FName MorphTargetTabID("MeshMorpherMorphTargetTab");
	static const FName ToolTabID("MeshMorpherToolTab");
}


void FMeshMorpherToolkit::Construct(const FArguments& InArgs, USkeletalMesh* SkeletalMesh)
{
	SDockTab::Construct(InArgs);
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(128));
	
	UICommands = MakeShareable(new FUICommandList);

	UICommands->MapAction(
		FMeshMorpherCommands::Get().SaveButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnSaveAsset),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsSaveAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().New,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OpenNewToolkit),
		FCanExecuteAction());

	UICommands->MapAction(
		FMeshMorpherCommands::Get().OpenMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnOpenSelected),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().DeselectMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnMorphTargetDeselect),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsSelectedMorphValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().AddMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnShowCreateMorphTargetWindow),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().DeleteMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnRemoveMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().RenameMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnRenameMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().DuplicateMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnDuplicateMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().EnableBuildDataButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnEnableBuildData),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsBuildDataAvailable));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().DisableBuildDataButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnDisableBuildData),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsBuildDataAvailable));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().SettingsButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnOpenSettings),
		FCanExecuteAction());

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CopyMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCopyMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().SelectReferenceMeshButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnSelectReferenceSkeletalMesh),
		FCanExecuteAction());
		//FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().MergeMorphsButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnMergeMorphTargets),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsMultiSelectedMorphValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ExportMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnExportMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ExportOBJButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnExportMeshObj),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().BakeButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnBake),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMorphFromPoseButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMorphTargetFromPose),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsPoseBarEnabled));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMorphFromMeshButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMorphTargetFromMesh),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMorphFromFBXButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMorphTargetFromFBX),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAssetValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMetaMorphFromFBXButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMetaMorphTargetFromFBX),
		FCanExecuteAction());

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMetaMorphFromMorphTargetButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMetaMorphTargetFromMorphTargets),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMorphFromMetaButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMorphFromMeta, false),
		FCanExecuteAction());

	UICommands->MapAction(
		FMeshMorpherCommands::Get().CreateMorphFromMetaCurrentButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnCreateMorphFromMeta, true),
		FCanExecuteAction());


	UICommands->MapAction(
		FMeshMorpherCommands::Get().ClearMaskSelectiontButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnClearMaskSelection),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasMaskSelection));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ToggleMaskSelectionButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnToggleMaskSelection),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasSelectedTool));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().IncreaseBrushSizeButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnIncreaseBrushSize),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasSelectedTool));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().DecreaseBrushSizeButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnDecreaseBrushSize),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasSelectedTool));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().SaveSelectionToAssetButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnExportSelectionToAsset),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasMaskSelection));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().SaveSelectionButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnSaveSelectionAsset),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasMaskSelectionAsset));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().OpenMaskSelectionButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnOpenSelectionFromAsset),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasSelectedTool));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ExportMaskSelectionOBJButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnExportMaskSelectionOBJ),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::HasMaskSelection));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ExportMorphTargetOBJButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnExportMorphTargetOBJ),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ExportMorphTargetOBJMenuButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnExportMorphTargetOBJ),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));


	UICommands->MapAction(
		FMeshMorpherCommands::Get().FocusCameraButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnFocusCamera));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ApplyMorphTargetToLODsButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnApplyMorphTargetToLODs),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().MorphMagnitudeButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnUpdateMagnitudeMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValid));


	UICommands->MapAction(
		FMeshMorpherCommands::Get().StitchMorphButton,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::OnStitchMorphTarget),
		FCanExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne));


	UICommands->MapAction(
		FMeshMorpherCommands::Get().Exit,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::RequestManualClose));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().About,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::ShowAboutWindow));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ToolbarTab,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::ToggleToolbarTab),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FMeshMorpherToolkit::IsToolbarTabValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().PoseTab,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::TogglePoseTab),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FMeshMorpherToolkit::IsPoseTabValid));


	UICommands->MapAction(
		FMeshMorpherCommands::Get().PreviewTab,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::TogglePreviewTab),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FMeshMorpherToolkit::IsPreviewTabValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().ToolTab,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::ToggleToolTab),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FMeshMorpherToolkit::IsToolTabValid));

	UICommands->MapAction(
		FMeshMorpherCommands::Get().MorphTargetsTab,
		FExecuteAction::CreateRaw(this, &FMeshMorpherToolkit::ToggleMorphTargetsTab),
		FCanExecuteAction(),
		FIsActionChecked::CreateRaw(this, &FMeshMorpherToolkit::IsMorphTargetsTabValid));

	ThisPtr = StaticCastSharedRef<FMeshMorpherToolkit>(AsShared());

	FSlateApplication::Get().RegisterInputPreProcessor(ThisPtr);

	SAssignNew(PreviewViewport, SMeshMorpherPreviewViewport);


	PoseSkeletalMeshComponent = NewObject<USkeletalMeshComponent>(PreviewViewport->PreviewScene->GetWorld(), NAME_None, RF_Transient);
	PoseSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	PoseSkeletalMeshComponent->SetPlayRate(0.0f);
	PoseSkeletalMeshComponent->SetForcedLOD(1);
	
	ReferenceSkeletalMeshComponent = NewObject<USkeletalMeshComponent>(PreviewViewport->PreviewScene->GetWorld(), NAME_None, RF_Transient);
	ReferenceSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	ReferenceSkeletalMeshComponent->SetPlayRate(0.0f);
	ReferenceSkeletalMeshComponent->SetForcedLOD(1);
	SetMasterPoseNull();
	
	PreviewViewport->AddComponent(PoseSkeletalMeshComponent);
	PreviewViewport->AddComponent(ReferenceSkeletalMeshComponent);

	UMeshMorpherEdMode* Mode = PreviewViewport->GetEditorMode();

	SAssignNew(ToolWidget, SMeshMorpherToolWidget)
				.EdMode(Mode);

	Settings = GetMutableDefault<UMeshMorpherSettings>();

	FSlateApplication::Get().ForEachUser([&](FSlateUser& User) {
		FSlateApplication::Get().SetKeyboardFocus(PreviewViewport, EFocusCause::SetDirectly);
	});

	TabManager = FGlobalTabmanager::Get()->NewTabManager(SharedThis(this));
	TabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateRaw(this, &FMeshMorpherToolkit::HandleTabManagerPersistLayout));
	RegisterTabSpawners(TabManager);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_MeshMorpher")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(MeshMorpher::ToolbarID, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.1f)

				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.8f)
						->Split
						(
							FTabManager::NewStack()
							->AddTab(MeshMorpher::PreviewTabID, ETabState::OpenedTab)
							->SetHideTabWell(true)
							->SetSizeCoefficient(0.8f)
						)
						->Split
						(
							FTabManager::NewStack()
							->AddTab(MeshMorpher::PoseTabID, ETabState::OpenedTab)
							->SetHideTabWell(true)
							->SetSizeCoefficient(0.2f)
						)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.2f)
						->Split
						(
							FTabManager::NewStack()
							->AddTab(MeshMorpher::MorphTargetTabID, ETabState::OpenedTab)
							->SetSizeCoefficient(0.3f)
							->SetHideTabWell(false)
						)
						->Split
						(
							FTabManager::NewStack()
							->AddTab(MeshMorpher::ToolTabID, ETabState::OpenedTab)
							->SetSizeCoefficient(0.7f)
							->SetHideTabWell(false)
						)
					)
				)
			)
		);

	TSharedRef<FTabManager::FLayout> LayoutToUse = FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, Layout);
	SetContent(GenerateWindow(LayoutToUse));
	if (SkeletalMesh)
	{
		SelectSourceSkeletalMesh(SkeletalMesh);
	}

	this->TabLabel.BindRaw(this, &FMeshMorpherToolkit::GetTitle);
	TabManager->SetAllowWindowMenuBar(true);
}

FText FMeshMorpherToolkit::GetTitle() const
{
	FString Title = "Mesh Morpher";
	if (SourceFile.IsValid())
	{
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			Title = LocalSource->GetName() + " - " + Title;

			if (SelectedMorphTarget.IsValid())
			{
				Title = *SelectedMorphTarget + " - " + Title;
			}
		}
	}
	return FText::FromString(Title);
}




FMeshMorpherToolkit::~FMeshMorpherToolkit()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			PreviewViewport->GetEditorMode()->CancelTool();
		}
	}

	if (FSlateApplication::IsInitialized() && ThisPtr.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ThisPtr);
	}

	if (TabManager.IsValid())
	{
		TabManager->UnregisterAllTabSpawners();
	}
}

void FMeshMorpherToolkit::HandleTabManagerPersistLayout(const TSharedRef<FTabManager::FLayout>& LayoutToSave)
{
	if (FUnrealEdMisc::Get().IsSavingLayoutOnClosedAllowed())
	{
		FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, LayoutToSave);
	}
}

void FMeshMorpherToolkit::RegisterTabSpawners(TSharedPtr<FTabManager>& InTabManager)
{
	const auto& LocalCategories = InTabManager->GetLocalWorkspaceMenuRoot()->GetChildItems();
	TSharedRef<FWorkspaceItem> ToolbarSpawnerCategory = LocalCategories.Num() > 0 ? LocalCategories[0] : InTabManager->GetLocalWorkspaceMenuRoot();

	InTabManager->RegisterTabSpawner(MeshMorpher::ToolbarID, FOnSpawnTab::CreateRaw(this, &FMeshMorpherToolkit::SpawnTab_Toolbar))
		.SetDisplayName(LOCTEXT("ToolbarTab", "Toolbar"))
		.SetGroup(ToolbarSpawnerCategory)
		.SetIcon(FSlateIcon("MeshMorpherStyle", "MeshMorpher.ToolbarTab"));

	auto WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MeshMorpher", "Mesh Morpher"));

	InTabManager->RegisterTabSpawner(MeshMorpher::PreviewTabID, FOnSpawnTab::CreateRaw(this, &FMeshMorpherToolkit::SpawnTab_Preview))
		.SetDisplayName(LOCTEXT("PreviewTabName", "Preview"))
		.SetGroup(WorkspaceMenuCategory)
		.SetIcon(FSlateIcon("MeshMorpherStyle", "MeshMorpher.PreviewTab"));

	InTabManager->RegisterTabSpawner(MeshMorpher::PoseTabID, FOnSpawnTab::CreateRaw(this, &FMeshMorpherToolkit::SpawnTab_Pose))
		.SetDisplayName(LOCTEXT("PoseTabName", "Pose"))
		.SetGroup(WorkspaceMenuCategory)
		.SetIcon(FSlateIcon("MeshMorpherStyle", "MeshMorpher.PoseTab"));
	
	InTabManager->RegisterTabSpawner(MeshMorpher::ToolTabID, FOnSpawnTab::CreateRaw(this, &FMeshMorpherToolkit::SpawnTab_Tool))
		.SetDisplayName(LOCTEXT("ToolTabName", "Tool"))
		.SetGroup(WorkspaceMenuCategory)
		.SetIcon(FSlateIcon("MeshMorpherStyle", "MeshMorpher.ToolTab"));

	InTabManager->RegisterTabSpawner(MeshMorpher::MorphTargetTabID, FOnSpawnTab::CreateRaw(this, &FMeshMorpherToolkit::SpawnTab_MorphTargets))
		.SetDisplayName(LOCTEXT("MorphTargetsTabName", "Morph Targets"))
		.SetGroup(WorkspaceMenuCategory)
		.SetIcon(FSlateIcon("MeshMorpherStyle", "MeshMorpher.MorphTargetsTab"));
}

TSharedRef<SDockTab> FMeshMorpherToolkit::SpawnTab_Toolbar(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == MeshMorpher::ToolbarID);

	auto WidgetSelectAsset = SNew(SBox)
		.MinDesiredWidth(FOptionalSize(300.f))
		.MaxDesiredHeight(FOptionalSize(50.f))
		.VAlign(EVerticalAlignment::VAlign_Center)
		.Padding(FMargin(0.0f, -3.0f, 0.0f, 0.0f))
		[
			SNew(SObjectPropertyEntryBox)
			.ObjectPath_Raw(this, &FMeshMorpherToolkit::GetSkeletalMeshAssetName)
			.OnObjectChanged_Raw(this, &FMeshMorpherToolkit::OnSourceAssetSelected, true)
			.AllowClear(true)
			.AllowedClass(FMeshMorpherToolkit::OnGetClassesForSourcePicker())
			.OnShouldFilterAsset_Raw(this, &FMeshMorpherToolkit::FilterSourcePicker)
			.DisplayThumbnail(true)
			.ThumbnailPool(ThumbnailPool)
			.DisplayCompactSize(true)
			.DisplayBrowse(true)
		];

	FMultiBoxCustomization Customization(TEXT("MeshMorpherToolbar"));

	TSharedPtr<FToolBarBuilder> ToolBarBuilder = MakeShareable(new FToolBarBuilder(UICommands, Customization));

	ToolBarBuilder->BeginSection(TEXT("MeshMorpherToolBarBrowse"));
	ToolBarBuilder->AddToolBarWidget(WidgetSelectAsset);
	ToolBarBuilder->EndSection();
	ToolBarBuilder->BeginSection(TEXT("MeshMorpherToolBarSave"));
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().SaveButton);
	ToolBarBuilder->EndSection();
	ToolBarBuilder->BeginSection(TEXT("MeshMorpherEditBarMorph"));
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().AddMorphButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().DeleteMorphButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().RenameMorphButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().DuplicateMorphButton);
	ToolBarBuilder->EndSection();
	ToolBarBuilder->BeginSection(TEXT("MeshMorpherToolBarMorph"));
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().CopyMorphButton);
	//ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().SelectReferenceMeshButton);
	//ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().CreateMorphFromPoseButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().MergeMorphsButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().ExportMorphButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().BakeButton);
	ToolBarBuilder->EndSection();
	ToolBarBuilder->BeginSection(TEXT("MeshMorpherToolBarMetaMorph"));
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().CreateMetaMorphFromFBXButton);
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().CreateMorphFromMetaButton, NAME_None,LOCTEXT("CreateMorphFromMetaButton", "Create From Meta Morph"), LOCTEXT("CreateMorphFromMetaButton_ToolTip", "Creates Morph Target(s) from selected Meta Morph assets"), FSlateIcon(FName("MeshMorpherStyle"), FName("MeshMorpher.CreateMorphFromMeta"), FName("MeshMorpher.CreateMorphFromMeta.Small")));
	ToolBarBuilder->EndSection();
	ToolBarBuilder->BeginSection(TEXT("MeshMorpherToolBarFileMorph"));
	ToolBarBuilder->AddToolBarButton(FMeshMorpherCommands::Get().CreateMorphFromFBXButton);
	ToolBarBuilder->EndSection();
	TSharedPtr<SDockTab> ToolbarTab = SNew(SDockTab)
		.ShouldAutosize(true)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			[
				ToolBarBuilder->MakeWidget()
			]
	];

	return ToolbarTab.ToSharedRef();
}

TSharedRef<SDockTab> FMeshMorpherToolkit::SpawnTab_Preview(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == MeshMorpher::PreviewTabID);

	TSharedPtr<SDockTab> PreviewTab = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(FSlateColor(FLinearColor::Black))
			[
				PreviewViewport.ToSharedRef()
			]
		];

	return PreviewTab.ToSharedRef();
}

TSharedRef<SDockTab> FMeshMorpherToolkit::SpawnTab_Pose(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == MeshMorpher::PoseTabID);

	TSharedPtr<SDockTab> PoseTab = SNew(SDockTab)
	.TabRole(ETabRole::PanelTab)
	[
		SNew(SBorder)
		.Padding(0)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(FSlateColor(FLinearColor::Black))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(FOptionalSize(300.f))
					.MaxDesiredHeight(FOptionalSize(50.f))
					.VAlign(EVerticalAlignment::VAlign_Center)
					.Padding(FMargin(0.0f, -3.0f, 0.0f, 0.0f))
					[
						SNew(SObjectPropertyEntryBox)
						.ObjectPath_Raw(this, &FMeshMorpherToolkit::GetPoseAssetName)
						.OnObjectChanged_Raw(this, &FMeshMorpherToolkit::OnPoseAssetSelected)
						.AllowClear(true)
						.AllowedClass(FMeshMorpherToolkit::OnGetClassesForSourcePicker())
						.OnShouldFilterAsset_Raw(this, &FMeshMorpherToolkit::FilterPosePicker)
						.DisplayThumbnail(true)
						.ThumbnailPool(ThumbnailPool)
						.DisplayCompactSize(true)
						.DisplayBrowse(true)
						.IsEnabled_Raw(this, &FMeshMorpherToolkit::IsAssetValid)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SBox)
					[
						SNew(SSlider)
						.MinValue(0.0f)
						.MaxValue(1.0f)
						.StepSize_Lambda([&]()
						{
							return 1.0f / MaxPosePosition;
						})		
						.Orientation(EOrientation::Orient_Horizontal)
						.IsEnabled_Raw(this, &FMeshMorpherToolkit::IsPoseBarEnabled)
						.OnValueChanged_Raw(this, &FMeshMorpherToolkit::PosePositionChanged)
						.Value_Lambda([&]()
						{
							return CurrentPosePosition;
						})
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(FOptionalSize(70.f))
					.MaxDesiredWidth(FOptionalSize(70.f))
					.MaxDesiredHeight(FOptionalSize(50.f))
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew( SSpinBox<float> )
						.MinSliderValue(0.0f)
						.MaxSliderValue_Lambda([&]()
						{
							return MaxPosePosition;
						})					
						.MinValue(0.0f)
						.MaxValue_Lambda([&]()
						{
							return MaxPosePosition;
						})
						.Value_Lambda([&]()
						{
							return MaxPosePosition * CurrentPosePosition;
						})
						.OnValueCommitted_Raw(this, &FMeshMorpherToolkit::PosePositionComitted)
						.IsEnabled_Raw(this, &FMeshMorpherToolkit::IsPoseBarEnabled)
					]

				]				
			]
		]
	];
	return PoseTab.ToSharedRef();
}


void FMeshMorpherToolkit::PosePositionComitted(float NewPosition, ETextCommit::Type CommitType)
{
	if (NewPosition != CurrentPosePosition && MaxPosePosition > 0.0f)
	{
		CurrentPosePosition = NewPosition / MaxPosePosition;
		OnPoseAssetSelected(PoseFile);
	}
}

TSharedRef<SDockTab> FMeshMorpherToolkit::SpawnTab_Tool(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == MeshMorpher::ToolTabID);

	TSharedPtr<SDockTab> ToolTab = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.BorderBackgroundColor(FSlateColor(FLinearColor::Black))
				[
					ToolWidget.ToSharedRef()
				]
			]
		];
		return ToolTab.ToSharedRef();
}

TSharedRef<SDockTab> FMeshMorpherToolkit::SpawnTab_MorphTargets(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == MeshMorpher::MorphTargetTabID);
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());

	TSharedPtr<SDockTab> MorphTargetsTab = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(FSlateColor(FLinearColor::Black))
			[
				SNew(SBox)
				[
					SAssignNew(MorphListView, SMorphTargetListWidget)
					.Source(LocalSource)
					.InitialSelection(SelectedMorphTarget)
					.OnSelectionChanged_Raw(this, &FMeshMorpherToolkit::MorphTargetSelectionChanged)
					.Commands(UICommands)
				]
			]
		];

	return MorphTargetsTab.ToSharedRef();
}

bool FMeshMorpherToolkit::IsToolbarTabValid() const
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::ToolbarID);
	if (FoundTab.IsValid())
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsPoseTabValid() const
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::PoseTabID);
	if (FoundTab.IsValid())
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsPreviewTabValid() const
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::PreviewTabID);
	if (FoundTab.IsValid())
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsToolTabValid() const
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::ToolTabID);
	if (FoundTab.IsValid())
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsMorphTargetsTabValid() const
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::MorphTargetTabID);
	if (FoundTab.IsValid())
	{
		return true;
	}
	return false;
}

void FMeshMorpherToolkit::ToggleToolbarTab()
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::ToolbarID);
	if (FoundTab.IsValid())
	{
		FoundTab.Pin()->RequestCloseTab();
	}
	else {
		TabManager->TryInvokeTab(MeshMorpher::ToolbarID);
	}
}

void FMeshMorpherToolkit::TogglePoseTab()
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::PoseTabID);
	if (FoundTab.IsValid())
	{
		FoundTab.Pin()->RequestCloseTab();
	}
	else {
		TabManager->TryInvokeTab(MeshMorpher::PoseTabID);
	}
}


void FMeshMorpherToolkit::TogglePreviewTab()
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::PreviewTabID);
	if (FoundTab.IsValid())
	{
		FoundTab.Pin()->RequestCloseTab();
	}
	else {
		TabManager->TryInvokeTab(MeshMorpher::PreviewTabID);
	}
}

void FMeshMorpherToolkit::ToggleToolTab()
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::ToolTabID);
	if (FoundTab.IsValid())
	{
		FoundTab.Pin()->RequestCloseTab();
	}
	else {
		TabManager->TryInvokeTab(MeshMorpher::ToolTabID);
	}
}

void FMeshMorpherToolkit::ToggleMorphTargetsTab()
{
	TWeakPtr<SDockTab> FoundTab = TabManager->FindExistingLiveTab(MeshMorpher::MorphTargetTabID);
	if (FoundTab.IsValid())
	{
		FoundTab.Pin()->RequestCloseTab();
	}
	else {
		TabManager->TryInvokeTab(MeshMorpher::MorphTargetTabID);
	}
}

void FMeshMorpherToolkit::ShowAboutWindow()
{
	auto AboutWindow = SNew(SWindow)
		.CreateTitleBar(true)
		.Title(FText::FromString(TEXT("About")))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.FocusWhenFirstShown(true)
		.IsTopmostWindow(true)
		.SizingRule(ESizingRule::Autosized)
		[
			SNew(SAboutWindow)
		];

	if (GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddModalWindow(AboutWindow, GetParentWindow());
	}
}

void FMeshMorpherToolkit::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> InCursor)
{
	
}

void FMeshMorpherToolkit::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
}

bool FMeshMorpherToolkit::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (GetParentWindow().IsValid())
	{
		TSharedPtr<SWidget> Widget = FSlateApplication::Get().GetUserFocusedWidget(0);
		if (Widget.IsValid())
		{
			if (Widget->GetTypeAsString().Contains("EditableText"))
			{
				//UE_LOG(LogTemp, Warning, TEXT("Widget: %s"), *Widget->GetTypeAsString());
				return false;
			}
		}

		if (UICommands.IsValid() && !GetParentWindow()->IsWindowMinimized() && (GetParentWindow()->HasAnyUserFocusOrFocusedDescendants() || GetParentWindow()->HasKeyboardFocus()))
		{
			TSharedPtr<SDockTab> Tabl = FGlobalTabmanager::Get()->GetActiveTab();
			bool bContinue = false;
			if (Tabl.IsValid())
			{
				FString Identifier = FGlobalTabmanager::Get()->GetActiveTab()->GetLayoutIdentifier().ToString();
				if (Identifier.Contains("MeshMorpher"))
				{
					bContinue = true;
				}
			}

			if(bContinue)
			{
				if (!TrackedWindow.IsValid())
				{
					FModifierKeysState KeyState = InKeyEvent.GetModifierKeys();
					if (UICommands->ProcessCommandBindings(InKeyEvent.GetKey(), KeyState, InKeyEvent.IsRepeat())) //-V547
					{
						return true;
					}

					FInputChord CheckChord(InKeyEvent.GetKey(), EModifierKey::FromBools(InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(), InKeyEvent.IsShiftDown(), InKeyEvent.IsCommandDown()));

					if (PreviewViewport.IsValid())
					{
						if (PreviewViewport->GetEditorMode() != nullptr)
						{
							if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
							{
								if (CurTool->SelectedTool)
								{
									if (!CurTool->SelectedTool->IsUnreachable())
									{
										IMeshMorpherToolInterface::Execute_OnKeyPressed(CurTool->SelectedTool, CheckChord);
										//UE_LOG(LogTemp, Warning, TEXT("Pressed: %s"), *InKeyEvent.GetKey().ToString());
										return false;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

bool FMeshMorpherToolkit::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				FInputChord CheckChord(InKeyEvent.GetKey(), EModifierKey::FromBools(InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(), InKeyEvent.IsShiftDown(), InKeyEvent.IsCommandDown()));
				if (CurTool->SelectedTool)
				{
					if (!CurTool->SelectedTool->IsUnreachable())
					{
						IMeshMorpherToolInterface::Execute_OnKeyUnPressed(CurTool->SelectedTool, CheckChord);
						//UE_LOG(LogTemp, Warning, TEXT("Unpressed: %s"), *InKeyEvent.GetKey().ToString());
					}
				}
			}
		}
	}
	return false;
}

void FMeshMorpherToolkit::RequestManualClose()
{
	RequestClose();

	if (FSlateApplication::IsInitialized() && ThisPtr.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ThisPtr);
	}

	RequestCloseTab();
}


void FMeshMorpherToolkit::RequestClose()
{

	if (PreviewViewport.IsValid())
	{
		if(Settings)
		{
			Settings->SaveConfig();
		}
	}
	
	if (PoseSkeletalMeshComponent)
	{
		//PoseSkeletalMeshComponent->DestroyComponent();
		PoseSkeletalMeshComponent = nullptr;
	}
	if (ReferenceSkeletalMeshComponent)
	{
		//ReferenceSkeletalMeshComponent->DestroyComponent();
		ReferenceSkeletalMeshComponent = nullptr;
	}

	if (PreviewViewport.IsValid())
	{

		//PreviewViewport->RemoveComponent(PoseSkeletalMeshComponent);
		//PreviewViewport->RemoveComponent(ReferenceSkeletalMeshComponent);

		if (PreviewViewport->ViewportClient->GetModeTools())
		{
			PreviewViewport->ViewportClient->GetModeTools()->DeactivateMode(UMeshMorpherEdMode::EM_MeshMorpherEdModeId);
			PreviewViewport->ViewportClient->GetModeTools()->DestroyMode(UMeshMorpherEdMode::EM_MeshMorpherEdModeId);
		}

		if (PreviewViewport->PreviewScene.IsValid())
		{
			if (PreviewViewport->PreviewScene->GetWorld())
			{
				PreviewViewport->PreviewScene->GetWorld()->CleanupWorld();
			}
		}
	}

	if (GEngine)
	{
		GEngine->ForceGarbageCollection(true);
	}

	
}



void FMeshMorpherToolkit::OnWindowClosed(const TSharedRef<SWindow>& WindowBeingClosed)
{
	if (TrackedWindow.IsValid())
	{
		TrackedWindow.Reset();
		TrackedWindow = nullptr;
	}
}

void FMeshMorpherToolkit::FillFileMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().New);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().SaveButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().Exit);
}

void FMeshMorpherToolkit::FillEditMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().FocusCameraButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().OpenMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().DeselectMorphButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().AddMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().DeleteMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().RenameMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().DuplicateMorphButton);
	Builder.AddMenuSeparator();
	//Builder.AddMenuEntry(FMeshMorpherCommands::Get().StitchMorphButton);
	//Builder.AddMenuEntry(FMeshMorpherCommands::Get().EnableBuildDataButton);
	//Builder.AddMenuEntry(FMeshMorpherCommands::Get().DisableBuildDataButton);
	Builder.AddMenuSeparator();
	Builder.AddSubMenu(LOCTEXT("MaskSelectionMenu", "Selection"), LOCTEXT("MaskSelectionMenu_ToolTip", "Mask Selection Menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillSelectionMaskMenu), true, FSlateIcon(FName("MeshMorpherStyle"), FName("MeshMorpher.MaskSelectionMenu"), FName("MeshMorpher.MaskSelectionMenu.Small")));
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().SettingsButton);
}

void FMeshMorpherToolkit::FillSelectionMaskMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ToggleMaskSelectionButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ClearMaskSelectiontButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().OpenMaskSelectionButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().SaveSelectionButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().SaveSelectionToAssetButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ExportMaskSelectionOBJButton);
}

void FMeshMorpherToolkit::FillExportOBJMaskMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ExportOBJButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ExportMorphTargetOBJButton);
}


void FMeshMorpherToolkit::FillCreateMorphFromMetaMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMorphFromMetaButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMorphFromMetaCurrentButton);
}

void FMeshMorpherToolkit::FillToolsMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().SelectReferenceMeshButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CopyMorphButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMorphFromPoseButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMorphFromMeshButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMorphFromFBXButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().MergeMorphsButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ExportMorphButton);

	Builder.AddSubMenu(LOCTEXT("ExportOBJMenu", "Export to OBJ"), LOCTEXT("ExportOBJMenu_ToolTip", "Export to OBJ Menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillExportOBJMaskMenu), true, FSlateIcon(FName("MeshMorpherStyle"), FName("MeshMorpher.ExportOBJMenu"), FName("MeshMorpher.ExportOBJMenu.Small")));	

	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().BakeButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ApplyMorphTargetToLODsButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().MorphMagnitudeButton);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMetaMorphFromFBXButton);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().CreateMetaMorphFromMorphTargetButton);

	Builder.AddSubMenu(LOCTEXT("CreateMorphFromMetaMenu", "Create From Meta Morph"), LOCTEXT("CreateMorphFromMetaMenu_ToolTip", "Create From Meta Morph Menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillCreateMorphFromMetaMenu), true, FSlateIcon(FName("MeshMorpherStyle"), FName("MeshMorpher.CreateMorphFromMeta"), FName("MeshMorpher.CreateMorphFromMeta.Small")));	
}

void FMeshMorpherToolkit::FillWindowMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ToolbarTab);
	Builder.AddMenuSeparator();
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().PreviewTab);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().PoseTab);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().MorphTargetsTab);
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().ToolTab);
}

void FMeshMorpherToolkit::FillHelpMenu(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().About);
}

TSharedRef < SWidget > FMeshMorpherToolkit::GenerateWindow(TSharedPtr<FTabManager::FLayout> Layout)
{
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	TSharedPtr<SWidget> RestoredUI = TabManager->RestoreFrom(Layout.ToSharedRef(), ParentWindow);

	checkf(RestoredUI.IsValid(), TEXT("The layout must have a primary dock area"));

	TSharedPtr<FMenuBarBuilder> MenuBuilder = MakeShareable(new FMenuBarBuilder(UICommands));
	MenuBuilder->AddPullDownMenu(LOCTEXT("FileMenu", "File"), LOCTEXT("FileMenu_ToolTip", "Open the file menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillFileMenu));
	MenuBuilder->AddPullDownMenu(LOCTEXT("EditMenu", "Edit"), LOCTEXT("EditMenu_ToolTip", "Open the edit menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillEditMenu));
	MenuBuilder->AddPullDownMenu(LOCTEXT("ToolsMenu", "Tools"), LOCTEXT("ToolsMenu_ToolTip", "Open the tools menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillToolsMenu));
	MenuBuilder->AddPullDownMenu(LOCTEXT("WindowMenu", "Window"), LOCTEXT("WindowMenu_ToolTip", "Open the window menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillWindowMenu));
	MenuBuilder->AddPullDownMenu(LOCTEXT("HelpMenu", "Help"), LOCTEXT("HelpMenu_ToolTip", "Open the help menu"), FNewMenuDelegate::CreateRaw(this, &FMeshMorpherToolkit::FillHelpMenu));

#if PLATFORM_MAC
	TabManager->SetAllowWindowMenuBar(true);
	TabManager->SetMenuMultiBox(MenuBuilder->GetMultiBox(), MenuBuilder->MakeWidget());
#endif

	//FGlobalTabmanager::Get()
	return SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				[
					MenuBuilder->MakeWidget()
				]
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SBorder)
					.Padding(0)
					.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				]				
			]
		]
		+ SVerticalBox::Slot()
		.Padding(1.0f)
		.FillHeight(1.0f)
		[
			RestoredUI.ToSharedRef()
		];
}

UClass* FMeshMorpherToolkit::OnGetClassesForSourcePicker()
{
	return UObject::StaticClass();
}

bool FMeshMorpherToolkit::FilterSourcePicker(const FAssetData& AssetData) const
{
	if (AssetData.GetClass()->IsChildOf(USkeletalMesh::StaticClass()) || (AssetData.GetClass() == USkeletalMesh::StaticClass()))
	{
		return false;
	}
	return true;
}

bool FMeshMorpherToolkit::FilterPosePicker(const FAssetData& AssetData) const
{
	if (AssetData.GetClass()->IsChildOf(UAnimSequence::StaticClass()) || (AssetData.GetClass() == UAnimSequence::StaticClass()))
	{
		if (UAnimSequence* LocalPose = Cast<UAnimSequence>(AssetData.GetAsset()))
		{
			if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
			{
				USkeleton* Skeleton = LocalSource->GetSkeleton();
				if (Skeleton->IsCompatibleSkeletonByAssetData(AssetData))
				{
					return false;
				}
			}
		}
	}
	return true;
}


FString FMeshMorpherToolkit::GetSkeletalMeshAssetName() const
{
	if (SourceFile.IsValid())
	{
		return SourceFile.ObjectPath.ToString();
	}
	return FString("");
}

FString FMeshMorpherToolkit::GetPoseAssetName() const
{
	if (PoseFile.IsValid())
	{
		return PoseFile.ObjectPath.ToString();
	}
	return FString("");
}

void FMeshMorpherToolkit::OnSourceAssetSelected(const FAssetData& AssetData, bool bDestroyTrackedWindow)
{
	if (bDestroyTrackedWindow && TrackedWindow.IsValid())
	{
		TrackedWindow->RequestDestroyWindow();
	}
	PoseFile = NULL;
	CurrentPosePosition = 0.0f;
	MaxPosePosition = 0.0f;
	SourceFile = NULL;
	SelectedMorphTarget = nullptr;
	SelectedMorphTargetList.Empty();
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			PreviewViewport->GetEditorMode()->WeldedDynamicMesh.Clear();
			PreviewViewport->GetEditorMode()->IdenticalDynamicMesh.Clear();
			PreviewViewport->GetEditorMode()->UpdateSpatialData();
		}
	}
	if (AssetData.IsValid())
	{
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(AssetData.GetAsset()))
		{
			SelectSourceSkeletalMesh(LocalSource);
		}
	}
	else {
		PoseSkeletalMeshComponent->SetSkeletalMesh(nullptr);
		PoseSkeletalMeshComponent->CleanUpOverrideMaterials();
		PoseSkeletalMeshComponent->EmptyOverrideMaterials();
		if (PreviewViewport.IsValid())
		{
			if (PreviewViewport->GetEditorMode() != nullptr)
			{
				PreviewViewport->GetEditorMode()->CancelTool();
				USelection* Selection = PreviewViewport->GetEditorMode()->GetModeManager()->GetSelectedComponents();
				if (Selection->IsSelected(PoseSkeletalMeshComponent))
				{
					Selection->Deselect(PoseSkeletalMeshComponent);
				}
			}

		}

		if (MorphListView.IsValid())
		{
			MorphListView->SetSource(nullptr);
		}
	}
}


void FMeshMorpherToolkit::OnPoseAssetSelected(const FAssetData& AssetData)
{
	if (SelectedMorphTarget.IsValid())
	{
		SaveSelectedMorphTarget();
		CancelTool();
		SelectedMorphTarget = nullptr;
	}
	
	if (AssetData.IsValid())
	{
		if (UAnimSequence* LocalPose = Cast<UAnimSequence>(AssetData.GetAsset()))
		{
			if (AssetData != PoseFile)
			{
				CurrentPosePosition = 0.0f;
				MaxPosePosition = 0.0f;
				PoseFile = AssetData;
			}

			if (SourceFile.IsValid())
			{
				if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
				{
					PoseSkeletalMeshComponent->SetVisibility(false, true);
					PoseSkeletalMeshComponent->SetAnimation(LocalPose);
					MaxPosePosition = LocalPose->GetPlayLength();
					PoseSkeletalMeshComponent->SetPosition(MaxPosePosition * CurrentPosePosition, false);
					PoseSkeletalMeshComponent->SetVisibility(true, true);
					TArray<FFinalSkinVertex> Vertexes;
					PoseSkeletalMeshComponent->GetCPUSkinnedVertices(Vertexes, 0);
					ApplyPoseToReferenceMesh(PoseFile);

					if (PreviewViewport.IsValid())
					{
						if (PreviewViewport->GetEditorMode() != nullptr)
						{
							UMeshOperationsLibrary::SkeletalMeshToDynamicMesh(LocalSource, PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, &PreviewViewport->GetEditorMode()->WeldedDynamicMesh, Vertexes);
							PreviewViewport->GetEditorMode()->UpdateSpatialData();
						}
					}
				}
			}
		}
	}
	else {
		PoseFile = NULL;
		CurrentPosePosition = 0.0f;
		MaxPosePosition = 0.0f;
		if (SourceFile.IsValid())
		{
			if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
			{
				PoseSkeletalMeshComponent->SetVisibility(false, true);
				PoseSkeletalMeshComponent->SetAnimation(nullptr);
				ReferenceSkeletalMeshComponent->SetAnimation(nullptr);
				PoseSkeletalMeshComponent->SetVisibility(true, true);

				if (PreviewViewport.IsValid())
				{
					if (PreviewViewport->GetEditorMode() != nullptr)
					{
						TArray<FFinalSkinVertex> Vertexes;
						PoseSkeletalMeshComponent->GetCPUSkinnedVertices(Vertexes, 0);
						UMeshOperationsLibrary::SkeletalMeshToDynamicMesh(LocalSource, PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, &PreviewViewport->GetEditorMode()->WeldedDynamicMesh, Vertexes);
						PreviewViewport->GetEditorMode()->UpdateSpatialData();
					}
				}
			}
		}
	}
}

void FMeshMorpherToolkit::SetMasterPoseNull()
{
	if (PoseSkeletalMeshComponent)
	{
		PoseSkeletalMeshComponent->SetMasterPoseComponent(nullptr, true);
		PoseSkeletalMeshComponent->MarkRenderStateDirty();
		PoseSkeletalMeshComponent->MarkRenderDynamicDataDirty();
		PoseSkeletalMeshComponent->RecreateRenderState_Concurrent();
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			SelectSourceSkeletalMesh(LocalSource);

			const TArray<TSharedPtr<FString>> MorphTargetsSel = {SelectedMorphTarget};
			MorphTargetSelectionChanged(MorphTargetsSel, false, true, true);
		}
		OnPoseAssetSelected(PoseFile);
	}
	if (ReferenceSkeletalMeshComponent)
	{
		ReferenceSkeletalMeshComponent->SetMasterPoseComponent(nullptr, true);
		ReferenceSkeletalMeshComponent->MarkRenderStateDirty();
		ReferenceSkeletalMeshComponent->MarkRenderDynamicDataDirty();
		ReferenceSkeletalMeshComponent->RecreateRenderState_Concurrent();
	}
}

void FMeshMorpherToolkit::SetMasterPose()
{
	if (PoseSkeletalMeshComponent)
	{
		PoseSkeletalMeshComponent->SetMasterPoseComponent(nullptr, true);
		PoseSkeletalMeshComponent->MarkRenderStateDirty();
		PoseSkeletalMeshComponent->MarkRenderDynamicDataDirty();
		PoseSkeletalMeshComponent->RecreateRenderState_Concurrent();
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			SelectSourceSkeletalMesh(LocalSource);

			const TArray<TSharedPtr<FString>> MorphTargetsSel = {SelectedMorphTarget};
			MorphTargetSelectionChanged(MorphTargetsSel, false, true, true);
		}
		OnPoseAssetSelected(PoseFile);
	}
	if (ReferenceSkeletalMeshComponent && PoseSkeletalMeshComponent)
	{
		ReferenceSkeletalMeshComponent->SetMasterPoseComponent(PoseSkeletalMeshComponent, true);
		ReferenceSkeletalMeshComponent->MarkRenderStateDirty();
		ReferenceSkeletalMeshComponent->MarkRenderDynamicDataDirty();
		ReferenceSkeletalMeshComponent->RecreateRenderState_Concurrent();
	}
}

void FMeshMorpherToolkit::SetMasterPoseInv()
{
	if (ReferenceSkeletalMeshComponent)
	{
		ReferenceSkeletalMeshComponent->SetMasterPoseComponent(nullptr, true);
		ReferenceSkeletalMeshComponent->MarkRenderStateDirty();
		ReferenceSkeletalMeshComponent->MarkRenderDynamicDataDirty();
		ReferenceSkeletalMeshComponent->RecreateRenderState_Concurrent();
	}
	if (PoseSkeletalMeshComponent && ReferenceSkeletalMeshComponent)
	{
		PoseSkeletalMeshComponent->MarkRenderDynamicDataDirty();
		PoseSkeletalMeshComponent->SetMasterPoseComponent(ReferenceSkeletalMeshComponent, true);
		PoseSkeletalMeshComponent->MarkRenderStateDirty();
		PoseSkeletalMeshComponent->RecreateRenderState_Concurrent();
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			SelectSourceSkeletalMesh(LocalSource);
			
			const TArray<TSharedPtr<FString>> MorphTargetsSel = {SelectedMorphTarget};
			MorphTargetSelectionChanged(MorphTargetsSel, false, true, true);
		}
		OnPoseAssetSelected(PoseFile);
	}
}

void FMeshMorpherToolkit::ApplyPoseToReferenceMesh(const FAssetData& PoseAssetData)
{
	if (ReferenceSkeletalMeshComponent->SkeletalMesh)
	{
		if (PoseAssetData.IsValid())
		{
			if (UAnimSequence* LocalPose = Cast<UAnimSequence>(PoseAssetData.GetAsset()))
			{

				USkeleton* Skeleton = ReferenceSkeletalMeshComponent->SkeletalMesh->GetSkeleton();

				if (Skeleton->IsCompatibleSkeletonByAssetData(PoseAssetData))
				{
					ReferenceSkeletalMeshComponent->SetVisibility(false, true);
					ReferenceSkeletalMeshComponent->SetAnimation(LocalPose);
					ReferenceSkeletalMeshComponent->SetPosition(MaxPosePosition * CurrentPosePosition, false);
					//ReferenceSkeletalMeshComponent->Play(false);
					ReferenceSkeletalMeshComponent->SetVisibility(true, true);
					return;
				}
			}
		}
	}
	ReferenceSkeletalMeshComponent->SetAnimation(nullptr);
	//ReferenceSkeletalMeshComponent->Stop();
}


void FMeshMorpherToolkit::PosePositionChanged(float InValue)
{

	if (InValue != CurrentPosePosition)
	{
		CurrentPosePosition = InValue;
		OnPoseAssetSelected(PoseFile);
	}
}

void FMeshMorpherToolkit::SelectSourceSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
	if (SkeletalMesh)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		SourceFile = AssetRegistryModule.Get().GetAssetByObjectPath(*SkeletalMesh->GetPathName());
		if (SourceFile != NULL)
		{
			PoseSkeletalMeshComponent->SetVisibility(false, true);
			PoseSkeletalMeshComponent->SetSkeletalMesh(nullptr);
			PoseSkeletalMeshComponent->CleanUpOverrideMaterials();
			PoseSkeletalMeshComponent->EmptyOverrideMaterials();
			PoseSkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
			PoseSkeletalMeshComponent->SetAnimation(nullptr);
			ReferenceSkeletalMeshComponent->SetAnimation(nullptr);
			PoseSkeletalMeshComponent->SetVisibility(true, true);
			if (PreviewViewport.IsValid())
			{
				if (PreviewViewport->GetEditorMode() != nullptr)
				{
					
					TArray<FFinalSkinVertex> Vertexes;
					PoseSkeletalMeshComponent->GetCPUSkinnedVertices(Vertexes, 0);
					UMeshOperationsLibrary::SkeletalMeshToDynamicMesh(SkeletalMesh, PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, &PreviewViewport->GetEditorMode()->WeldedDynamicMesh, Vertexes);
					PreviewViewport->GetEditorMode()->UpdateSpatialData();

					PreviewViewport->GetEditorMode()->CancelTool();

					USelection* Selection = PreviewViewport->GetEditorMode()->GetModeManager()->GetSelectedComponents();
					if (!Selection->IsSelected(PoseSkeletalMeshComponent))
					{
						Selection->Select(PoseSkeletalMeshComponent);
					}
				}
			}
			if (MorphListView.IsValid())
			{
				MorphListView->SetSource(SkeletalMesh);
			}
		}
	}
}

bool FMeshMorpherToolkit::IsAssetValid() const
{
	if (SourceFile.IsValid())
	{
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			return true;
		}
	}
	return false;
}

bool FMeshMorpherToolkit::IsPoseBarEnabled() const
{
	if (SourceFile.IsValid() && PoseFile.IsValid())
	{
		return true;
	}
	return false;
}


bool FMeshMorpherToolkit::IsSaveAssetValid() const
{
	if (IsAssetValid())
	{
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			if (PreviewViewport.IsValid())
			{
				if (PreviewViewport->GetEditorMode() != nullptr)
				{
					if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
					{
						if (CurTool->DynamicMeshComponent)
						{
							if (CurTool->DynamicMeshComponent->bMeshChanged)
							{
								return true;
							}
						}
					}
				}
			}

			UPackage* Package = LocalSource->GetOutermost();

			if (Package != NULL)
			{
				return Package->IsDirty();
			}
		}
	}
	return false;
}


void FMeshMorpherToolkit::OnSaveAsset()
{

	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{

		if (GetParentWindow().IsValid())
		{
			GetParentWindow()->BringToFront(true);
		}

		SaveSelectedMorphTarget();
		UMeshOperationsLibrary::SaveSkeletalMesh(LocalSource);
	}
}

void FMeshMorpherToolkit::OpenNewToolkit()
{
	if (IMeshMorpherModule::Get().IsAvailable())
	{
		IMeshMorpherModule::Get().OpenToolkitAction();
	}
}

void FMeshMorpherToolkit::OnShowCreateMorphTargetWindow()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{

		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		TSharedPtr<SAddMorphTargetWidget> Widget;
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Create Morph Target")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			.IsTopmostWindow(true)
			.SizingRule(ESizingRule::Autosized)
			[
				SAssignNew(Widget, SAddMorphTargetWidget)
				.Source(LocalSource)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		TrackedWindow->SetWidgetToFocusOnActivate(Widget->TextBox);

		if (GetParentWindow().IsValid())
		{
			//FSlateApplication::Get().AddWindowAsNativeChild(MorphWindow, GetParentWindow().ToSharedRef(), true);
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), ToolkitPtr);
		}


	}
}

bool FMeshMorpherToolkit::IsSelectedMorphValid() const
{
	if (IsAssetValid() && SelectedMorphTarget.IsValid())
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsAnySelectedMorphValid() const
{
	if(IsAssetValid() && SelectedMorphTargetList.Num() > 0)
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsAnySelectedMorphValidJustOne() const
{
	if (IsAssetValid() && SelectedMorphTargetList.Num() == 1)
	{
		return true;
	}
	return false;
}

bool FMeshMorpherToolkit::IsToolOpen()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->DynamicMeshComponent)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool FMeshMorpherToolkit::HasSelectedTool()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool FMeshMorpherToolkit::HasMaskSelection()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->DynamicMeshComponent)
				{
					return (CurTool->DynamicMeshComponent->SelectedVertices.Num() > 0) || (CurTool->DynamicMeshComponent->SelectedTriangles.Num() > 0);
				}
			}
		}
	}
	return false;
}

bool FMeshMorpherToolkit::HasMaskSelectionAsset()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					if (!CurTool->SelectedTool->IsUnreachable())
					{

						UStandaloneMaskSelection* Selection = IMeshMorpherToolInterface::Execute_GetMaskSelectionAsset(CurTool->SelectedTool);
						if (Selection)
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool FMeshMorpherToolkit::IsMultiSelectedMorphValid() const
{
	if (IsAssetValid() && SelectedMorphTargetList.Num() > 1)
	{
		return true;
	}
	return false;
}

void FMeshMorpherToolkit::OnOpenSelected()
{
	if (SelectedMorphTargetList.Num() >= 1)
	{
		TArray<TSharedPtr<FString>> Selected;
		Selected.Add(SelectedMorphTargetList[0]);
		MorphTargetSelectionChanged(Selected, false, true, true);
	}
}

void FMeshMorpherToolkit::OnMorphTargetDeselect()
{
	if (MorphListView.IsValid())
	{
		if (MorphListView->MorphListView.IsValid())
		{
			MorphListView->MorphListView->ClearSelection();
		}

	}
	MorphTargetSelectionChanged(TArray<TSharedPtr<FString>>(), true, true, true);
}

void FMeshMorpherToolkit::OnRemoveMorphTarget()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource && SelectedMorphTargetList.Num() > 0)
	{

		auto result = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString("Are you sure?"));
		if (GetParentWindow().IsValid())
		{
			GetParentWindow()->BringToFront();
		}
		if (result == EAppReturnType::Yes)
		{
			if (SelectedMorphTargetList.Contains(SelectedMorphTarget))
			{
				CancelTool();
				SelectedMorphTarget = nullptr;
			}


			TArray<FString> MorphNames;
			for (auto& Morphtarget : SelectedMorphTargetList)
			{
				if (Morphtarget.IsValid())
				{
					MorphNames.Add(*Morphtarget);
				}
			}

			UMeshOperationsLibrary::RemoveMorphTargetsFromImportData(LocalSource, MorphNames);
			RefreshMorphList();

		}
	}
}

void FMeshMorpherToolkit::OnRenameMorphTarget()
{
	if (SelectedMorphTargetList.Num() > 0)
	{
		USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
		if (LocalSource && SelectedMorphTargetList[0].IsValid())
		{
			if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList[0]->Equals(*SelectedMorphTarget))
			{
				SaveSelectedMorphTarget();

				CancelTool();
				SelectedMorphTarget = nullptr;
			}

			if (TrackedWindow.IsValid())
			{
				TrackedWindow->RequestDestroyWindow();
			}

			TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
			TSharedPtr<SRenameMorphTargetWidget> Widget;
			SAssignNew(TrackedWindow, SWindow)
				.CreateTitleBar(true)
				.Title(FText::FromString(TEXT("Rename Morph Target: ") + *SelectedMorphTargetList[0]))
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.FocusWhenFirstShown(true)
				.IsTopmostWindow(true)
				.MinWidth(500.f)
				.SizingRule(ESizingRule::Autosized)
				[
					SAssignNew(Widget, SRenameMorphTargetWidget)
					.Source(LocalSource)
					.MorphTarget(SelectedMorphTargetList[0])
					.Toolkit(ToolkitPtr)
				];

			TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
			TrackedWindow->SetWidgetToFocusOnActivate(Widget->TextBox);

			if (GetParentWindow().IsValid())
			{
				FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
			}
		}
	}
}

void FMeshMorpherToolkit::OnDuplicateMorphTarget()
{
	if (SelectedMorphTargetList.Num() > 0)
	{
		USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
		if (LocalSource && SelectedMorphTargetList[0].IsValid())
		{
			if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList[0]->Equals(*SelectedMorphTarget))
			{
				SaveSelectedMorphTarget();
			}

			if (TrackedWindow.IsValid())
			{
				TrackedWindow->RequestDestroyWindow();
			}

			TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
			TSharedPtr<SDuplicateMorphTargetWidget> Widget;
			SAssignNew(TrackedWindow, SWindow)
				.CreateTitleBar(true)
				.Title(FText::FromString(TEXT("Duplicate Morph Target: ") + *SelectedMorphTargetList[0]))
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.FocusWhenFirstShown(true)
				.IsTopmostWindow(true)
				.MinWidth(500.f)
				.SizingRule(ESizingRule::Autosized)
				[
					SAssignNew(Widget, SDuplicateMorphTargetWidget)
					.Source(LocalSource)
					.MorphTarget(SelectedMorphTargetList[0])
					.Toolkit(ToolkitPtr)
				];

			TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
			TrackedWindow->SetWidgetToFocusOnActivate(Widget->TextBox);

			if (GetParentWindow().IsValid())
			{
				FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
			}
		}
	}
}

bool FMeshMorpherToolkit::IsBuildDataAvailable() const
{
	if (SourceFile.IsValid())
	{
		if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
		{
			return !LocalSource->IsLODImportedDataEmpty(0);
		}
	}
	return false;
}

void FMeshMorpherToolkit::OnEnableBuildData()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		EAppReturnType::Type bDialogResult = EAppReturnType::Yes;
		bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Don't use this functionality with Meta Humans Heads in Unreal Engine 4.27. It will break your Skeletal Mesh. Continue?"))));
		if (GetParentWindow().IsValid())
		{
			GetParentWindow()->BringToFront();
		}
		if (bDialogResult == EAppReturnType::Yes)
		{

			UMeshOperationsLibrary::SetEnableBuildData(LocalSource, true);

			FTextBuilder Builder;
			Builder.AppendLine(FString("Build Mesh Data has been enabled."));

			bDialogResult = FMessageDialog::Open(EAppMsgType::Ok, Builder.ToText());
			if (GetParentWindow().IsValid())
			{
				GetParentWindow()->BringToFront();
			}
		}
	}
}

void FMeshMorpherToolkit::OnDisableBuildData()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		EAppReturnType::Type bDialogResult = EAppReturnType::Yes;
		bDialogResult = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(FString::Printf(TEXT("Don't use this functionality with Meta Humans Heads in Unreal Engine 4.27. It will break your Skeletal Mesh. Continue?"))));
		if (GetParentWindow().IsValid())
		{
			GetParentWindow()->BringToFront();
		}
		if (bDialogResult == EAppReturnType::Yes)
		{

			UMeshOperationsLibrary::SetEnableBuildData(LocalSource, false);

			FTextBuilder Builder;
			Builder.AppendLine(FString("Build Mesh Data has been disabled."));

			bDialogResult = FMessageDialog::Open(EAppMsgType::Ok, Builder.ToText());
			if (GetParentWindow().IsValid())
			{
				GetParentWindow()->BringToFront();
			}
		}
	}
}

void FMeshMorpherToolkit::OnOpenSettings()
{
	const FName CategoryName("Plugins");
	const FName SectionName("Mesh Morpher");
	const FName ContainerName("Editor");

	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if(SettingsModule)
	SettingsModule->ShowViewer(ContainerName, CategoryName, SectionName);

}


void FMeshMorpherToolkit::OnCopyMorphTarget()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList.Contains(SelectedMorphTarget))
		{
			SaveSelectedMorphTarget();
		}

		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Copy Selected Morph Targets")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			//.IsTopmostWindow(true)
			.MinWidth(700.f)
			.MinHeight(583.f)
			.SizingRule(ESizingRule::FixedSize)
			[
				SNew(SCopyMorphTargetWidget)
				.Source(LocalSource)
				.Selection(SelectedMorphTargetList)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
		}
	}
}

void FMeshMorpherToolkit::OnSelectReferenceSkeletalMesh()
{
	TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());

	if (TrackedWindow.IsValid())
	{
		TrackedWindow->RequestDestroyWindow();
	}

	SAssignNew(TrackedWindow, SWindow)
		.CreateTitleBar(true)
		.Title(FText::FromString(TEXT("Select a Reference Skeletal Mesh")))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.FocusWhenFirstShown(true)
		.IsTopmostWindow(false)
		.ClientSize(FVector2D(800, 400))
		.SizingRule(ESizingRule::UserSized)
		[
			SNew(SSelectReferenceSkeletalMeshWidget)
			.Toolkit(ToolkitPtr)
		];
	TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
	if (GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(TrackedWindow.ToSharedRef(), GetParentWindow().ToSharedRef(), true);
	}
}

void FMeshMorpherToolkit::OnMergeMorphTargets()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList.Contains(SelectedMorphTarget))
		{
			SaveSelectedMorphTarget();

			CancelTool();
			SelectedMorphTarget = nullptr;

		}

		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());

		TSharedPtr<SMergeMorphTargetsWidget> Widget;
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Merge selected Morph Targets")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			//.IsTopmostWindow(true)
			.MinWidth(500.f)
			.SizingRule(ESizingRule::Autosized)
			[
				SAssignNew(Widget, SMergeMorphTargetsWidget)
				.Source(LocalSource)
			    .Selection(SelectedMorphTargetList)
				.Toolkit(ToolkitPtr)
			];

		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		TrackedWindow->SetWidgetToFocusOnActivate(Widget->TextBox);

		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
		}
	}
}

void FMeshMorpherToolkit::OnExportMorphTarget()
{
	if (SelectedMorphTargetList.Num() > 0)
	{
		USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
		if (LocalSource && SelectedMorphTargetList[0].IsValid())
		{
			if (PreviewViewport.IsValid())
			{
				if (PreviewViewport->GetEditorMode() != nullptr)
				{

					if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList[0]->Equals(*SelectedMorphTarget))
					{
						SaveSelectedMorphTarget();
					}

					TArray<FStaticMaterial> StaticMaterials;
					UMeshOperationsLibrary::CopySkeletalMeshMaterials(LocalSource, StaticMaterials);

					FDynamicMesh3 LocalDynamicMesh = PreviewViewport->GetEditorMode()->WeldedDynamicMesh;

					TArray<FMorphTargetDelta> Deltas;
					UMeshOperationsLibrary::GetMorphTargetDeltas(LocalSource, *SelectedMorphTargetList[0], Deltas);
					UMeshOperationsLibrary::ApplyDeltasToDynamicMesh(PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, Deltas, LocalDynamicMesh);

					FMeshDescription MeshDescription;
					MeshDescription.Empty();
					FStaticMeshAttributes StaticMeshAttributes(MeshDescription);
					StaticMeshAttributes.Register();

					FDynamicMeshToMeshDescription Converter;
					Converter.Convert(&LocalDynamicMesh, MeshDescription);

					UMeshOperationsLibrary::ExportMorphTargetToStaticMesh(*SelectedMorphTargetList[0], MeshDescription, StaticMaterials);
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnExportMeshObj()
{
	TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());

	if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
	{
		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Export to OBJ File (Experimental)...")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			.IsTopmostWindow(false)
			.MinWidth(700.f)
			.MaxWidth(700.f)
			.SizingRule(ESizingRule::Autosized)
			[
				SNew(SExportOBJWidget)
				.Source(LocalSource)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(TrackedWindow.ToSharedRef(), GetParentWindow().ToSharedRef(), true);
		}
	}
}


void FMeshMorpherToolkit::OnClearMaskSelection()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					IMeshMorpherToolInterface::Execute_ClearMaskSelection(CurTool->SelectedTool);
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnToggleMaskSelection()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					IMeshMorpherToolInterface::Execute_ToggleMaskSelection(CurTool->SelectedTool);
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnIncreaseBrushSize()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					IMeshMorpherToolInterface::Execute_IncreaseBrushSize(CurTool->SelectedTool);
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnDecreaseBrushSize()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					IMeshMorpherToolInterface::Execute_DecreaseBrushSize(CurTool->SelectedTool);
				}
			}
		}
	}
}


void FMeshMorpherToolkit::OnExportSelectionToAsset()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{

				if (CurTool->DynamicMeshComponent && CurTool->TargetMeshComponent->SkeletalMesh)
				{
					if (CurTool->SelectedTool)
					{

						if (CurTool->DynamicMeshComponent->GetVertexCount() || CurTool->DynamicMeshComponent->GetTriangleCount())
						{

							FString PackageName = FString(TEXT("/Game/MaskSelections/")) + "MaskSelection";
							FString Name;
							FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
							AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

							TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
								SNew(SDlgPickAssetPath)
								.Title(FText::FromString(FString("Choose New Mask Selection Location")))
								.DefaultAssetPath(FText::FromString(PackageName));
							if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
							{
								FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
								FName MaskSelectionName(*FPackageName::GetLongPackageAssetName(UserPackageName));

								// Check if the user inputed a valid asset name, if they did not, give it the generated default name
								if (MaskSelectionName == NAME_None)
								{
									// Use the defaults that were already generated.
									UserPackageName = PackageName;
									MaskSelectionName = *Name;
								}

								// Then find/create it.
								UPackage* Package = CreatePackage(*UserPackageName);
								check(Package);

								// Create SelectionObject object
								UStandaloneMaskSelection* NewMaskSelection = NewObject<UStandaloneMaskSelection>(Package, MaskSelectionName, RF_Public | RF_Standalone);
								IMeshMorpherToolInterface::Execute_SaveMaskSelection(CurTool->SelectedTool, NewMaskSelection);

								NewMaskSelection->PostLoad();
								NewMaskSelection->MarkPackageDirty();
								// Notify asset registry of new asset
								FAssetRegistryModule::AssetCreated(NewMaskSelection);

							}
						}
					}
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnSaveSelectionAsset()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					if (!CurTool->SelectedTool->IsUnreachable())
					{

						UStandaloneMaskSelection* Selection = IMeshMorpherToolInterface::Execute_GetMaskSelectionAsset(CurTool->SelectedTool);
						if (Selection)
						{
							IMeshMorpherToolInterface::Execute_SaveMaskSelection(CurTool->SelectedTool, Selection);
							Selection->MarkPackageDirty();

							TArray< UPackage*> Packages;

							UPackage* Pkg = Cast<UPackage>(Selection->GetOuter());
							if (Pkg)
							{
								Packages.Add(Pkg);
							}

							FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, true, nullptr, false, true);
						}
					}
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnOpenSelectionFromAsset()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{

					if (TrackedWindow.IsValid())
					{
						TrackedWindow->RequestDestroyWindow();
					}

					TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
					SAssignNew(TrackedWindow, SWindow)
						.CreateTitleBar(true)
						.Title(FText::FromString(TEXT("Open Mask Selection")))
						.SupportsMaximize(false)
						.SupportsMinimize(false)
						.FocusWhenFirstShown(true)
						//.IsTopmostWindow(true)
						.MinWidth(700.f)
						.SizingRule(ESizingRule::Autosized)
						[
							SNew(SOpenMaskSelectionWidget)
							.Tool(CurTool->SelectedTool)
							.Toolkit(ToolkitPtr)
						];
					TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
					if (GetParentWindow().IsValid())
					{
						FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
					}
				}
			}
		}
	}
}

void FMeshMorpherToolkit::OnExportMaskSelectionOBJ()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
			{
				if (CurTool->SelectedTool)
				{
					if (CurTool->DynamicMeshComponent)
					{
						if ((CurTool->DynamicMeshComponent->SelectedVertices.Num() > 0) || (CurTool->DynamicMeshComponent->SelectedTriangles.Num() > 0))
						{
							const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
							IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
							if (DesktopPlatform && ParentWindowHandle)
							{
								//Opening the file picker!
								uint32 SelectionFlag = 0; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
								TArray<FString> OutFileNames;
								const FString Title = "Export Selection to OBJ...";
								const bool bResult = DesktopPlatform->SaveFileDialog(ParentWindowHandle, Title, "", "", "OBJ Files|*.OBJ", SelectionFlag, OutFileNames);
								if (bResult && OutFileNames.Num() > 0)
								{
									IMeshMorpherToolInterface::Execute_ExportToFile(CurTool->SelectedTool, OutFileNames[0]);
								}
							}
						}
					}
				}
			}
		}
	}
}


void FMeshMorpherToolkit::OnExportMorphTargetOBJ()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
			{
				if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList[0]->Equals(*SelectedMorphTarget))
				{
					SaveSelectedMorphTarget();
				}
			
				const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
				IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
				if (DesktopPlatform && ParentWindowHandle)
				{
					//Opening the file picker!
					uint32 SelectionFlag = 0; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
					TArray<FString> OutFileNames;
					const FString Title = "Export Morph Target to OBJ...";
					const bool bResult = DesktopPlatform->SaveFileDialog(ParentWindowHandle, Title, "", "", "OBJ Files|*.OBJ", SelectionFlag, OutFileNames);

					if (bResult && OutFileNames.Num() > 0)
					{
						TArray<FSkeletalMaterial> Materials;
						if(Settings)
						{
							if(Settings->ExportMaterialSlotsAsSections)
							{
								UMeshOperationsLibraryRT::GetUsedMaterials(LocalSource, 0, Materials);
							}
						}

						FDynamicMesh3 LocalDynamicMesh;
						FDynamicMesh3 LocalWeldedDynamicMesh;
						UMeshOperationsLibrary::SkeletalMeshToDynamicMesh(LocalSource, LocalDynamicMesh, &LocalWeldedDynamicMesh);
						
						TArray<FMorphTargetDelta> Deltas;
						UMeshOperationsLibrary::GetMorphTargetDeltas(LocalSource, *SelectedMorphTargetList[0], Deltas);

						UMeshOperationsLibrary::ApplyDeltasToDynamicMesh(LocalDynamicMesh, Deltas, LocalWeldedDynamicMesh);
						UMeshMorpherToolHelper::ExportMeshToOBJFile(&LocalWeldedDynamicMesh, TSet<int32>(), OutFileNames[0], false, Materials);
					}
				}
			}
		}
	}
}


void FMeshMorpherToolkit::OnFocusCamera()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			PreviewViewport->GetEditorMode()->FocusCameraAtCursorHotkey();
		}
	}
}

void FMeshMorpherToolkit::OnBake()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		SaveSelectedMorphTarget();
		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Bake")))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			.FocusWhenFirstShown(true)
			.IsTopmostWindow(false)
			.ClientSize(FVector2D(800, 400))
			.SizingRule(ESizingRule::UserSized)
			[
				SNew(SBakeSkeletalMeshWidget)
				.Source(LocalSource)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(TrackedWindow.ToSharedRef(), GetParentWindow().ToSharedRef(), true);
		}
	}
}

void FMeshMorpherToolkit::OnCreateMorphTargetFromPose()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Create a Morph target from selected Pose")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			//.IsTopmostWindow(true)
			.MinWidth(700.f)
			.SizingRule(ESizingRule::Autosized)
			[
				SNew(SCreateMorphTargetFromPoseWidget)
				.Source(LocalSource)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
		}
	}
}


void FMeshMorpherToolkit::OnCreateMorphTargetFromMesh()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Create a Morph target from a Mesh")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			//.IsTopmostWindow(true)
			.MinWidth(700.f)
			.SizingRule(ESizingRule::Autosized)
			[
				SNew(SCreateMorphTargetFromMeshWidget)
				.Target(LocalSource)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
		}
	}
}


void FMeshMorpherToolkit::OnCreateMorphTargetFromFBX()
{
	if (USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset()))
	{
		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Create a Morph Target from Mesh Files (Experimental)")))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			.FocusWhenFirstShown(true)
			.IsTopmostWindow(false)
			.ClientSize(FVector2D(800, 400))
			.SizingRule(ESizingRule::UserSized)
			[
				SNew(SCreateMorphTargetFromFBXWidget)
				.Target(LocalSource)
				.Toolkit(ToolkitPtr)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(TrackedWindow.ToSharedRef(), GetParentWindow().ToSharedRef(), true);
		}
	}
}

void FMeshMorpherToolkit::OnCreateMetaMorphTargetFromFBX()
{

	if (TrackedWindow.IsValid())
	{
		TrackedWindow->RequestDestroyWindow();
	}

	TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
	SAssignNew(TrackedWindow, SWindow)
		.CreateTitleBar(true)
		.Title(FText::FromString(TEXT("Create a Meta Morph from Mesh Files (Experimental)")))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.FocusWhenFirstShown(true)
		.IsTopmostWindow(false)
		.ClientSize(FVector2D(800, 400))
		.SizingRule(ESizingRule::UserSized)
		[
			SNew(SCreateMetaMorphFromFBXWidget)
			.Toolkit(ToolkitPtr)
		];
	TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
	if (GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(TrackedWindow.ToSharedRef(), GetParentWindow().ToSharedRef(), true);
	}
}

void FMeshMorpherToolkit::OnCreateMetaMorphTargetFromMorphTargets()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList.Contains(SelectedMorphTarget))
		{
			SaveSelectedMorphTarget();
		}


		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Create a Meta Morph from Selected Morph Targets (Experimental)")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			//.IsTopmostWindow(true)
			.MinWidth(700.f)
			.SizingRule(ESizingRule::Autosized)
			[
				SNew(SCreateMetaMorphFromMorphsWidget)
				.Toolkit(ToolkitPtr)
			.Source(LocalSource)
			.Selection(SelectedMorphTargetList)
			];
		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
		}
	}
}

void FMeshMorpherToolkit::OnCreateMorphFromMeta(bool bNoTarget)
{
	if (TrackedWindow.IsValid())
	{
		TrackedWindow->RequestDestroyWindow();
	}
	TSet<USkeletalMesh*> Targets;
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		Targets.Add(LocalSource);
	}

	TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
	SAssignNew(TrackedWindow, SWindow)
		.CreateTitleBar(true)
		.Title(FText::FromString(TEXT("Create Morph Target(s) from Meta Morph (Experimental)")))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.FocusWhenFirstShown(true)
		//.IsTopmostWindow(true)
		.MinWidth(700.f)
		.SizingRule(ESizingRule::Autosized)
		[
			SNew(SCreateMorphTargetFromMetaWidget)
			.Toolkit(ToolkitPtr)
			.NoTarget(bNoTarget)
			.Targets(Targets)
		];
	TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
	if (GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
	}
}


void FMeshMorpherToolkit::OnApplyMorphTargetToLODs()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		SaveSelectedMorphTarget();

		FFormatNamedArguments Args;
		Args.Add(TEXT("SkeletalMesh"), FText::FromString(LocalSource->GetName()));
		const FText StatusUpdate = FText::Format(LOCTEXT("ApplyMorphTargetsToLODs", "({SkeletalMesh}) Applying Morph Target(s)..."), Args);
		GWarn->BeginSlowTask(StatusUpdate, true);
		if (!GWarn->GetScopeStack().IsEmpty())
		{
			GWarn->GetScopeStack().Last()->MakeDialog(false, true);
		}
		int32 Current = 0;
		for (auto& MorphTarget : SelectedMorphTargetList)
		{
			Current++;
			if (MorphTarget.IsValid())
			{
				FFormatNamedArguments MorphArg;
				MorphArg.Add(TEXT("MorphTarget"), FText::FromString(*MorphTarget));
				const FText MorphUpdate = FText::Format(LOCTEXT("ApplyMorphTargetToLODs", "({MorphTarget}) Applying to LODs..."), MorphArg);
				GWarn->StatusUpdate(Current, SelectedMorphTargetList.Num(), MorphUpdate);

				TArray<FMorphTargetDelta> Deltas;
				UMeshOperationsLibrary::GetMorphTargetDeltas(LocalSource, *MorphTarget, Deltas, 0);
				UMeshOperationsLibrary::ApplyMorphTargetToImportData(LocalSource, *MorphTarget, Deltas, false);
			}
		}
		GWarn->EndSlowTask();
		LocalSource->InitMorphTargetsAndRebuildRenderData();
	}
}

void FMeshMorpherToolkit::OnUpdateMagnitudeMorphTarget()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource)
	{
		if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList.Contains(SelectedMorphTarget))
		{
			SaveSelectedMorphTarget();

			CancelTool();
			SelectedMorphTarget = nullptr;

		}

		if (TrackedWindow.IsValid())
		{
			TrackedWindow->RequestDestroyWindow();
		}

		TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());

		TSharedPtr<SMagnitudeMorphTargetWidget> Widget;
		SAssignNew(TrackedWindow, SWindow)
			.CreateTitleBar(true)
			.Title(FText::FromString(TEXT("Change Magnitude of selected Morph Target(s)")))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.FocusWhenFirstShown(true)
			//.IsTopmostWindow(true)
			.MinWidth(500.f)
			.SizingRule(ESizingRule::Autosized)
			[
				SAssignNew(Widget, SMagnitudeMorphTargetWidget)
				.Source(LocalSource)
				.Selection(SelectedMorphTargetList)
				.Toolkit(ToolkitPtr)
			];

		TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));

		TrackedWindow->SetWidgetToFocusOnActivate(Widget->NumericBox);

		if (GetParentWindow().IsValid())
		{
			FSlateApplication::Get().AddModalWindow(TrackedWindow.ToSharedRef(), GetParentWindow());
		}
	}
}


void FMeshMorpherToolkit::OnStitchMorphTarget()
{
	if (SelectedMorphTargetList.Num() > 0)
	{
		USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
		if (LocalSource && SelectedMorphTargetList[0].IsValid())
		{
			if (SelectedMorphTarget.IsValid() && SelectedMorphTargetList[0]->Equals(*SelectedMorphTarget))
			{
				SaveSelectedMorphTarget();

				CancelTool();
				SelectedMorphTarget = nullptr;
			}

			if (TrackedWindow.IsValid())
			{
				TrackedWindow->RequestDestroyWindow();
			}

			TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(this->AsShared());
			SAssignNew(TrackedWindow, SWindow)
				.CreateTitleBar(true)
				.Title(FText::FromString(TEXT("Stitch Morph Target")))
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.FocusWhenFirstShown(true)
				//.IsTopmostWindow(true)
				.MinWidth(700.f)
				.MinHeight(583.f)
				.SizingRule(ESizingRule::FixedSize)
				[
					SNew(SStitchMorphTargetWidget)
					.Source(LocalSource)
					.MorphTarget(SelectedMorphTargetList[0])
					.Toolkit(ToolkitPtr)
				];

			TrackedWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMeshMorpherToolkit::OnWindowClosed));
			if (GetParentWindow().IsValid())
			{
				FSlateApplication::Get().AddWindowAsNativeChild(TrackedWindow.ToSharedRef(), GetParentWindow().ToSharedRef(), true);
			}
		}
	}
}



void FMeshMorpherToolkit::SaveSelectedMorphTarget()
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (SelectedMorphTarget.IsValid() && LocalSource)
	{
		if (PreviewViewport.IsValid())
		{
			if (PreviewViewport->GetEditorMode() != nullptr)
			{
				if (UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(PreviewViewport->GetEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left)))
				{
					if (CurTool->DynamicMeshComponent)
					{
						if (CurTool->DynamicMeshComponent->bMeshChanged)
						{

							FDynamicMesh3 ChangedMesh = *CurTool->GetMesh();

							UMeshOperationsLibrary::ApplyChangesToMorphTarget(LocalSource, PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, *SelectedMorphTarget, PreviewViewport->GetEditorMode()->WeldedDynamicMesh, ChangedMesh);

							CurTool->DynamicMeshComponent->bMeshChanged = false;
						}
					}
				}
			}
		}
	}
}

void FMeshMorpherToolkit::MorphTargetSelectionChanged(const TArray<TSharedPtr<FString>>& InItem, bool UpdateSelectionList, bool SaveMorphTargetChanges, bool bAutoLoadMorphTarget)
{
	if (UpdateSelectionList)
	{
		SelectedMorphTargetList = InItem;
	}

	if (SaveMorphTargetChanges)
	{
		SaveSelectedMorphTarget();
	}

	if (UpdateSelectionList && !bAutoLoadMorphTarget)
	{
		return;
	}

	if (InItem.Num() == 1 && InItem[0].IsValid())
	{

		if (bAutoLoadMorphTarget)
		{
			SelectedMorphTarget = InItem[0];
			if (PreviewViewport.IsValid())
			{
				if (PreviewViewport->GetEditorMode() != nullptr)
				{
					PreviewViewport->GetEditorMode()->ModifiedDynamicMesh = PreviewViewport->GetEditorMode()->WeldedDynamicMesh;
					TArray<FMorphTargetDelta> Deltas;
					USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
					if (LocalSource)
					{
						UMeshOperationsLibrary::GetMorphTargetDeltas(LocalSource, *SelectedMorphTarget, Deltas);
					}
					UMeshOperationsLibrary::ApplyDeltasToDynamicMesh(PreviewViewport->GetEditorMode()->IdenticalDynamicMesh, Deltas, PreviewViewport->GetEditorMode()->ModifiedDynamicMesh);

					PreviewViewport->GetEditorMode()->CancelTool();
					PreviewViewport->GetEditorMode()->EnableTool();


					FSlateApplication::Get().ForEachUser([&](FSlateUser& User) {
						FSlateApplication::Get().SetUserFocus(User.GetUserIndex(), PreviewViewport, EFocusCause::SetDirectly);
					});

				}
			}
		}

	}
	else {
		CancelTool();
		SelectedMorphTarget = nullptr;
	}
}

void FMeshMorpherToolkit::OnAssetChanged(UObject* ChangedAsset)
{
	USkeletalMesh* LocalSource = Cast<USkeletalMesh>(SourceFile.GetAsset());
	if (LocalSource && !LocalSource->IsUnreachable() && (LocalSource == ChangedAsset) && PreviewViewport.IsValid() && PreviewViewport->GetEditorMode() != nullptr)
	{
		RefreshMorphList();

		OnPoseAssetSelected(PoseFile);
	}
	else if (LocalSource && LocalSource->IsUnreachable() && (LocalSource == ChangedAsset) && PreviewViewport.IsValid() && PreviewViewport->GetEditorMode() != nullptr)
	{
		CancelTool();
		if (MorphListView.IsValid())
		{
			MorphListView->SetSource(nullptr);
		}
	}
}

void FMeshMorpherToolkit::CancelTool()
{
	if (PreviewViewport.IsValid())
	{
		if (PreviewViewport->GetEditorMode() != nullptr)
		{
			PreviewViewport->GetEditorMode()->CancelTool();
		}
	}
}

void FMeshMorpherToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PoseSkeletalMeshComponent);
	Collector.AddReferencedObject(ReferenceSkeletalMeshComponent);
	Collector.AddReferencedObject(Settings);
}

void FMeshMorpherToolkit::RefreshMorphList()
{
	if (MorphListView.IsValid())
	{
		MorphListView->Refresh();
	}
}

void FMeshMorpherToolkit::SetReferenceMesh(USkeletalMesh* SkeletalMesh)
{
	if (ReferenceSkeletalMeshComponent)
	{
		ReferenceSkeletalMeshComponent->CleanUpOverrideMaterials();
		ReferenceSkeletalMeshComponent->EmptyOverrideMaterials();
		
		if (ReferenceSkeletalMeshComponent->SkeletalMesh != SkeletalMesh)
		{
			ReferenceSkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);

			ApplyPoseToReferenceMesh(PoseFile);
		}
	}
}

void FMeshMorpherToolkit::SetReferenceMaterials(const TArray<UMaterialInterface*>& Materials)
{
	if (ReferenceSkeletalMeshComponent)
	{
		ReferenceSkeletalMeshComponent->CleanUpOverrideMaterials();
		ReferenceSkeletalMeshComponent->EmptyOverrideMaterials();

		if (ReferenceSkeletalMeshComponent->SkeletalMesh)
		{
			for (int32 MatIndex = 0; MatIndex < Materials.Num(); ++MatIndex)
			{
				ReferenceSkeletalMeshComponent->SetMaterial(MatIndex, Materials[MatIndex]);
			}
		}
	}
}

void FMeshMorpherToolkit::SetReferenceTransform(const FTransform& Transform)
{
	if (ReferenceSkeletalMeshComponent)
	{
		ReferenceSkeletalMeshComponent->SetWorldTransform(Transform);
	}
}


#undef LOCTEXT_NAMESPACE

