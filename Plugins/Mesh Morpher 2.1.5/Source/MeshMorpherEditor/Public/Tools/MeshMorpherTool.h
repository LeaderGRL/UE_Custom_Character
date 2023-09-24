// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

#pragma once
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif
#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "UObject/NoExportTypes.h"
#include "BaseTools/MeshSurfacePointTool.h"
#include "Tools/ValueWatcher.h"
#include "Blueprint/UserWidget.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshMorpherTool.generated.h"

using namespace UE::Geometry;

class UMaterialInstanceDynamic;
class UMeshMorpherMeshComponent;
class USkeletalMeshComponent;
class UInteractiveGizmoManager;

/**
 * Tool Builder
 */
UCLASS()
class MESHMORPHEREDITOR_API UMeshMorpherToolBuilder : public UMeshSurfacePointToolBuilder
{
	GENERATED_BODY()

public:
	virtual UMeshSurfacePointTool* CreateNewTool(const FToolBuilderState& SceneState) const override;

	/** @return true if a single mesh source can be found in the active selection */
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;

	virtual void InitializeNewTool(UMeshSurfacePointTool* Tool, const FToolBuilderState& SceneState) const;

	FDynamicMesh3* WeldedDynamicMesh;
	FDynamicMesh3* IdenticalDynamicMesh;
	FDynamicMesh3* ModifiedDynamicMesh;

};

UCLASS()
class MESHMORPHEREDITOR_API UMeshMorpherToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** Primary Brush Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tool, meta = (DisplayName = "Tool Type", MustImplement = "MeshMorpherToolInterface"))
		TSubclassOf<UUserWidget> ToolClass;

};


DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectedToolChanged, UUserWidget*);

/**
 * Dynamic Mesh Sculpt Tool Class
 */
UCLASS()
class MESHMORPHEREDITOR_API UMeshMorpherTool : public UMeshSurfacePointTool
{
	friend class UMeshMorpherBaseTool;
	friend class UMeshMorpherToolBuilder;
	GENERATED_BODY()

public:
	UMeshMorpherTool();
	~UMeshMorpherTool();
	virtual void SetWorld(UWorld* World, UInteractiveGizmoManager* GizmoManagerIn);

	virtual void Setup() override;
	void SetMeshDescription(FMeshDescription& MeshDescription, FMeshDescription& OriginalMeshDescription);

	void SetMesh(FDynamicMesh3* DynamicMesh, FDynamicMesh3* OriginalDynamicMesh);

	void SetMaterial();

	FDynamicMesh3* GetMesh();
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override { return true; }

	virtual bool HitTest(const FRay& Ray, FHitResult& OutHit) override;

	virtual void OnBeginDrag(const FRay& Ray) override;

	virtual void OnUpdateDrag(const FRay& Ray) override;
	virtual void OnEndDrag(const FRay& Ray) override;

	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;

public:
	/** Properties that control sculpting*/
	UPROPERTY(Transient)
		UMeshMorpherToolProperties* SculptProperties;

	UPROPERTY(Transient)
		UUserWidget* SelectedTool;

	UPROPERTY(Transient)
		USkeletalMeshComponent* TargetMeshComponent = nullptr;

	UPROPERTY(Transient)
		UMeshMorpherMeshComponent* DynamicMeshComponent;

	UPROPERTY(Transient)
		UMeshMorpherMeshComponent* OriginalMeshComponent;

private:
	UWorld* TargetWorld;		// required to spawn UPreviewMesh/etc

	TMeshMorpherValueWatcher<TSubclassOf<UUserWidget>> ToolTypeWatcher;

	FDynamicMesh3* WeldedDynamicMesh;
	FDynamicMesh3* IdenticalDynamicMesh;
	FDynamicMesh3* ModifiedDynamicMesh;

	void OnDynamicMeshComponentChanged();
	FDelegateHandle OnDynamicMeshComponentChangedHandle;


public:
	UInteractiveGizmoManager* GizmoManager;
	FOnSelectedToolChanged OnToolChanged;
};



