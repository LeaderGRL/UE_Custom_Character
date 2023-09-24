// Copyright 2020-2022 Yaki Studios. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "EdMode.h"
#include "InputState.h"
#include "EdModeInteractiveToolsContext.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "Tools/LegacyEdModeWidgetHelpers.h"
#include "MeshMorpherEdMode.generated.h"

class FEditorComponentSourceFactory;
class FUICommandList;
struct FMeshMorpherBoneNameInfo;
class FEditorViewportClient;
class FMeshMorpherToolkit;

using namespace UE::Geometry;

UCLASS(Transient)
class MESHMORPHEREDITOR_API UMeshMorpherEdMode : public UBaseLegacyWidgetEdMode
{
	friend class FMeshMorpherToolkit;
	GENERATED_BODY()
public:
	const static FEditorModeID EM_MeshMorpherEdModeId;
public:
	UMeshMorpherEdMode();
	UMeshMorpherEdMode(FVTableHelper& Helper);
	virtual ~UMeshMorpherEdMode();

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual bool UsesToolkits() const override;

	// these disable the standard gizmo, which is probably want we want in
	// these tools as we can't hit-test the standard gizmo...
	virtual bool AllowWidgetMove() override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesTransformWidget() const override;

	virtual void ActorSelectionChangeNotify() override;

	virtual bool ProcessEditDelete();

	virtual bool ProcessEditCut() override;

	virtual bool CanAutoSave() const override;

	virtual void Enter() override;

	bool AcceptTool();

	bool CancelTool();

	bool EnableTool();

	void UpdateSpatialData();

	void FocusCameraAtCursorHotkey();

	virtual void Exit() override;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnModelingModeToolNotification, const FText&);
	FOnModelingModeToolNotification OnToolNotificationMessage;
	FOnModelingModeToolNotification OnToolWarningMessage;
	FDynamicMesh3 WeldedDynamicMesh;
	FDynamicMesh3 IdenticalDynamicMesh;
	UE::Geometry::FDynamicMeshAABBTree3 SpatialData;
	FDynamicMesh3 ModifiedDynamicMesh;

protected:
	/** Command list lives here so that the key bindings on the commands can be processed in the viewport. */
	TSharedPtr<FUICommandList> UICommandList;

private:
	FEditorViewportClient* Client = nullptr;
};
