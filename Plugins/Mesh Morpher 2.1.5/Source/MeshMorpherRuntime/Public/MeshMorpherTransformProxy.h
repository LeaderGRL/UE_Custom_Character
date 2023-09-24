// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "BaseGizmos/TransformProxy.h"
#include "MeshMorpherTransformProxy.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGizmoUpdateTransform, FTransform, NewTransform);

UCLASS()
class MESHMORPHERRUNTIME_API UMeshMorpherTransformProxy : public UTransformProxy
{
	GENERATED_BODY()
public:
	virtual void AddMeshComponent(USceneComponent* Component);
	virtual FTransform GetTransform() const override;
	virtual void SetTransform(const FTransform& Transform) override;

	virtual void BeginTransformEditSequence() override;
	virtual void EndTransformEditSequence() override;

public:
	FOnGizmoUpdateTransform Callback;
	USceneComponent* DynamicMeshComponent;
};
