// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherTransformProxy.h"
#include "Components/MeshMorpherMeshComponent.h"

#define LOCTEXT_NAMESPACE "UMeshMorpherTransformProxy"


void UMeshMorpherTransformProxy::AddMeshComponent(USceneComponent* Component)
{
	check(Component);
	DynamicMeshComponent = Component;
	SharedTransform = FTransform::Identity;
	InitialSharedTransform = SharedTransform;
	OnTransformChanged.Broadcast(this, SharedTransform);
}


FTransform UMeshMorpherTransformProxy::GetTransform() const
{
    return SharedTransform;
}

void UMeshMorpherTransformProxy::SetTransform(const FTransform& Transform)
{
	FTransform Temp = Transform.GetRelativeTransform(InitialSharedTransform);
	if (!Temp.ContainsNaN())
	{
		SharedTransform = Transform;
		Callback.ExecuteIfBound(Transform);
	}
}

void UMeshMorpherTransformProxy::BeginTransformEditSequence()
{
	OnBeginTransformEdit.Broadcast(this);
}

void UMeshMorpherTransformProxy::EndTransformEditSequence()
{
	OnEndTransformEdit.Broadcast(this);
}

#undef LOCTEXT_NAMESPACE