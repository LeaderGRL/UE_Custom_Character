// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "EditorUndoClient.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Framework/Application/IInputProcessor.h"
#include "Widgets/Docking/SDockTab.h"
#include "UObject/GCObject.h"
#include "IDetailRootObjectCustomization.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif
#include "DynamicMesh/DynamicMesh3.h"


class SWindow;
class SWidget;
class SMorphTargetListWidget;
class SMeshMorpherToolWidget;
class FReply;
struct FAssetData;
class FMeshMorpherEdMode;
class UObject;
class UClass;
class FAssetThumbnailPool;
class FUICommandList;
class SMeshMorpherPreviewViewport;
class FAdvancedPreviewScene;
class USkeletalMesh;
class UMaterialInterface;
class UMeshMorpherTransformGizmo;
class Actor;
class UMeshMorpherSettings;

/**
 * Customization for Tool properties multi-object DetailsView, that just hides per-object header
 */
class FMeshMorpherDetailRootObjectCustomization : public IDetailRootObjectCustomization
{
public:
	FMeshMorpherDetailRootObjectCustomization()
	{
	}

	virtual TSharedPtr<SWidget> CustomizeObjectHeader(const FDetailsObjectSet& InRootObjectSet) override
	{
		return SNew(STextBlock).Text(FText::FromString(InRootObjectSet.RootObjects[0]->GetName()));
	}

	virtual bool AreObjectsVisible(const FDetailsObjectSet& InRootObjectSet) const override
	{
		return true;
	}

	virtual bool ShouldDisplayHeader(const FDetailsObjectSet& InRootObjectSet) const override
	{
		return false;
	}
};


class FMeshMorpherToolkit : public FEditorUndoClient, public IInputProcessor, public SDockTab, public FGCObject
{
	friend class FMeshMorpherModule;
	friend class SCreateMorphTargetFromPoseWidget;
	friend class SCreateMorphTargetFromFBXWidget;
	friend class SCreateMorphTargetFromMetaWidget;
public:

