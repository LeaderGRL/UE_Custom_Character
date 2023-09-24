// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MetaMorph.generated.h"

UCLASS(hidecategories = Object, BlueprintType)
class MESHMORPHERRUNTIME_API UMetaMorph : public UObject
{
	GENERATED_BODY()

public:
	TArray<uint8> Data;

	virtual void Serialize(FArchive& Ar) override
	{
		Ar << Data;
	}
};