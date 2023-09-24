// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherEdMode.h"

#include "InteractiveTool.h"
#include "ToolTargetManager.h"
#include "Framework/Commands/UICommandList.h"
#include "EditorViewportClient.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "Selection/PersistentMeshSelectionManager.h"
#include "Selection/StoredMeshSelectionUtil.h"
#include "Snapping/ModelingSceneSnappingManager.h"

#include "Tools/MeshMorpherTool.h"

#include "MeshMorpherToolHelper.h"

#define LOCTEXT_NAMESPACE "UMeshMorpherEdMode"

const FEditorModeID UMeshMorpherEdMode::EM_MeshMorpherEdModeId = TEXT("EM_MeshMorpherEdMode");

UMeshMorpherEdMode::UMeshMorpherEdMode()
{
	Info = FEditorModeInfo(
		EM_MeshMorpherEdModeId,
		LOCTEXT("MeshMorpherModeName", "MeshMorpher"));
	UICommandList = MakeShareable(new FUICommandList);
}

UMeshMorpherEdMode::UMeshMorpherEdMode(FVTableHelper& Helper)
{
}

UMeshMorpherEdMode::~UMeshMorpherEdMode()
{
}



void UMeshMorpherEdMode::ActorSelectionChangeNotify()
{
}



bool UMeshMorpherEdMode::ProcessEditDelete()
{
	if (UEdMode::ProcessEditDelete())
	{
		return true;
	}

	// for now we disable deleting in an Accept-style tool because it can result in crashes if we are deleting target object
	if (GetToolManager()->HasAnyActiveTool() && GetToolManager()->GetActiveTool(EToolSide::Mouse)->HasAccept())
	{
		GetToolManager()->DisplayMessage(
			LOCTEXT("CannotDeleteWarning", "Cannot delete objects while this Tool is active"), EToolMessageLevel::UserWarning);
		return true;
	}

	// clear any active selection
	//UE::Geometry::ClearActiveToolSelection(GetToolManager());

	return false;
}

bool UMeshMorpherEdMode::ProcessEditCut()
{
	// for now we disable deleting in an Accept-style tool because it can result in crashes if we are deleting target object
	if (GetToolManager()->HasAnyActiveTool() && GetToolManager()->GetActiveTool(EToolSide::Mouse)->HasAccept())
	{
		GetToolManager()->DisplayMessage(
			LOCTEXT("CannotCutWarning", "Cannot cut objects while this Tool is active"), EToolMessageLevel::UserWarning);
		return true;
	}

	// clear any active selection
	//UE::Geometry::ClearActiveToolSelection(GetToolManager());

	return false;
}


bool UMeshMorpherEdMode::ShouldDrawWidget() const
{
	// allow standard xform gizmo if we don't have an active tool
	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext != nullptr && ToolsContext->ToolManager->HasAnyActiveTool())
	{
		return false;
	}

	return UBaseLegacyWidgetEdMode::ShouldDrawWidget();
}

void UMeshMorpherEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	if (ViewportClient)
	{
		if (Client == nullptr)
		{
			Client = ViewportClient;
		}

		if (ViewportClient->GetWidgetMode() != UE::Widget::EWidgetMode::WM_None)
		{
			ViewportClient->SetWidgetMode(UE::Widget::EWidgetMode::WM_None);
		}
	}


	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext)
	{
		ToolsContext->Tick(ViewportClient, DeltaTime);
	}

}


bool UMeshMorpherEdMode::CanAutoSave() const
{
	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext)
	{
		return ToolsContext->ToolManager->HasAnyActiveTool() == false;
	}
	return false;
}

bool UMeshMorpherEdMode::AllowWidgetMove()
{
	return false;
}

bool UMeshMorpherEdMode::UsesTransformWidget() const
{
	return false;
}

void UMeshMorpherEdMode::Enter()
{
	UEdMode::Enter();

	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext)
	{
		ToolsContext->OnToolNotificationMessage.AddLambda([this](const FText& Message)
			{
				this->OnToolNotificationMessage.Broadcast(Message);
			});
		ToolsContext->OnToolWarningMessage.AddLambda([this](const FText& Message)
			{
				this->OnToolWarningMessage.Broadcast(Message);
			});
	}


	UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(GetInteractiveToolsContext());

	UMeshMorpherToolBuilder* MoveVerticesToolBuilder = NewObject<UMeshMorpherToolBuilder>();
	MoveVerticesToolBuilder->WeldedDynamicMesh = &WeldedDynamicMesh;
	MoveVerticesToolBuilder->IdenticalDynamicMesh = &IdenticalDynamicMesh;
	MoveVerticesToolBuilder->ModifiedDynamicMesh = &ModifiedDynamicMesh;
	ToolsContext->ToolManager->RegisterToolType(TEXT("MoveVerticesTool"), MoveVerticesToolBuilder);
	ToolsContext->ToolManager->SelectActiveToolType(EToolSide::Left, TEXT("MoveVerticesTool"));
	// listen for Tool start/end events to bind/unbind any hotkeys relevant to that Tool
}