	void Construct(const FArguments& InArgs, USkeletalMesh* SkeletalMesh);
	FText GetTitle() const;
	virtual ~FMeshMorpherToolkit();
	void OnAssetChanged(UObject* ChangedAsset);
	void CancelTool();
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	void RefreshMorphList();
	void SetReferenceMesh(USkeletalMesh* SkeletalMesh);
	void SetReferenceMaterials(const TArray<UMaterialInterface*>& Materials = TArray<UMaterialInterface*>());
	void SetReferenceTransform(const FTransform& Transform = FTransform::Identity);
	void SetMasterPoseNull();
	void SetMasterPose();
	void SetMasterPoseInv();
private:
	void HandleTabManagerPersistLayout(const TSharedRef<FTabManager::FLayout>& LayoutToSave);
	void RegisterTabSpawners(TSharedPtr<FTabManager>& InTabManager);
	TSharedRef<SDockTab> SpawnTab_Toolbar(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Preview(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Pose(const FSpawnTabArgs& Args);
	
	TSharedRef<SDockTab> SpawnTab_Tool(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_MorphTargets(const FSpawnTabArgs& Args);
	bool IsToolbarTabValid() const;
	bool IsPoseTabValid() const;
	bool IsPreviewTabValid() const;
	bool IsToolTabValid() const;
	bool IsMorphTargetsTabValid() const;
	void ToggleToolbarTab();
	void TogglePoseTab();
	void TogglePreviewTab();
	void ToggleToolTab();
	void ToggleMorphTargetsTab();
	void ShowAboutWindow();

protected:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> InCursor) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	void RequestManualClose();
	void RequestClose();

	void OnWindowClosed(const TSharedRef<SWindow>& WindowBeingClosed);

	virtual FString GetReferencerName() const override
	{
		return "MeshMorpherToolkit" + FString::FromInt(FMath::Rand());
	}

private:

	void FillFileMenu(FMenuBuilder& Builder);
	void FillEditMenu(FMenuBuilder& Builder);
	void FillSelectionMaskMenu(FMenuBuilder& Builder);
	void FillExportOBJMaskMenu(FMenuBuilder& Builder);
	void FillCreateMorphFromMetaMenu(FMenuBuilder& Builder);
	void FillToolsMenu(FMenuBuilder& Builder);
	void FillWindowMenu(FMenuBuilder& Builder);
	void FillHelpMenu(FMenuBuilder& Builder);
	TSharedRef<SWidget> GenerateWindow(TSharedPtr<FTabManager::FLayout> Layout);
	UClass* OnGetClassesForSourcePicker();
	bool FilterSourcePicker(const FAssetData& AssetData) const;
	bool FilterPosePicker(const FAssetData& AssetData) const;
	FString GetSkeletalMeshAssetName() const;
	FString GetPoseAssetName() const;
	void OnSourceAssetSelected(const FAssetData& AssetData, bool bDestroyTrackedWindow = true);
	void OnPoseAssetSelected(const FAssetData& AssetData);
	void ApplyPoseToReferenceMesh(const FAssetData& PoseAssetData);
	void PosePositionChanged(float InValue);
	void PosePositionComitted(float NewPosition, ETextCommit::Type CommitType);
	void SelectSourceSkeletalMesh(USkeletalMesh* SkeletalMesh);
	bool IsAssetValid() const;
	bool IsPoseBarEnabled() const;
	bool IsSaveAssetValid() const;
	void OnSaveAsset();
	void OpenNewToolkit();
	void OnShowCreateMorphTargetWindow();
	bool IsSelectedMorphValid() const;

	bool IsAnySelectedMorphValid() const;

	bool IsAnySelectedMorphValidJustOne() const;

	bool IsToolOpen();

	bool HasSelectedTool();

	bool HasMaskSelection();

	bool HasMaskSelectionAsset();

	bool IsMultiSelectedMorphValid() const;

	void OnOpenSelected();
	void OnMorphTargetDeselect();
	void OnRemoveMorphTarget();
	void OnRenameMorphTarget();
	void OnDuplicateMorphTarget();
	bool IsBuildDataAvailable() const;
	void OnEnableBuildData();
	void OnDisableBuildData();
	void OnOpenSettings();
	void OnCopyMorphTarget();
	void OnSelectReferenceSkeletalMesh();
	void OnMergeMorphTargets();
	void OnExportMorphTarget();
	void OnExportMeshObj();

	void OnClearMaskSelection();

	void OnToggleMaskSelection();

	void OnIncreaseBrushSize();

	void OnDecreaseBrushSize();

	void OnExportSelectionToAsset();
	void OnSaveSelectionAsset();
	void OnOpenSelectionFromAsset();

	void OnExportMaskSelectionOBJ();
	void OnExportMorphTargetOBJ();

	void OnFocusCamera();

	void OnBake();
	void OnCreateMorphTargetFromPose();
	void OnCreateMorphTargetFromMesh();
	void OnCreateMorphTargetFromFBX();
	void OnCreateMetaMorphTargetFromFBX();
	void OnCreateMetaMorphTargetFromMorphTargets();
	void OnCreateMorphFromMeta(bool bNoTarget = false);
	void OnApplyMorphTargetToLODs();
	void OnUpdateMagnitudeMorphTarget();
	void OnStitchMorphTarget();
	void SaveSelectedMorphTarget();
	void MorphTargetSelectionChanged(const TArray<TSharedPtr<FString>>& InItem, bool UpdateSelectionList = false, bool SaveMorphTargetChanges = true, bool bAutoLoadMorphTarget = false);

public:
	FAssetData SourceFile = NULL;
	FAssetData PoseFile = NULL;
	TSharedPtr<FString> SelectedMorphTarget = nullptr;
	TArray<TSharedPtr<FString>> SelectedMorphTargetList;
	USkeletalMeshComponent* PoseSkeletalMeshComponent = nullptr;
	USkeletalMeshComponent* ReferenceSkeletalMeshComponent = nullptr;
	TSharedPtr <SWindow> TrackedWindow = nullptr;
private:
	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
	float CurrentPosePosition = 0.0f;
	float MaxPosePosition = 0.0f;
	TSharedPtr<FTabManager> TabManager;
	TSharedPtr<class FUICommandList> UICommands;
	TSharedPtr<class SMorphTargetListWidget> MorphListView;
	TSharedPtr<class SMeshMorpherToolWidget> ToolWidget;
	TSharedPtr<class SMeshMorpherPreviewViewport> PreviewViewport;


	TSharedPtr<FMeshMorpherToolkit> ThisPtr = nullptr;

	UMeshMorpherSettings* Settings = nullptr;
	UMeshMorpherTransformGizmo* TransformGizmo;
};