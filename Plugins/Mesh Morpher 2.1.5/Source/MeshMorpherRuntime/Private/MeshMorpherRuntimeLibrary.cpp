// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#include "MeshMorpherRuntimeLibrary.h"
#include "Generators/SphereGenerator.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
#else
#include "Misc/CoreDelegates.h"
#endif

FDelegateHandle UMeshMorpherRuntimeLibrary::EndPIEHandle = FDelegateHandle();
TMap<USkeletalMesh*, int32> UMeshMorpherRuntimeLibrary::Tracker = {};

UMeshMorpherRuntimeLibrary::UMeshMorpherRuntimeLibrary()
{
#if WITH_EDITOR
	//EndPIEHandle = FEditorDelegates::PrePIEEnded.AddStatic(UMeshMorpherRuntimeLibrary::OnEndPIE);
#endif
}

UMeshMorpherRuntimeLibrary::~UMeshMorpherRuntimeLibrary()
{
#if WITH_EDITOR
	//FEditorDelegates::PrePIEEnded.Remove(EndPIEHandle);
#endif
}

void UMeshMorpherRuntimeLibrary::InvalidateCachedBounds(USkeletalMeshComponent* SkeletalMeshComponent)
{
	if(SkeletalMeshComponent)
	{
		SkeletalMeshComponent->InvalidateCachedBounds();
	}
}

USceneComponent* UMeshMorpherRuntimeLibrary::CreateComponent(UObject* Outer, TSubclassOf<USceneComponent> Class)
{
	if(Outer && Outer->GetWorld())
	{
		USceneComponent* Component = NewObject<USceneComponent>(Outer, Class);
		Component->RegisterComponentWithWorld(Outer->GetWorld());
		return Component;
	}
	return nullptr;
}

UMeshMorpherMeshComponent* UMeshMorpherRuntimeLibrary::CreateSphereComponent(UObject* Outer, const int32 Resolution)
{
	if(Outer && Outer->GetWorld())
	{
		UMeshMorpherMeshComponent* Component = NewObject<UMeshMorpherMeshComponent>(Outer);
		Component->RegisterComponentWithWorld(Outer->GetWorld());

		FSphereGenerator SphereGen;
		SphereGen.NumPhi = SphereGen.NumTheta = Resolution;
		SphereGen.Generate();
		FDynamicMesh3 Mesh(&SphereGen);

		Component->InitializeMesh(&Mesh);
		
		return Component;
	}
	return nullptr;
}

void UMeshMorpherRuntimeLibrary::DestroyComponent(USceneComponent* Component)
{
	if(Component)
	{
		if(Component->IsRegistered())
		{
			Component->UnregisterComponent();
		}
		Component->DestroyComponent();
		Component->ConditionalBeginDestroy();
	}
}

UMorphTarget* UMeshMorpherRuntimeLibrary::FindMorphTarget(USkeletalMesh* SkeletalMesh, FName Name)
{
	if (SkeletalMesh)
	{
		return SkeletalMesh->FindMorphTarget(Name);
	}
	return nullptr;
}

void UMeshMorpherRuntimeLibrary::GetMorphTargetsNames(USkeletalMesh* SkeletalMesh, TArray<FName>& Names)
{
	Names.Empty();

	int32 Count = 0;
	Count = SkeletalMesh->GetMorphTargets().Num();


	if (Count > 0)
	{
		Names.SetNum(Count);
		int32 Index = 0;

		for (UMorphTarget* MorphTarget : SkeletalMesh->GetMorphTargets())
		{
			Names[Index] = MorphTarget ? MorphTarget->GetFName() : NAME_None;
			Index++;
		}
	}
}

bool UMeshMorpherRuntimeLibrary::RemoveMorphTarget(USkeletalMesh* SkeletalMesh, FName Name)
{
	if (SkeletalMesh)
	{
		if (UMorphTarget* MorphTarget = FindMorphTarget(SkeletalMesh, Name))
		{
			SkeletalMesh->UnregisterMorphTarget(MorphTarget);
			return true;
		}
	}
	return false;
}

void UMeshMorpherRuntimeLibrary::OnEndPIE(const bool bIsSimulating)
{
#if WITH_EDITOR
	for (auto& Mesh : Tracker)
	{
		if (Mesh.Key && Mesh.Value > 0)
		{
			Mesh.Key->InitMorphTargetsAndRebuildRenderData();
		}
	}
	Tracker.Empty();
#endif
}