bool UMeshMorpherEdMode::AcceptTool()
{
	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext)
	{
		if (ToolsContext->CanAcceptActiveTool())
		{
			ToolsContext->EndTool(EToolShutdownType::Accept);
			if (Client)
			{
				ToolsContext->Tick(Client, 0.0f);
			}
			return true;
		}
	}
	return false;
}

bool UMeshMorpherEdMode::CancelTool()
{
	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext)
	{
		if (ToolsContext->CanCancelActiveTool())
		{
			ToolsContext->EndTool(EToolShutdownType::Cancel);
			if (Client)
			{
				ToolsContext->Tick(Client, 0.0f);
			}

			return true;
		}
	}
	return false;
}

bool UMeshMorpherEdMode::EnableTool()
{
	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	if (ToolsContext)
	{
		FString ToolIdentifier = TEXT("MoveVerticesTool");
		if (ToolsContext->CanStartTool(ToolIdentifier))
		{
			ToolsContext->StartTool(ToolIdentifier);
			if (Client)
			{
				ToolsContext->Tick(Client, 0.0f);
			}
			return true;
		}
	}
	return false;
}

void UMeshMorpherEdMode::UpdateSpatialData()
{
	SpatialData.SetMesh(&IdenticalDynamicMesh, true);
}

void UMeshMorpherEdMode::FocusCameraAtCursorHotkey()
{

	UEditorInteractiveToolsContext* ToolsContext = GetInteractiveToolsContext();
	FRay Ray = GetInteractiveToolsContext()->GetLastWorldRay();

	double NearestHitDist = (double)HALF_WORLD_MAX;
	FVector HitPoint = FVector::ZeroVector;
	{
		FTransform Transform = FTransform::Identity;
		FVector Origin = Transform.InverseTransformPosition(Ray.Origin);
		FVector Direction = Transform.InverseTransformVectorNoScale(Ray.Direction);
		Direction.Normalize();

		IMeshSpatial::FQueryOptions RaycastOptions;
		RaycastOptions.TriangleFilterF = [this, Origin](int TriangleID) {
			FVector Normal, Centroid;
			double Area;
			IdenticalDynamicMesh.GetTriInfo(TriangleID, Normal, Area, Centroid);
			return FVector::DotProduct(Normal, (Centroid - Origin)) < 0.0;
		};

		int32 HitTID = SpatialData.FindNearestHitTriangle(UE::Geometry::TRay<double>(Ray.Origin, Ray.Direction), RaycastOptions);

		if (HitTID != IndexConstants::InvalidID)
		{
			FVector v0, v1, v2;
			IdenticalDynamicMesh.GetTriVertices(HitTID, v0, v1, v2);

			double Distance = 0.0;
			const bool bIsInterescting = UMeshMorpherToolHelper::FindTriangleIntersectionPoint(Origin, Direction, v0, v1, v2, Distance);

			if (bIsInterescting)
			{
				FVector Normal = IdenticalDynamicMesh.GetTriNormal(HitTID);

				FVector r = Origin + Distance * Direction;
				HitPoint = Transform.TransformPosition(r);
				NearestHitDist = Ray.GetParameter(HitPoint);
			}

		}
	}


	// cast ray against tool
	if (GetToolManager()->HasAnyActiveTool())
	{
		UMeshMorpherTool* Tool = Cast<UMeshMorpherTool>(GetToolManager()->GetActiveTool(EToolSide::Left));
		FHitResult HitResult;
		if (Tool->HitTest(Ray, HitResult))
		{
			double HitDepth = (double)Ray.GetParameter(HitResult.ImpactPoint);
			if (HitDepth < NearestHitDist)
			{
				NearestHitDist = HitDepth;
				HitPoint = HitResult.ImpactPoint;
			}
		}
	}


	if (NearestHitDist < (double)HALF_WORLD_MAX && Client)
	{
		Client->CenterViewportAtPoint(HitPoint, false);
	}
}


void UMeshMorpherEdMode::Exit()
{
	// clear any active selection
	UE::Geometry::ClearActiveToolSelection(GetToolManager());

	// deregister selection manager (note: may not have been registered, depending on mode settings)
	UE::Geometry::DeregisterPersistentMeshSelectionManager(GetInteractiveToolsContext());

	// exit any exclusive active tools w/ cancel
	if (UInteractiveTool* ActiveTool = GetToolManager()->GetActiveTool(EToolSide::Left))
	{
		if (Cast<IInteractiveToolExclusiveToolAPI>(ActiveTool))
		{
			GetToolManager()->DeactivateTool(EToolSide::Left, EToolShutdownType::Cancel);
		}
	}

	// deregister snapping manager and shut down level objects tracker
	UE::Geometry::DeregisterSceneSnappingManager(GetInteractiveToolsContext());

	// re-enable HitProxy rendering
	GetInteractiveToolsContext()->SetEnableRenderingDuringHitProxyPass(true);

	//UE::TransformGizmoUtil::DeregisterTransformGizmoContextObject(GetInteractiveToolsContext());


	OnToolNotificationMessage.Clear();
	OnToolWarningMessage.Clear();

	UEdMode::Exit();
}

bool UMeshMorpherEdMode::UsesToolkits() const
{
	return false;
}

#undef LOCTEXT_NAMESPACE