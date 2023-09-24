// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StandaloneMaskSelection.generated.h"

UCLASS(hidecategories = Object, BlueprintType)
class MESHMORPHERRUNTIME_API UStandaloneMaskSelection : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
		USkeletalMesh* SkeletalMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
		TArray<int32> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
		FTransform Transform = FTransform::Identity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
		int32 MeshVertexCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
		int32 MeshTriangleCount = 0;
};