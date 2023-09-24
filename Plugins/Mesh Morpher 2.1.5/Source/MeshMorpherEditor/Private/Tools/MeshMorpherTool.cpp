// Copyright 2020-2021 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Tools/MeshMorpherTool.h"
#include "InteractiveToolManager.h"
#include "InteractiveGizmoManager.h"
#include "ToolBuilderUtil.h"
#include "ToolSetupUtil.h"
#include "Materials/Material.h"
#include "Components/MeshMorpherMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Tools/MeshMorpherToolInterface.h"
#include "Engine/Engine.h"
#include "Engine/ObjectLibrary.h"
#include "MeshMorpherSettings.h"

#define LOCTEXT_NAMESPACE "UMeshMorpherTool"

/*
 * ToolBuilder
 */
UMeshSurfacePointTool* UMeshMorpherToolBuilder::CreateNewTool(const FToolBuilderState& SceneState) const
{
	UMeshMorpherTool* SculptTool = NewObject<UMeshMorpherTool>(SceneState.ToolManager);
	SculptTool->WeldedDynamicMesh = WeldedDynamicMesh;
	SculptTool->IdenticalDynamicMesh = IdenticalDynamicMesh;
	SculptTool->ModifiedDynamicMesh = ModifiedDynamicMesh;
	SculptTool->SetWorld(SceneState.World, SceneState.GizmoManager);
	return SculptTool;
}

bool UMeshMorpherToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	if (SceneState.SelectedComponents.Num() > 0)
	{
		return !!Cast<USkeletalMeshComponent>(SceneState.SelectedComponents[0]);
	}
	return false;

}

void UMeshMorpherToolBuilder::InitializeNewTool(UMeshSurfacePointTool* Tool, const FToolBuilderState& SceneState) const
{
	if (SceneState.SelectedComponents.Num() > 0)
	{
		USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(SceneState.SelectedComponents[0]);
		check(MeshComponent != nullptr);
		Tool->SetStylusAPI(this->StylusAPI);
		UMeshMorpherTool* NewTool = Cast< UMeshMorpherTool>(Tool);
		if (NewTool)
		{
			NewTool->TargetMeshComponent = MeshComponent;
			NewTool->GizmoManager = SceneState.GizmoManager;
		}
	}
}

/*
 * Tool
 */
UMeshMorpherTool::UMeshMorpherTool()
{
	// initialize parameters
}

UMeshMorpherTool::~UMeshMorpherTool()
{
}

void UMeshMorpherTool::SetWorld(UWorld* World, UInteractiveGizmoManager* GizmoManagerIn)
{
	this->TargetWorld = World;
}

void UMeshMorpherTool::Setup()
{
	UMeshSurfacePointTool::Setup();

	auto ItemLibrary = UObjectLibrary::CreateLibrary(UUserWidget::StaticClass(), true, GIsEditor);
	ItemLibrary->LoadBlueprintsFromPath(TEXT("/MeshMorpher/"));

	DynamicMeshComponent = NewObject<UMeshMorpherMeshComponent>(this);
	DynamicMeshComponent->RegisterComponentWithWorld(TargetWorld);

	OriginalMeshComponent = NewObject<UMeshMorpherMeshComponent>(this);

	// hide input Component
	TargetMeshComponent->SetVisibility(false);

	SculptProperties = NewObject<UMeshMorpherToolProperties>(this);
	AddToolPropertySource(SculptProperties);

	OnDynamicMeshComponentChangedHandle = DynamicMeshComponent->OnMeshChanged.Add(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UMeshMorpherTool::OnDynamicMeshComponentChanged));


	ToolTypeWatcher.Initialize(
		[this]() { return SculptProperties->ToolClass; },
		[this](TSubclassOf<UUserWidget> ToolType)
		{
			if (SelectedTool)
			{
				if (!SelectedTool->IsUnreachable())
				{
					IMeshMorpherToolInterface::Execute_OnUninitialize(SelectedTool);
				}
				SelectedTool->ConditionalBeginDestroy();
				SelectedTool = nullptr;
				OnToolChanged.Broadcast(nullptr);
			}

			if (GEngine)
			{
				GEngine->ForceGarbageCollection(true);
			}

			if (ToolType)
			{
				UBlueprint* loadedWidget = Cast<UBlueprint>(ToolType->ClassGeneratedBy);

				if (loadedWidget)
				{
					UClass* WidgetClass = loadedWidget->GeneratedClass;
					if (WidgetClass)
					{
						UMeshMorpherSettings* Settings = GetMutableDefault<UMeshMorpherSettings>();
						SelectedTool = CreateWidget<UUserWidget>(this->GetWorld(), WidgetClass);
						if (SelectedTool)
						{
							DynamicMeshComponent->NotifyMeshUpdated(TArray<int32>(), true);
							IMeshMorpherToolInterface::Execute_OnInitialize(SelectedTool, Settings, TargetMeshComponent, DynamicMeshComponent, OriginalMeshComponent);
						}
						OnToolChanged.Broadcast(SelectedTool);
					}
					WidgetClass = nullptr;
				}
			}

		}, nullptr);

	SculptProperties->RestoreProperties(this);

	SetMesh(ModifiedDynamicMesh, WeldedDynamicMesh);

}

void UMeshMorpherTool::SetMeshDescription(FMeshDescription& MeshDescription, FMeshDescription& OriginalMeshDescription)
{
	DynamicMeshComponent->InitializeMesh(&MeshDescription);

	OriginalMeshComponent->InitializeMesh(&OriginalMeshDescription);

	DynamicMeshComponent->MarkRenderStateDirty();
	OriginalMeshComponent->MarkRenderStateDirty();
	SetMaterial();
}

void UMeshMorpherTool::SetMesh(FDynamicMesh3* DynamicMesh, FDynamicMesh3* OriginalDynamicMesh)
{
	DynamicMeshComponent->InitializeMesh(DynamicMesh);


	OriginalMeshComponent->InitializeMesh(OriginalDynamicMesh);

	DynamicMeshComponent->MarkRenderStateDirty();
	OriginalMeshComponent->MarkRenderStateDirty();
	SetMaterial();
}

void UMeshMorpherTool::OnDynamicMeshComponentChanged()
{
	if (DynamicMeshComponent)
	{
		DynamicMeshComponent->bMeshChanged = true;
	}
	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			return IMeshMorpherToolInterface::Execute_OnMeshChanged(SelectedTool);
		}
	}
}


void UMeshMorpherTool::SetMaterial()
{
	if (TargetMeshComponent)
	{
		TArray<UMaterialInterface*> OutMaterials;
		TargetMeshComponent->GetUsedMaterials(OutMaterials);
		if (OutMaterials.Num() > 0)
		{
			for (int32 i = 0; i < OutMaterials.Num(); ++i)
			{
				if(OutMaterials[i])
				{
					DynamicMeshComponent->SetMaterial(i, OutMaterials[i]);
				} else
				{
					DynamicMeshComponent->SetMaterial(i, UMaterial::GetDefaultMaterial(MD_Surface));
				}
				
			}
		}
		else {
			UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/MeshMorpher/SculptMaterial"));
			if (Material == nullptr)
			{
				Material = ToolSetupUtil::GetDefaultSculptMaterial(GetToolManager());
			}

			if (Material != nullptr)
			{
				DynamicMeshComponent->OverrideMaterials.Empty();
				DynamicMeshComponent->SetMaterial(0, Material);
			}
		}
	}
}


FDynamicMesh3* UMeshMorpherTool::GetMesh()
{
	return DynamicMeshComponent->GetMesh();
}

void UMeshMorpherTool::Shutdown(EToolShutdownType ShutdownType)
{

	GizmoManager->DestroyAllGizmosByOwner(this);

	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			IMeshMorpherToolInterface::Execute_OnUninitialize(SelectedTool);
		}
		SelectedTool->ConditionalBeginDestroy();
		SelectedTool = nullptr;
		OnToolChanged.Broadcast(nullptr);
	}

	if (DynamicMeshComponent != nullptr)
	{
		DynamicMeshComponent->OnMeshChanged.Remove(OnDynamicMeshComponentChangedHandle);
		DynamicMeshComponent->UnregisterComponent();
		DynamicMeshComponent->DestroyComponent();
		DynamicMeshComponent->ConditionalBeginDestroy();
		DynamicMeshComponent = nullptr;
	}

	if (OriginalMeshComponent != nullptr)
	{
		OriginalMeshComponent->DestroyComponent();
		OriginalMeshComponent->ConditionalBeginDestroy();
		OriginalMeshComponent = nullptr;
	}

	if (SculptProperties)
	{
		SculptProperties->SaveProperties(this);
		RemoveToolPropertySource(SculptProperties);
		SculptProperties->ConditionalBeginDestroy();
		SculptProperties = nullptr;
	}

	TargetMeshComponent->SetVisibility(true);

	if (GEngine)
	{
		GEngine->ForceGarbageCollection(true);
	}
}


bool UMeshMorpherTool::HitTest(const FRay& Ray, FHitResult& OutHit)
{
	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			return IMeshMorpherToolInterface::Execute_OnHitTest(SelectedTool, Ray.Origin, Ray.Direction, OutHit);
		}
	}
	return false;
}

void UMeshMorpherTool::OnBeginDrag(const FRay& Ray)
{
	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			IMeshMorpherToolInterface::Execute_OnBeginDrag(SelectedTool, Ray.Origin, Ray.Direction);
		}
	}
}

void UMeshMorpherTool::OnUpdateDrag(const FRay& WorldRay)
{
	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			IMeshMorpherToolInterface::Execute_OnUpdateDrag(SelectedTool, WorldRay.Origin, WorldRay.Direction);
		}
	}
}

void UMeshMorpherTool::OnEndDrag(const FRay& Ray)
{
	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			IMeshMorpherToolInterface::Execute_OnEndDrag(SelectedTool, Ray.Origin, Ray.Direction);
		}
	}

}

FInputRayHit UMeshMorpherTool::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	return UMeshSurfacePointTool::BeginHoverSequenceHitTest(PressPos);
}

bool UMeshMorpherTool::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	if (SelectedTool)
	{
		if (!SelectedTool->IsUnreachable())
		{
			return IMeshMorpherToolInterface::Execute_OnUpdateHover(SelectedTool, DevicePos.WorldRay.Origin, DevicePos.WorldRay.Direction);
		}
	}
	return false;
}

void UMeshMorpherTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	UMeshSurfacePointTool::Render(RenderAPI);
}

void UMeshMorpherTool::OnTick(float DeltaTime)
{
	// Allow a tick to pass between application of brush stamps. Bizarrely this
	// improves responsiveness in the Editor...
	//static int32 TICK_SKIP_HACK = 0;
	//if (TICK_SKIP_HACK++ % 2 == 0)
	//{
	//	return;
	//}

	ToolTypeWatcher.CheckAndUpdate();

	if (GetToolManager())
	{
		if (GetToolManager()->GetContextQueriesAPI())
		{
			FViewCameraState StateOut;
			GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(StateOut);

			if (SelectedTool)
			{
				if (!SelectedTool->IsUnreachable())
				{
					FTransform CameraTransform;
					CameraTransform.SetLocation(StateOut.Position);
					CameraTransform.SetRotation(StateOut.Orientation);
					IMeshMorpherToolInterface::Execute_OnUpdate(SelectedTool, DeltaTime, CameraTransform);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

