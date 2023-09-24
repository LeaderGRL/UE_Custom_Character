// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshOperationsLibrary.h"
#include "SkeletalMeshAttributes.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/Skeleton.h"
#include "GenericQuadTree.h"
#include "Async/ParallelFor.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "AssetRegistryModule.h"
#include "Widgets/SBakeSkeletalMeshWidget.h"
#include "MeshMorpherRuntimeLibrary.h"
#include "StandaloneMaskSelection.h"
#include "FileHelpers.h"
#include "Misc/MessageDialog.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMeshEditor.h"
#include "MeshUtilities.h"
#include "Misc/FeedbackContext.h"
#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif

#include "Engine/SkeletalMeshEditorData.h"

#include "MeshMorpherSettings.h"
#include "MeshOperationsLibraryRT.h"
#include "Components/MeshMorpherMeshComponent.h"

#include "MetaMorph.h"

#include "Implicit/Solidify.h"
#include "ProjectionTargets.h"
#include "MeshMorpherWrapper.h"


#define LOCTEXT_NAMESPACE "MeshMorpherOperations"

static const FString XorKey = "MeshMorpherMeta";

void UMeshOperationsLibrary::NotifyMessage(const FString& Message)
{
	auto result = FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

bool UMeshOperationsLibrary::CopySkeletalMeshMaterialsToStaticMesh(USkeletalMesh* SkeletalMesh, UStaticMesh* StaticMesh)
{
	if (SkeletalMesh && StaticMesh)
	{

		TArray<FStaticMaterial>& StaticMaterials = StaticMesh->GetStaticMaterials();

		TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();

		StaticMaterials.Empty();

		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMaterials.Num(); ++MaterialIndex)
		{
			FStaticMaterial StaticMaterial;
			const FName Slot = *FString("MAT" + FString::FromInt(MaterialIndex));
			StaticMaterial.ImportedMaterialSlotName = Slot;
			StaticMaterial.MaterialInterface = SkeletalMaterials[MaterialIndex].MaterialInterface;
			StaticMaterial.MaterialSlotName = Slot;
			StaticMaterial.UVChannelData = SkeletalMaterials[MaterialIndex].UVChannelData;
			StaticMaterials.Add(StaticMaterial);
		}
		return true;
	}
	return false;
}

bool UMeshOperationsLibrary::CopySkeletalMeshMaterials(USkeletalMesh* SkeletalMesh, TArray<FStaticMaterial>& StaticMaterials)
{
	if (SkeletalMesh)
	{
		TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();

		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMaterials.Num(); ++MaterialIndex)
		{
			FStaticMaterial StaticMaterial;
			const FName Slot = *FString("MAT" + FString::FromInt(MaterialIndex));
			StaticMaterial.ImportedMaterialSlotName = Slot;
			StaticMaterial.MaterialInterface = SkeletalMaterials[MaterialIndex].MaterialInterface;
			StaticMaterial.MaterialSlotName = Slot;
			StaticMaterial.UVChannelData = SkeletalMaterials[MaterialIndex].UVChannelData;
			StaticMaterials.Add(StaticMaterial);
		}
		return true;
	}
	return false;
}

bool UMeshOperationsLibrary::CopySkeletalMeshMaterials(USkeletalMesh* SkeletalMesh, TMap<int32, FName>& StaticMaterials)
{
	if (SkeletalMesh)
	{
		TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();
		StaticMaterials.Empty();
		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMaterials.Num(); ++MaterialIndex)
		{
			FStaticMaterial StaticMaterial;
			const FName Slot = *FString("MAT" + FString::FromInt(MaterialIndex));
			StaticMaterials.Add(MaterialIndex, Slot);
		}
		return true;
	}
	return false;
}

bool UMeshOperationsLibrary::CopySkeletalMeshMaterials(USkeletalMesh* SkeletalMesh, TMap<FName, int32>& StaticMaterials)
{
	if (SkeletalMesh)
	{
		TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();
		StaticMaterials.Empty();
		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMaterials.Num(); ++MaterialIndex)
		{
			FStaticMaterial StaticMaterial;
			const FName Slot = *FString("MAT" + FString::FromInt(MaterialIndex));
			StaticMaterials.Add(Slot, MaterialIndex);
		}
		return true;
	}
	return false;
}


bool UMeshOperationsLibrary::SetStaticMesh(UStaticMesh* StaticMesh, const FMeshDescription& MeshDescription)
{
	if (StaticMesh)
	{
		TArray <const FMeshDescription*> MD;
		MD.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MD);
		return true;
	}
	return false;
}

struct FCompareMorphTargetDeltas3
{
	FORCEINLINE bool operator()(const FMorphTargetDelta& A, const FMorphTargetDelta& B) const
	{
		return ((int32)A.SourceIdx - (int32)B.SourceIdx) < 0 ? true : false;
	}
};


bool UMeshOperationsLibrary::SkeletalMeshToDynamicMesh(USkeletalMesh* SkeletalMesh, FDynamicMesh3& IdenticalDynamicMesh, FDynamicMesh3* WeldedDynamicMesh, const TArray<FFinalSkinVertex>& FinalVertices, int32 LOD, bool bUseRenderData)
{
	/*
	ReadObj("E:\\Girl_Fat.obj", IdenticalDynamicMesh, false, FVector(-1, 1, 1));
	//ReadObj("E:\\tz.obj", IdenticalDynamicMesh);
	if (WeldedDynamicMesh)
	{
		WeldedDynamicMesh->Copy(IdenticalDynamicMesh);
	}
	return true;
	*/
	return UMeshOperationsLibraryRT::SkeletalMeshToDynamicMesh_RenderData(SkeletalMesh, IdenticalDynamicMesh, WeldedDynamicMesh, FinalVertices, LOD);
}


void UMeshOperationsLibrary::CreateMorphTarget(USkeletalMesh* Mesh, FString MorphTargetName)
{
	checkf(Mesh, TEXT("Invalid skeletal mesh."));

	if (Mesh)
	{
		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();
		if (ResourceImported)
		{

			if (ResourceImported->LODModels.IsValidIndex(0))
			{
				FSkeletalMeshLODModel& LODModel = ResourceImported->LODModels[0];
				FSkeletalMeshImportData RawMesh;


				Mesh->LoadLODImportedData(0, RawMesh);

				if (RawMesh.Points.Num())
				{

					Mesh->Modify();
					Mesh->InvalidateDeriveDataCacheGUID();

					int32 ImportMorphIndex = RawMesh.MorphTargetNames.IndexOfByKey(MorphTargetName);
					if (ImportMorphIndex == INDEX_NONE)
					{
						ImportMorphIndex = RawMesh.MorphTargetNames.Add(MorphTargetName);
						RawMesh.MorphTargets.AddDefaulted();
						RawMesh.MorphTargetModifiedPoints.AddDefaulted();
					}
					else {
						RawMesh.MorphTargets[ImportMorphIndex].Points.Empty();
						RawMesh.MorphTargetModifiedPoints[ImportMorphIndex].Empty();
					}

					if (LODModel.MeshToImportVertexMap.IsValidIndex(0))
					{
						const int32 RawIndex = LODModel.MeshToImportVertexMap[0];
						if (RawMesh.Points.IsValidIndex(RawIndex))
						{
							RawMesh.MorphTargets[ImportMorphIndex].Points.Add(RawMesh.Points[RawIndex] + FVector3f::ZeroVector);
							RawMesh.MorphTargetModifiedPoints[ImportMorphIndex].Add(RawIndex);
						}
					}
					Mesh->SaveLODImportedData(0, RawMesh);

					Mesh->MarkPackageDirty();
				}
				else {
					UMeshOperationsLibraryRT::CreateMorphTargetObj(Mesh, MorphTargetName);
				}
			}
		}
	}
}

void UMeshOperationsLibrary::ApplyDeltasToDynamicMesh(const TArray<FMorphTargetDelta>& Deltas, FDynamicMesh3& DynamicMesh)
{

	const int32 Count = Deltas.Num();
	if(Count > 0)
	{
		const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
		const int32 LastChunkSize = Count - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				const FMorphTargetDelta& Vert = Deltas[Index];
				if (DynamicMesh.IsVertex(Vert.SourceIdx))
				{
					const FVector3d Position = DynamicMesh.GetVertex(Vert.SourceIdx) + FVector3d(Vert.PositionDelta);
					DynamicMesh.SetVertex(Vert.SourceIdx, Position, false);
				}
			}
		});

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				if (DynamicMesh.IsVertex(Index))
				{
					DynamicMesh.SetVertexNormal(Index, FVector3f(FMeshNormals::ComputeVertexNormal(DynamicMesh, Index)));
				}
			}
		});
		
	}
}

void UMeshOperationsLibrary::ApplyDeltasToDynamicMesh(FDynamicMesh3& SourceDynamicMesh, const TArray<FMorphTargetDelta>& Deltas, FDynamicMesh3& DynamicMesh)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Deltas"), FText::FromString(FString::FromInt(Deltas.Num())));
	const FText StatusUpdate = FText::Format(LOCTEXT("ApplyDeltasToMorphTarget", "({Deltas}) Applying deltas..."), Args);
	GWarn->BeginSlowTask(StatusUpdate, true);
	if (!GWarn->GetScopeStack().IsEmpty())
	{
		GWarn->GetScopeStack().Last()->MakeDialog(false, true);
	}
	TArray<FMorphTargetDelta> NewDeltas;
	GWarn->StatusForceUpdate(1, 3, FText::FromString("Convert Deltas to Welded Mesh ..."));
	ApplySourceDeltasToDynamicMesh(SourceDynamicMesh, DynamicMesh, Deltas, TSet<int32>(), NewDeltas);
	GWarn->StatusForceUpdate(2, 3, FText::FromString("Apply Deltas to Welded Mesh ..."));
	ApplyDeltasToDynamicMesh(NewDeltas, DynamicMesh);
	GWarn->StatusForceUpdate(3, 3, FText::FromString("Finished!"));
	GWarn->EndSlowTask();
}

void UMeshOperationsLibrary::ApplyChangesToMorphTarget(USkeletalMesh* Mesh, const FDynamicMesh3& DynamicMesh, FString MorphTargetName, const FDynamicMesh3& Original, const FDynamicMesh3& Changed)
{
	checkf(Mesh, TEXT("Invalid skeletal mesh."));
	checkf(Mesh->GetImportedModel()->LODModels.Num() > 0, TEXT("Invalid LOD for skeletal mesh."));

	const FSkeletalMeshRenderData* Resource = Mesh->GetResourceForRendering();
	if (!Resource || !Resource->LODRenderData.IsValidIndex(0))
	{
		return;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("MorphTarget"), FText::FromString(MorphTargetName));
	const FText StatusUpdate = FText::Format(LOCTEXT("ApplyChangesToMorphTarget", "({MorphTarget}) Applying changes..."), Args);
	GWarn->BeginSlowTask(StatusUpdate, true);
	if (!GWarn->GetScopeStack().IsEmpty())
	{
		GWarn->GetScopeStack().Last()->MakeDialog(false, true);
	}
	TArray<FMorphTargetDelta> Deltas;
	GWarn->StatusUpdate(1, 3, FText::FromString("Get Deltas..."));
	UMeshOperationsLibraryRT::GetMorphDeltas(Original, Changed, Deltas);

	if (Deltas.Num() > 0)
	{
		TArray<FMorphTargetDelta> NewDeltas;
		GWarn->StatusUpdate(2, 3, FText::FromString("Apply Deltas to Skeletal Mesh..."));
		ApplySourceDeltasToDynamicMesh(Original, DynamicMesh, Deltas, TSet<int32>(), NewDeltas);
		if (NewDeltas.Num() > 0)
		{

			GWarn->StatusUpdate(3, 3, FText::FromString("Populate Morph target with Deltas.."));
			ApplyMorphTargetToImportData(Mesh, MorphTargetName, NewDeltas);
		}
	}
	GWarn->EndSlowTask();
}

void UMeshOperationsLibrary::GetMorphTargetNames(USkeletalMesh* Mesh, TArray<FString>& MorphTargets)
{
	MorphTargets.Empty();
	if (Mesh)
	{
		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();
		if (ResourceImported)
		{
			if (ResourceImported->LODModels.IsValidIndex(0))
			{
				FSkeletalMeshLODModel& LODModel = ResourceImported->LODModels[0];
				FSkeletalMeshImportData RawMesh;
				Mesh->LoadLODImportedData(0, RawMesh);
				MorphTargets = RawMesh.MorphTargetNames;
			}
		}
		TArray<FString> LocalMorphTargets;
		UMeshOperationsLibraryRT::GetMorphTargetNames(Mesh, LocalMorphTargets);
		for (const FString& MorphTargetName : LocalMorphTargets)
		{
			if (!MorphTargets.Contains(MorphTargetName))
			{
				MorphTargets.Add(MorphTargetName);
			}
		}
	}
}

void UMeshOperationsLibrary::RenameMorphTargetInImportData(USkeletalMesh* Mesh, FString NewName, FString OriginalName, bool bInvalidateRenderData)
{
	if (Mesh)
	{
		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();
		if (ResourceImported)
		{

			Mesh->Modify();
			Mesh->InvalidateDeriveDataCacheGUID();

			for (int32 LODIdx = 0; LODIdx < ResourceImported->LODModels.Num(); ++LODIdx)
			{
				FSkeletalMeshLODModel& LODModel = ResourceImported->LODModels[LODIdx];
				FSkeletalMeshImportData RawMesh;

				Mesh->LoadLODImportedData(LODIdx, RawMesh);

				const int32 ImportMorphIndex = RawMesh.MorphTargetNames.IndexOfByKey(OriginalName);
				if (ImportMorphIndex != INDEX_NONE)
				{
					RawMesh.MorphTargetNames[ImportMorphIndex] = NewName;
					Mesh->SaveLODImportedData(LODIdx, RawMesh);
					Mesh->MarkPackageDirty();
				}
			}
		}

		UMorphTarget* MorphTargetObj = UMeshOperationsLibraryRT::FindMorphTarget(Mesh, OriginalName);
		if (MorphTargetObj)
		{
			MorphTargetObj->Rename(*NewName);
			if (bInvalidateRenderData)
			{
				Mesh->InitMorphTargetsAndRebuildRenderData();
			}
			MorphTargetObj->MarkPackageDirty();
		}
	}
}

void UMeshOperationsLibrary::RemoveMorphTargetsFromImportData(USkeletalMesh* Mesh, const TArray<FString>& MorphTargets, bool bInvalidateRenderData)
{
	if (Mesh && MorphTargets.Num() > 0)
	{
		Mesh->Modify();
		Mesh->InvalidateDeriveDataCacheGUID();

		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();


		if (ResourceImported)
		{
			for (int32 LODIdx = 0; LODIdx < ResourceImported->LODModels.Num(); ++LODIdx)
			{

				FSkeletalMeshLODModel& LODModel = ResourceImported->LODModels[LODIdx];
				FSkeletalMeshImportData RawMesh;

				Mesh->LoadLODImportedData(LODIdx, RawMesh);

				for (const FString& MorphTarget : MorphTargets)
				{
					const int32 ImportMorphIndex = RawMesh.MorphTargetNames.IndexOfByKey(MorphTarget);
					if (ImportMorphIndex != INDEX_NONE)
					{
						if (RawMesh.MorphTargets.IsValidIndex(ImportMorphIndex))
						{
							RawMesh.MorphTargets.RemoveAt(ImportMorphIndex);
						}
						if (RawMesh.MorphTargetNames.IsValidIndex(ImportMorphIndex))
						{
							RawMesh.MorphTargetNames.RemoveAt(ImportMorphIndex);
						}
						if (RawMesh.MorphTargetModifiedPoints.IsValidIndex(ImportMorphIndex))
						{
							RawMesh.MorphTargetModifiedPoints.RemoveAt(ImportMorphIndex);
						}
					}
				}

				Mesh->SaveLODImportedData(LODIdx, RawMesh);
			}
		}

		bool bMorphTargetObjRemoved = false;

		TArray<UMorphTarget*>& LocalMorphTargets = Mesh->GetMorphTargets();

		for (const FString& MorphTarget : MorphTargets)
		{
			UMorphTarget* MorphTargetObj = UMeshOperationsLibraryRT::FindMorphTarget(Mesh, MorphTarget);
			if (MorphTargetObj)
			{
				bMorphTargetObjRemoved = true;
				LocalMorphTargets.Remove(MorphTargetObj);
			}
		}

		if (bMorphTargetObjRemoved && bInvalidateRenderData)
		{
			Mesh->InitMorphTargetsAndRebuildRenderData();
		}

		Mesh->MarkPackageDirty();
	}
}

void UMeshOperationsLibrary::ApplyMorphTargetToImportData(USkeletalMesh* Mesh, FString MorphTargetName, const TArray<FMorphTargetDelta>& Deltas, bool bInvalidateRenderData)
{
	Mesh->Modify();
	Mesh->InvalidateDeriveDataCacheGUID();
	ApplyMorphTargetToImportData(Mesh, MorphTargetName, Deltas, 0);
	ApplyMorphTargetToLODs(Mesh, MorphTargetName, Deltas);
	if (bInvalidateRenderData)
	{
		Mesh->InitMorphTargetsAndRebuildRenderData();
	}
	Mesh->MarkPackageDirty();
}

void UMeshOperationsLibrary::ApplyMorphTargetToImportData(USkeletalMesh* Mesh, FString MorphTargetName, const TArray<FMorphTargetDelta>& Deltas, int32 LOD)
{
	if (Mesh && Deltas.Num() > 0)
	{
		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();
		if (ResourceImported)
		{
			if (ResourceImported->LODModels.IsValidIndex(LOD))
			{
				FSkeletalMeshLODModel& LODModel = ResourceImported->LODModels[LOD];
				FSkeletalMeshImportData RawMesh;

				Mesh->LoadLODImportedData(LOD, RawMesh);

				FSkeletalMeshImportData* ImportMorph = nullptr;
				TSet<uint32>* ModifiedPoints = nullptr;
				const int32 ImportMorphIndex = RawMesh.MorphTargetNames.IndexOfByKey(MorphTargetName);
				if (ImportMorphIndex != INDEX_NONE)
				{
					ImportMorph = &RawMesh.MorphTargets[ImportMorphIndex];
					ModifiedPoints = &RawMesh.MorphTargetModifiedPoints[ImportMorphIndex];
					ImportMorph->Points.Empty();
					ModifiedPoints->Empty();
				}
				else {
					RawMesh.MorphTargetNames.Add(MorphTargetName);
					ImportMorph = &RawMesh.MorphTargets.AddDefaulted_GetRef();
					ModifiedPoints = &RawMesh.MorphTargetModifiedPoints.AddDefaulted_GetRef();
				}

				if (RawMesh.Points.Num())
				{
					TArray<FMorphTargetDelta*> RawPointsDeltas;
					RawPointsDeltas.SetNumZeroed(RawMesh.Points.Num());

					{
						const int32 Count = Deltas.Num();
						if(Count > 0)
						{
							const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
							const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
							const int32 LastChunkSize = Count - (ChunkSize * Cores);
							const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

							ParallelFor(Chunks, [&](const int32 ChunkIndex)
							{
								const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
								for (int X = 0; X < IterationSize; ++X)
								{
									const int32 Index = (ChunkIndex * ChunkSize) + X;
									if (LODModel.MeshToImportVertexMap.IsValidIndex(Deltas[Index].SourceIdx))
									{
										const int32 RawIndex = LODModel.MeshToImportVertexMap[Deltas[Index].SourceIdx];
										if (RawMesh.Points.IsValidIndex(RawIndex))
										{
											RawPointsDeltas[RawIndex] = const_cast<FMorphTargetDelta*>(&Deltas[Index]);
										}
									}
								}
							});
						}
					}

					{
						FCriticalSection Lock;
						const int32 Count = RawMesh.Points.Num();
						if(Count > 0)
						{
							const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
							const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
							const int32 LastChunkSize = Count - (ChunkSize * Cores);
							const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

							ParallelFor(Chunks, [&](const int32 ChunkIndex)
							{
								TSet<uint32> LocalModifiedPoints;
								TArray<FVector3f> Points;
								const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
								for (int X = 0; X < IterationSize; ++X)
								{
									const int32 Index = (ChunkIndex * ChunkSize) + X;

									if(RawPointsDeltas[Index])
									{
										LocalModifiedPoints.Add(static_cast<uint32>(Index));
										Points.Add(RawMesh.Points[Index] + RawPointsDeltas[Index]->PositionDelta);
									}
								}

								if(LocalModifiedPoints.Num() > 0 && Points.Num() == LocalModifiedPoints.Num())
								{
									Lock.Lock();
									ImportMorph->Points.Append(Points);
									ModifiedPoints->Append(LocalModifiedPoints);
									Lock.Unlock();
								}
							});
						}
					}
			
					Mesh->SaveLODImportedData(LOD, RawMesh);
				}


				UMorphTarget* MorphTargetObj = UMeshOperationsLibraryRT::FindMorphTarget(Mesh, MorphTargetName);
				if (!MorphTargetObj)
				{
					UMeshOperationsLibraryRT::CreateMorphTargetObj(Mesh, MorphTargetName, false);

				}

				MorphTargetObj = UMeshOperationsLibraryRT::FindMorphTarget(Mesh, MorphTargetName);

				if (MorphTargetObj)
				{
					MorphTargetObj->PopulateDeltas(Deltas, LOD, ResourceImported->LODModels[LOD].Sections, false, false);
					if (!MorphTargetObj->HasDataForLOD(LOD))
					{
						UMeshOperationsLibraryRT::CreateEmptyLODModel(MorphTargetObj->GetMorphLODModels()[LOD]);
					}
				}
			}
		}
	}

}

void UMeshOperationsLibrary::GetMorphTargetDeltas(USkeletalMesh* Mesh, FString MorphTargetName, TArray<FMorphTargetDelta>& Deltas, int32 LOD)
{
	Deltas.Empty();
	if (Mesh)
	{
		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();
		if (ResourceImported)
		{
			if (ResourceImported->LODModels.IsValidIndex(LOD))
			{
				FSkeletalMeshLODModel& LODModel = ResourceImported->LODModels[LOD];
				bool bIsLODImportedDataBuildAvailable = false;
				bIsLODImportedDataBuildAvailable = Mesh->IsLODImportedDataBuildAvailable(LOD);

				if (bIsLODImportedDataBuildAvailable)
				{
					FSkeletalMeshImportData RawMesh;
					Mesh->LoadLODImportedData(LOD, RawMesh);


					const int32 ImportMorphIndex = RawMesh.MorphTargetNames.IndexOfByKey(MorphTargetName);
					if (ImportMorphIndex != INDEX_NONE)
					{
						const FSkeletalMeshImportData& ImportMorph = RawMesh.MorphTargets[ImportMorphIndex];
						const TSet<uint32>& ModifiedPoints = RawMesh.MorphTargetModifiedPoints[ImportMorphIndex];

						int32 CurrentPointIdx = 0;

						for (const uint32& Idx : ModifiedPoints)
						{
							const int32 MeshIndex = LODModel.MeshToImportVertexMap.Find(Idx);
							if (MeshIndex != INDEX_NONE)
							{
								FMorphTargetDelta NewDelta;
								NewDelta.SourceIdx = MeshIndex;
								NewDelta.PositionDelta = ImportMorph.Points[CurrentPointIdx] - RawMesh.Points[Idx];
								Deltas.Add(NewDelta);
							}
							CurrentPointIdx++;
						}
					}
				}
				else {
					UMorphTarget* MorphTargetObj = UMeshOperationsLibraryRT::FindMorphTarget(Mesh, MorphTargetName);
					if (MorphTargetObj)
					{
						UMeshOperationsLibraryRT::GetMorphTargetDeltas(Mesh, MorphTargetObj, Deltas, LOD);
					}
				}
			}
		}
	}
}

void UMeshOperationsLibrary::SetEnableBuildData(USkeletalMesh* Mesh, bool NewValue)
{
	if (Mesh)
	{
		FSkeletalMeshModel* ResourceImported = Mesh->GetImportedModel();
		Mesh->Modify();

		Mesh->InvalidateDeriveDataCacheGUID();

		if (ResourceImported)
		{
			for (int32 LODIdx = 0; LODIdx < Mesh->GetLODNum(); ++LODIdx)
			{
				if (!Mesh->IsLODImportedDataEmpty(LODIdx))
				{
					if (!NewValue)
					{
						Mesh->SetLODImportedDataVersions(LODIdx, ESkeletalMeshGeoImportVersions::Before_Versionning, ESkeletalMeshSkinningImportVersions::Before_Versionning);
					}
					else {
						Mesh->SetLODImportedDataVersions(LODIdx, ESkeletalMeshGeoImportVersions::LatestVersion, ESkeletalMeshSkinningImportVersions::LatestVersion);
					}
				}
			}
		}
		Mesh->MarkPackageDirty();

		UMeshOperationsLibrary::SaveSkeletalMesh(Mesh);
	}
}


void UMeshOperationsLibrary::ApplyMorphTargetToLODs(USkeletalMesh* Mesh, FString MorphTargetName, const TArray<FMorphTargetDelta>& Deltas)
{
	checkf(Mesh, TEXT("Invalid skeletal mesh."));

	Mesh->WaitForPendingInitOrStreaming();

	UMeshMorpherSettings* Settings = GetMutableDefault<UMeshMorpherSettings>();
	FDynamicMesh3 OriginalMesh;

	if (Settings && !Settings->bUseUVProjectionForLODs)
	{
		SkeletalMeshToDynamicMesh(Mesh, OriginalMesh);
	}

	if (Deltas.Num() > 0)
	{
		for (int32 CurrentLOD = 0; CurrentLOD < Mesh->GetImportedModel()->LODModels.Num(); ++CurrentLOD)
		{
			if (CurrentLOD > 0)
			{
				TArray<FMorphTargetDelta> LODDeltas;
				if (Settings)
				{
					if (Settings->bRemapMorphTargets)
					{
						FSkeletalMeshLODInfo* SrcLODInfo = Mesh->GetLODInfo(CurrentLOD);
						if (SrcLODInfo)
						{
							SrcLODInfo->ReductionSettings.bRemapMorphTargets = true;
						}
					}


					if (Settings->bUseUVProjectionForLODs)
					{
						ApplyMorphTargetToLOD(Mesh, Deltas, 0, CurrentLOD, LODDeltas);
					}
					else
					{
						FDynamicMesh3 LODMesh;
						SkeletalMeshToDynamicMesh(Mesh, LODMesh, NULL, TArray<FFinalSkinVertex>(), CurrentLOD);
						ApplySourceDeltasToDynamicMesh(OriginalMesh, LODMesh, Deltas, TSet<int32>(), LODDeltas, Settings->Threshold, 0.5, 1.0, Settings->SmoothIterations, Settings->SmoothStrength, false);
					}
				}
				else {
					ApplyMorphTargetToLOD(Mesh, Deltas, 0, CurrentLOD, LODDeltas);
				}

				ApplyMorphTargetToImportData(Mesh, MorphTargetName, LODDeltas, CurrentLOD);
			}
		}
	}
}

void FORCEINLINE RebuildTangentBasis(FSoftSkinVertex& DestVertex)
{
	// derive the new tangent by orthonormalizing the new normal against
	// the base tangent vector (assuming these are normalized)
	FVector3f Tangent = DestVertex.TangentX;
	const FVector4f Normal = DestVertex.TangentZ;
	Tangent = Tangent - ((Tangent | Normal) * Normal);
	Tangent.Normalize();
	DestVertex.TangentX = Tangent;
}

bool UMeshOperationsLibrary::ApplyMorphTargetsToSkeletalMesh(USkeletalMesh* SkeletalMesh, const TArray< TSharedPtr<FMeshMorpherMorphTargetInfo> >& MorphTargets)
{
	if (SkeletalMesh && MorphTargets.Num() > 0)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		
		TUniquePtr<FSlowTask> NewScope(new FSlowTask(0, FText::FromString("Bake Morph Target(s)"), true));

		GWarn->BeginSlowTask(FText::FromString("Bake Morph Target(s)"), true);

		if (!GWarn->GetScopeStack().IsEmpty())
		{
			GWarn->GetScopeStack().Last()->MakeDialog(false, true);
		}

		GWarn->StatusForceUpdate(0, 7, FText::FromString("Bake Morph Target(s)"));
		SkeletalMesh->Modify();
		FSkeletalMeshModel* Resource = SkeletalMesh->GetImportedModel();
		check(Resource);

		GWarn->StatusForceUpdate(1, 7, FText::FromString("Convert Skeletal Mesh to Dynamic Mesh..."));
		TArray<FDynamicMesh3> OriginalDynamicMeshes;
		OriginalDynamicMeshes.SetNum(Resource->LODModels.Num());
		for (int32 LOD = 0; LOD < Resource->LODModels.Num(); LOD++)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("LOD"), FText::FromString(FString::FromInt(LOD)));
			const FText StatusUpdate = FText::Format(LOCTEXT("ConvertSkeletalMesh", "Convert Skeletal Mesh to Dynamic Mesh for LOD ({Deltas})"), Args);
			GWarn->StatusForceUpdate(2, 7, StatusUpdate);
			SkeletalMeshToDynamicMesh(SkeletalMesh, OriginalDynamicMeshes[LOD], NULL, TArray<FFinalSkinVertex>(), LOD);
		}
		TArray<FDynamicMesh3> ChangedDynamicMeshes = OriginalDynamicMeshes;
		
		TArray<int32> SkipMorphTargetIdx;
		
		GWarn->StatusForceUpdate(3, 7, FText::FromString("Load Existing Morph Target(s)"));
		TMap<int32, TArray<TArray<FMorphTargetDelta>>> Deltas;
		for (int32 MorphTargetIdx = 0; MorphTargetIdx < MorphTargets.Num(); ++MorphTargetIdx)
		{
			if (MorphTargets[MorphTargetIdx].IsValid())
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("MorphTarget"), FText::FromString(MorphTargets[MorphTargetIdx]->Name.ToString()));
				const FText StatusUpdate = FText::Format(LOCTEXT("LoadExistingMorphTarget", "Load Morph Target: ({MorphTarget})"), Args);
				GWarn->StatusForceUpdate(3, 7, StatusUpdate);
				TArray <TArray<FMorphTargetDelta>>& MorphTargetDelta = Deltas.FindOrAdd(MorphTargetIdx);
				for (int32 LOD = 0; LOD < Resource->LODModels.Num(); LOD++)
				{
					TArray<FMorphTargetDelta>& LODDeltas = MorphTargetDelta.AddDefaulted_GetRef();
					GetMorphTargetDeltas(SkeletalMesh, MorphTargets[MorphTargetIdx]->Name.ToString(), LODDeltas, LOD);

					FDynamicMesh3& DynamicMesh = ChangedDynamicMeshes[LOD];
					if(!FMath::IsNearlyZero(MorphTargets[MorphTargetIdx]->Weight))
					{
						const int32 Count = LODDeltas.Num();
						if(Count > 0)
						{
							const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
							const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
							const int32 LastChunkSize = Count - (ChunkSize * Cores);
							const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

							ParallelFor(Chunks, [&](const int32 ChunkIndex)
							{
								const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
								for (int X = 0; X < IterationSize; ++X)
								{
									const int32 Index = (ChunkIndex * ChunkSize) + X;
									const FMorphTargetDelta& Key = LODDeltas[Index];
									if (DynamicMesh.IsVertex(Key.SourceIdx))
									{
										const FVector3d CurrentPosition = DynamicMesh.GetVertex(Key.SourceIdx) + (FVector(Key.PositionDelta) * MorphTargets[MorphTargetIdx]->Weight);
										DynamicMesh.SetVertex(Key.SourceIdx, CurrentPosition, false);
									}
								}
							});
						}
					}
				}

				if (MorphTargets[MorphTargetIdx]->bRemove)
				{
					SkipMorphTargetIdx.Add(MorphTargetIdx);
				}
			}
		}

		GWarn->StatusForceUpdate(4, 7, FText::FromString("Baking Selected Morph Target(s)"));
		for (int32 LOD = 0; LOD < Resource->LODModels.Num(); LOD++)
		{
			TArray<FMorphTargetDelta> LODDeltas;
			UMeshOperationsLibraryRT::GetMorphDeltas(OriginalDynamicMeshes[LOD], ChangedDynamicMeshes[LOD], LODDeltas);
			FDynamicMesh3& DynamicMesh = ChangedDynamicMeshes[LOD];
			FSkeletalMeshLODModel& LODModel = Resource->LODModels[LOD];
			FSkeletalMeshImportData RawImportDataMesh;
			SkeletalMesh->LoadLODImportedData(LOD, RawImportDataMesh);
			bool bSaveImportData = false;

			const int32 Count = LODDeltas.Num();
			if(Count > 0)
			{
				const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
				const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
				const int32 LastChunkSize = Count - (ChunkSize * Cores);
				const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

				ParallelFor(Chunks, [&](const int32 ChunkIndex)
				{
					const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
					for (int X = 0; X < IterationSize; ++X)
					{
						const int32 Index = (ChunkIndex * ChunkSize) + X;
						const FMorphTargetDelta& Delta = LODDeltas[Index];
						if (DynamicMesh.IsVertex(Delta.SourceIdx))
						{
							if (LODModel.MeshToImportVertexMap.IsValidIndex(Delta.SourceIdx))
							{
								const int32 RawIndex = LODModel.MeshToImportVertexMap[Delta.SourceIdx];
								if (RawImportDataMesh.Points.IsValidIndex(RawIndex))
								{
									bSaveImportData = true;
									RawImportDataMesh.Points[RawIndex] += Delta.PositionDelta;
								}
							}

							int32 SectionIndex;
							int32 Idx;
							LODModel.GetSectionFromVertexIndex(Delta.SourceIdx, SectionIndex, Idx);

							FSoftSkinVertex& DestVertex = LODModel.Sections[SectionIndex].SoftVertices[Idx];
							DestVertex.Position += Delta.PositionDelta;
							const FVector3f Normal = FVector3f(FMeshNormals::ComputeVertexNormal(DynamicMesh, Delta.SourceIdx));
							FVector3f TangentX, TangentY;
							VectorUtil::MakePerpVectors(Normal, TangentX, TangentY);
							DestVertex.TangentY = TangentY;
							DestVertex.TangentX = TangentX;
							DestVertex.TangentZ = FVector4f(Normal);
							DestVertex.TangentZ.W = GetBasisDeterminantSignByte(TangentX, TangentY, Normal);
						}
					}
				});
			}
						
			if (bSaveImportData)
			{
				if (RawImportDataMesh.Points.Num())
				{
					TMap<int32, int32> MorphVerticesMap;
					for (const auto& Delta : LODDeltas)
					{
						if (LODModel.MeshToImportVertexMap.IsValidIndex(Delta.SourceIdx))
						{
							const int32 RawIndex = LODModel.MeshToImportVertexMap[Delta.SourceIdx];
							if (RawImportDataMesh.Points.IsValidIndex(RawIndex))
							{
								MorphVerticesMap.Add(RawIndex, Delta.SourceIdx);
							}
						}
					}
					SkeletalMesh->SaveLODImportedData(LOD, RawImportDataMesh);
				}
			}
		}
	


		GWarn->StatusForceUpdate(5, 7, FText::FromString("Offsetting Morph Target(s)..."));
		for (int32 MorphTargetIdx = 0; MorphTargetIdx < MorphTargets.Num(); ++MorphTargetIdx)
		{
			if (MorphTargets[MorphTargetIdx].IsValid() && !SkipMorphTargetIdx.Contains(MorphTargetIdx))
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("MorphTarget"), FText::FromString(MorphTargets[MorphTargetIdx]->Name.ToString()));
				const FText StatusUpdate = FText::Format(LOCTEXT("OfsetMorphTarget", "Ofsetting Morph Target: ({MorphTarget})"), Args);
				GWarn->StatusForceUpdate(4, 4, StatusUpdate);

				TArray <TArray<FMorphTargetDelta>>& MorphTargetDelta = Deltas[MorphTargetIdx];
				for (int32 LOD = 0; LOD < Resource->LODModels.Num(); LOD++)
				{
					TArray<FMorphTargetDelta>& LODDeltas = MorphTargetDelta[LOD];
					ApplyMorphTargetToImportData(SkeletalMesh, MorphTargets[MorphTargetIdx]->Name.ToString(), LODDeltas, LOD);

				}
			}
		}


		UMeshMorpherSettings* Settings = GetMutableDefault<UMeshMorpherSettings>();
		if (Settings && Settings->bRemapMorphTargets)
		{
			for (int32 LOD = 0; LOD < Resource->LODModels.Num(); LOD++)
			{

				FSkeletalMeshLODInfo* SrcLODInfo = SkeletalMesh->GetLODInfo(LOD);
				if (SrcLODInfo)
				{
					SrcLODInfo->ReductionSettings.bRemapMorphTargets = true;
					//SrcLODInfo->BuildSettings.bRecomputeNormals = true;
					//SrcLODInfo->BuildSettings.bRecomputeTangents = true;
				}
			}
		}

		TArray<FString> MorphTargetNames;

		GWarn->StatusForceUpdate(6, 7, FText::FromString("Removing selected Morph Target(s)..."));
		for (const TSharedPtr<FMeshMorpherMorphTargetInfo>& MorphTarget : MorphTargets)
		{
			if (MorphTarget.IsValid() && MorphTarget->bRemove)
			{
				MorphTargetNames.Add(MorphTarget->Name.ToString());
			}
		}
		RemoveMorphTargetsFromImportData(SkeletalMesh, MorphTargetNames, false);

		GWarn->StatusForceUpdate(7, 7, FText::FromString("Initializing Morph Targets and Rebuilding Render Data..."));
		SkeletalMesh->InvalidateDeriveDataCacheGUID();
		SkeletalMesh->InitMorphTargetsAndRebuildRenderData();
		SkeletalMesh->MarkPackageDirty();
		GWarn->EndSlowTask();
		return true;

	}

	return false;
}

void UMeshOperationsLibrary::SaveSkeletalMesh(USkeletalMesh* Mesh)
{
	checkf(Mesh, TEXT("Invalid skeletal mesh."));
	Mesh->InitMorphTargetsAndRebuildRenderData();

	TArray< UPackage*> Packages;

	UPackage* SkeletalMeshPkg = Cast<UPackage>(Mesh->GetOuter());
	if (SkeletalMeshPkg)
	{
		Packages.Add(SkeletalMeshPkg);
	}

	USkeleton* Skeleton = Mesh->GetSkeleton();

	if (Skeleton)
	{
		UPackage* SkeletonPkg = Cast<UPackage>(Skeleton->GetOuter());
		if (SkeletonPkg)
		{
			Packages.Add(SkeletonPkg);
		}
	}
	FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, true, nullptr, false, true);
}

void UMeshOperationsLibrary::ExportMorphTargetToStaticMesh(FString MorphTargetName, const FMeshDescription& MeshDescription, const TArray<FStaticMaterial>& StaticMaterials)
{
	FString PackageName = FString(TEXT("/Game/Meshes/")) + MorphTargetName;
	FString Name;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

	TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
		SNew(SDlgPickAssetPath)
		.Title(FText::FromString(FString("Choose New StaticMesh Location")))
		.DefaultAssetPath(FText::FromString(PackageName));

	if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
	{
		// Get the full name of where we want to create the physics asset.
		FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
		FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

		// Check if the user inputed a valid asset name, if they did not, give it the generated default name
		if (MeshName == NAME_None)
		{
			// Use the defaults that were already generated.
			UserPackageName = PackageName;
			MeshName = *Name;
		}

		// If we got some valid data.
		if (MeshDescription.Polygons().Num() > 0)
		{
			// Then find/create it.
			UPackage* Package = CreatePackage(*UserPackageName);
			check(Package);

			// Create StaticMesh object
			UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
			StaticMesh->InitResources();

			StaticMesh->SetLightingGuid(FGuid::NewGuid());

			// Add source to new StaticMesh
			FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
			SrcModel.BuildSettings.bRecomputeNormals = true;
			SrcModel.BuildSettings.bRecomputeTangents = true;
			SrcModel.BuildSettings.bRemoveDegenerates = true;
			SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
			SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
			SrcModel.BuildSettings.bGenerateLightmapUVs = true;
			SrcModel.BuildSettings.SrcLightmapIndex = 0;
			SrcModel.BuildSettings.DstLightmapIndex = 1;
			StaticMesh->CreateMeshDescription(0, MeshDescription);
			StaticMesh->CommitMeshDescription(0);

			StaticMesh->SetStaticMaterials(StaticMaterials);

			//Set the Imported version before calling the build
			StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

			// Build mesh from source
			StaticMesh->Build(false);
			StaticMesh->PostEditChange();

			// Notify asset registry of new asset
			FAssetRegistryModule::AssetCreated(StaticMesh);

		}
	}
}

/**
	Below code is taken from LODUtilities but slightly modifed.
*/

struct FMMTargetMatch
{
	double BarycentricWeight[3]; //The weight we use to interpolate the TARGET data
	uint32 Indices[3]; //BASE Index of the triangle vertice
};

struct FMMTriangleElement
{
	FBox2D UVsBound;
	FBox PositionBound;
	TArray<FSoftSkinVertex> Vertices;
	TArray<uint32> Indexes;
	uint32 TriangleIndex;
};

/** Given three direction vectors, indicates if A and B are on the same 'side' of Vec. */
bool VectorsOnSameSide(const FVector2D& Vec, const FVector2D& A, const FVector2D& B)
{
	return !FMath::IsNegativeDouble(((B.Y - A.Y) * (Vec.X - A.X)) + ((A.X - B.X) * (Vec.Y - A.Y)));
}

double PointToSegmentDistanceSquare(const FVector2D& A, const FVector2D& B, const FVector2D& P)
{
	return FVector2D::DistSquared(P, FMath::ClosestPointOnSegment2D(P, A, B));
}

/** Return true if P is within triangle created by A, B and C. */
bool PointInTriangle(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& P)
{
	//If the point is on a triangle point we consider the point inside the triangle

	if (P.Equals(A) || P.Equals(B) || P.Equals(C))
	{
		return true;
	}
	// If its on the same side as the remaining vert for all edges, then its inside.	
	if (VectorsOnSameSide(A, B, P) &&
		VectorsOnSameSide(B, C, P) &&
		VectorsOnSameSide(C, A, P))
	{
		return true;
	}

	//Make sure point on the edge are count inside the triangle
	if (PointToSegmentDistanceSquare(A, B, P) <= DOUBLE_KINDA_SMALL_NUMBER)
	{
		return true;
	}
	if (PointToSegmentDistanceSquare(B, C, P) <= DOUBLE_KINDA_SMALL_NUMBER)
	{
		return true;
	}
	if (PointToSegmentDistanceSquare(C, A, P) <= DOUBLE_KINDA_SMALL_NUMBER)
	{
		return true;
	}
	return false;
}

/** Given three direction vectors, indicates if A and B are on the same 'side' of Vec. */
bool VectorsOnSameSide(const FVector& Vec, const FVector& A, const FVector& B, const double SameSideDotProductEpsilon)
{
	const FVector CrossA = Vec ^ A;
	const FVector CrossB = Vec ^ B;
	double DotWithEpsilon = SameSideDotProductEpsilon + (CrossA | CrossB);
	return !FMath::IsNegativeDouble(DotWithEpsilon);
}

/** Util to see if P lies within triangle created by A, B and C. */
bool PointInTriangle(const FVector& A, const FVector& B, const FVector& C, const FVector& P)
{
	// Cross product indicates which 'side' of the vector the point is on
	// If its on the same side as the remaining vert for all edges, then its inside.	
	if (VectorsOnSameSide(B - A, P - A, C - A, DOUBLE_KINDA_SMALL_NUMBER) &&
		VectorsOnSameSide(C - B, P - B, A - B, DOUBLE_KINDA_SMALL_NUMBER) &&
		VectorsOnSameSide(A - C, P - C, B - C, DOUBLE_KINDA_SMALL_NUMBER))
	{
		return true;
	}
	return false;
}

FVector GetBaryCentric(const FVector& Point, const FVector& A, const FVector& B, const FVector& C)
{
	// Compute the normal of the triangle
	const FVector TriNorm = (B - A) ^ (C - A);

	//check collinearity of A,B,C
	if (TriNorm.SizeSquared() <= DOUBLE_SMALL_NUMBER)
	{
		double DistA = FVector::DistSquared(Point, A);
		double DistB = FVector::DistSquared(Point, B);
		double DistC = FVector::DistSquared(Point, C);
		if (DistA <= DistB && DistA <= DistC)
		{
			return FVector(1.0, 0.0, 0.0);
		}
		if (DistB <= DistC)
		{
			return FVector(0.0, 1.0, 0.0);
		}
		return FVector(0.0, 0.0, 1.0);
	}
	return FMath::ComputeBaryCentric2D(Point, A, B, C);
}

bool FindTriangleUVMatch(const FVector2D& TargetUV, const TArray<FMMTriangleElement>& Triangles, const TArray<uint32>& QuadTreeTriangleResults, TArray<uint32>& MatchTriangleIndexes)
{
	for (uint32 TriangleIndex : QuadTreeTriangleResults)
	{
		const FMMTriangleElement& TriangleElement = Triangles[TriangleIndex];
		if (PointInTriangle(FVector2D(TriangleElement.Vertices[0].UVs[0]), FVector2D(TriangleElement.Vertices[1].UVs[0]), FVector2D(TriangleElement.Vertices[2].UVs[0]), TargetUV))
		{
			MatchTriangleIndexes.Add(TriangleIndex);
		}
		TriangleIndex++;
	}
	return MatchTriangleIndexes.Num() == 0 ? false : true;
}

void ProjectTargetOnBase(const TArray<FSoftSkinVertex>& BaseVertices, const TArray<TArray<uint32>>& PerSectionBaseTriangleIndices,
	TArray<FMMTargetMatch>& TargetMatchData, const TArray<FSkelMeshSection>& TargetSections, const TArray<int32>& TargetSectionMatchBaseIndex, const TCHAR* DebugContext, double InDistanceThreshold = 0.05, double InFailSafe = 0.1)
{
	bool bNoMatchMsgDone = false;
	TArray<FMMTriangleElement> Triangles;
	//Project section target vertices on match base section using the UVs coordinates
	for (int32 SectionIndex = 0; SectionIndex < TargetSections.Num(); ++SectionIndex)
	{
		//Use the remap base index in case some sections disappear during the reduce phase
		int32 BaseSectionIndex = TargetSectionMatchBaseIndex[SectionIndex];
		if (BaseSectionIndex == INDEX_NONE || !PerSectionBaseTriangleIndices.IsValidIndex(BaseSectionIndex) || PerSectionBaseTriangleIndices[BaseSectionIndex].Num() < 1)
		{
			continue;
		}
		//Target vertices for the Section
		const TArray<FSoftSkinVertex>& TargetVertices = TargetSections[SectionIndex].SoftVertices;
		//Base Triangle indices for the matched base section
		const TArray<uint32>& BaseTriangleIndices = PerSectionBaseTriangleIndices[BaseSectionIndex];
		FBox2D BaseMeshUVBound(EForceInit::ForceInit);
		FBox BaseMeshPositionBound(EForceInit::ForceInit);
		//Fill the triangle element to speed up the triangle research
		Triangles.Reset(BaseTriangleIndices.Num() / 3);

		for (uint32 TriangleIndex = 0; TriangleIndex < (uint32)BaseTriangleIndices.Num(); TriangleIndex += 3)
		{
			FMMTriangleElement TriangleElement;
			TriangleElement.UVsBound.Init();
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				uint32 CornerIndice = BaseTriangleIndices[TriangleIndex + Corner];
				check(BaseVertices.IsValidIndex(CornerIndice));
				const FSoftSkinVertex& BaseVertex = BaseVertices[CornerIndice];
				TriangleElement.Indexes.Add(CornerIndice);
				TriangleElement.Vertices.Add(BaseVertex);
				TriangleElement.UVsBound += FVector2D(BaseVertex.UVs[0]);
				BaseMeshPositionBound += FVector(BaseVertex.Position);
			}
			BaseMeshUVBound += TriangleElement.UVsBound;
			TriangleElement.TriangleIndex = Triangles.Num();
			Triangles.Add(TriangleElement);
		}

		check(!BaseMeshUVBound.GetExtent().IsNearlyZero());
		//Setup the Quad tree
		double UVsQuadTreeMinSize = 0.001;
		TQuadTree<uint32, 100> QuadTree(BaseMeshUVBound, UVsQuadTreeMinSize);
		for (FMMTriangleElement& TriangleElement : Triangles)
		{
			QuadTree.Insert(TriangleElement.TriangleIndex, TriangleElement.UVsBound, DebugContext);
		}
		//Retrieve all triangle that are close to our point, let get 5% of UV extend
		double DistanceThreshold = BaseMeshUVBound.GetExtent().Size() * InDistanceThreshold;
		//Find a match triangle for every target vertices


		const int32 Count = TargetVertices.Num();
		if(Count > 0)
		{
			const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
			const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
			const int32 LastChunkSize = Count - (ChunkSize * Cores);
			const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

			ParallelFor(Chunks, [&](const int32 ChunkIndex)
			{
				const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
				for (int X = 0; X < IterationSize; ++X)
				{
					const int32 TargetVertexIndex = (ChunkIndex * ChunkSize) + X;
					FVector2D TargetUV = FVector2D(TargetVertices[TargetVertexIndex].UVs[0]);
					//Reset the last data without flushing the memmery allocation

					TArray<uint32> QuadTreeTriangleResults;
					QuadTreeTriangleResults.Reserve(Triangles.Num() / 10); //Reserve 10% to speed up the query

					const uint32 FullTargetIndex = TargetSections[SectionIndex].BaseVertexIndex + TargetVertexIndex;
					//Make sure the array is allocate properly
					if (TargetMatchData.IsValidIndex(FullTargetIndex))
					{
						//Set default data for the target match, in case we cannot found a match
						FMMTargetMatch& TargetMatch = TargetMatchData[FullTargetIndex];
						for (int32 Corner = 0; Corner < 3; ++Corner)
						{
							TargetMatch.Indices[Corner] = INDEX_NONE;
							TargetMatch.BarycentricWeight[Corner] = 0.3333; //The weight will be use to found the proper delta
						}

						FVector2D Extent(DistanceThreshold, DistanceThreshold);
						FBox2D CurBox(TargetUV - Extent, TargetUV + Extent);
						while (QuadTreeTriangleResults.Num() <= 0)
						{
							QuadTree.GetElements(CurBox, QuadTreeTriangleResults);
							Extent *= 2;
							CurBox = FBox2D(TargetUV - Extent, TargetUV + Extent);
						}

						auto GetDistancePointToBaseTriangle = [&Triangles, &TargetVertices, &TargetVertexIndex](const uint32 BaseTriangleIndex)->double
						{
							FMMTriangleElement& CandidateTriangle = Triangles[BaseTriangleIndex];
							return FVector::DistSquared(FMath::ClosestPointOnTriangleToPoint(FVector(TargetVertices[TargetVertexIndex].Position), FVector(CandidateTriangle.Vertices[0].Position), FVector(CandidateTriangle.Vertices[1].Position), FVector(CandidateTriangle.Vertices[2].Position)), FVector(TargetVertices[TargetVertexIndex].Position));
						};

						auto FailSafeUnmatchVertex = [&GetDistancePointToBaseTriangle, &QuadTreeTriangleResults](uint32& OutIndexMatch)->bool
						{
							bool bFoundMatch = false;
							double ClosestTriangleDistSquared = MAX_dbl;
							for (const uint32 MatchTriangleIndex : QuadTreeTriangleResults)
							{
								const double TriangleDistSquared = GetDistancePointToBaseTriangle(MatchTriangleIndex);
								if (TriangleDistSquared < ClosestTriangleDistSquared)
								{
									ClosestTriangleDistSquared = TriangleDistSquared;
									OutIndexMatch = MatchTriangleIndex;
									bFoundMatch = true;
								}
							}
							return bFoundMatch;
						};

						//Find all Triangles that contain the Target UV
						if (QuadTreeTriangleResults.Num() > 0)
						{
							TArray<uint32> MatchTriangleIndexes;
							uint32 FoundIndexMatch = INDEX_NONE;
							bool bFoundMatch = true;
							if (!FindTriangleUVMatch(TargetUV, Triangles, QuadTreeTriangleResults, MatchTriangleIndexes))
							{
								if (!FailSafeUnmatchVertex(FoundIndexMatch))
								{
									bFoundMatch = false;
								}
							}
							if (bFoundMatch)
							{
								double ClosestTriangleDistSquared = MAX_dbl;
								if (MatchTriangleIndexes.Num() == 1)
								{
									//One match, this mean no mirror UVs simply take the single match
									FoundIndexMatch = MatchTriangleIndexes[0];
									ClosestTriangleDistSquared = GetDistancePointToBaseTriangle(FoundIndexMatch);
								}
								else
								{
									//Geometry can use mirror so the UVs are not unique. Use the closest match triangle to the point to find the best match
									for (uint32 MatchTriangleIndex : MatchTriangleIndexes)
									{
										double TriangleDistSquared = GetDistancePointToBaseTriangle(MatchTriangleIndex);
										if (TriangleDistSquared < ClosestTriangleDistSquared)
										{
											ClosestTriangleDistSquared = TriangleDistSquared;
											FoundIndexMatch = MatchTriangleIndex;
										}
									}
								}

								//FAIL SAFE, make sure we have a match that make sense
								//Use the mesh section geometry bound extent (10% of it) to validate we are close enough.
								if (ClosestTriangleDistSquared > BaseMeshPositionBound.GetExtent().SizeSquared() * InFailSafe)
								{
									//Executing fail safe, if the UVs are too much off because of the reduction, use the closest distance to polygons to find the match
									//This path is not optimize and should not happen often.
									FailSafeUnmatchVertex(FoundIndexMatch);
								}

								//We should always have a valid match at this point
								check(FoundIndexMatch != INDEX_NONE);
								FMMTriangleElement& BestTriangle = Triangles[FoundIndexMatch];
								//Found the surface area of the 3 barycentric triangles from the UVs
								FVector BarycentricWeight;
								BarycentricWeight = GetBaryCentric(FVector(TargetUV, 0.0), FVector(FVector2D(BestTriangle.Vertices[0].UVs[0]), 0.0), FVector(FVector2D(BestTriangle.Vertices[1].UVs[0]), 0.0), FVector(FVector2D(BestTriangle.Vertices[2].UVs[0]), 0.0));
								//Fill the target match
								for (int32 Corner = 0; Corner < 3; ++Corner)
								{
									TargetMatch.Indices[Corner] = BestTriangle.Indexes[Corner];
									TargetMatch.BarycentricWeight[Corner] = BarycentricWeight[Corner]; //The weight will be use to found the proper delta
								}
							}
						}
					}
				}
			});
		}
	}
}

bool CreateLODMorphTarget(const TArray<FMorphTargetDelta>& Deltas, const TMap<uint32, uint32>& PerMorphTargetBaseIndexToMorphTargetDelta, const TMap<uint32, TArray<uint32>>& BaseMorphIndexToTargetIndexList, const TArray<FSoftSkinVertex>& TargetVertices, const TArray<FMMTargetMatch>& TargetMatchData, TArray<FMorphTargetDelta>& OutDeltas)
{
	if (Deltas.Num() > 0)
	{
		OutDeltas.Empty();
		TSet<uint32> CreatedTargetIndex;
		TMap<FVector3f, TArray<uint32>> MorphTargetPerPosition;
		for (uint32 MorphDeltaIndex = 0; MorphDeltaIndex < (uint32)(Deltas.Num()); ++MorphDeltaIndex)
		{
			const FMorphTargetDelta& MorphDelta = Deltas[MorphDeltaIndex];
			const TArray<uint32>* TargetIndexesPtr = BaseMorphIndexToTargetIndexList.Find(MorphDelta.SourceIdx);
			if (TargetIndexesPtr == nullptr)
			{
				continue;
			}
			const TArray<uint32>& TargetIndexes = *TargetIndexesPtr;
			for (int32 MorphTargetIndex = 0; MorphTargetIndex < TargetIndexes.Num(); ++MorphTargetIndex)
			{
				uint32 TargetIndex = TargetIndexes[MorphTargetIndex];
				if (CreatedTargetIndex.Contains(TargetIndex))
				{
					continue;
				}
				CreatedTargetIndex.Add(TargetIndex);
				const FVector3f& SearchPosition = TargetVertices[TargetIndex].Position;
				FMorphTargetDelta MatchMorphDelta;
				MatchMorphDelta.SourceIdx = TargetIndex;

				const FMMTargetMatch& TargetMatch = TargetMatchData[TargetIndex];

				//Find the Position/tangent delta for the MatchMorphDelta using the barycentric weight
				MatchMorphDelta.PositionDelta = FVector3f(0.0f);
				MatchMorphDelta.TangentZDelta = FVector3f(0.0f);
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const uint32* BaseMorphTargetIndexPtr = PerMorphTargetBaseIndexToMorphTargetDelta.Find(TargetMatch.Indices[Corner]);
					if (BaseMorphTargetIndexPtr != nullptr && Deltas.IsValidIndex(*BaseMorphTargetIndexPtr))
					{
						const FMorphTargetDelta& BaseMorphTargetDelta = Deltas[*BaseMorphTargetIndexPtr];
						FVector3f BasePositionDelta = !BaseMorphTargetDelta.PositionDelta.ContainsNaN() ? BaseMorphTargetDelta.PositionDelta : FVector3f(0.0f);
						FVector3f BaseTangentZDelta = !BaseMorphTargetDelta.TangentZDelta.ContainsNaN() ? BaseMorphTargetDelta.TangentZDelta : FVector3f(0.0f);
						MatchMorphDelta.PositionDelta += BasePositionDelta * TargetMatch.BarycentricWeight[Corner];
						MatchMorphDelta.TangentZDelta += BaseTangentZDelta * TargetMatch.BarycentricWeight[Corner];
					}
					ensure(!MatchMorphDelta.PositionDelta.ContainsNaN());
					ensure(!MatchMorphDelta.TangentZDelta.ContainsNaN());
				}

				//Make sure all morph delta that are at the same position use the same delta to avoid hole in the geometry
				TArray<uint32>* MorphTargetsIndexUsingPosition = nullptr;
				MorphTargetsIndexUsingPosition = MorphTargetPerPosition.Find(SearchPosition);
				if (MorphTargetsIndexUsingPosition != nullptr)
				{
					//Get the maximum position/tangent delta for the existing matched morph delta
					FVector3f PositionDelta = MatchMorphDelta.PositionDelta;
					FVector3f TangentZDelta = MatchMorphDelta.TangentZDelta;
					for (uint32 ExistingMorphTargetIndex : *MorphTargetsIndexUsingPosition)
					{
						const FMorphTargetDelta& ExistingMorphDelta = OutDeltas[ExistingMorphTargetIndex];
						PositionDelta = PositionDelta.SizeSquared() > ExistingMorphDelta.PositionDelta.SizeSquared() ? PositionDelta : ExistingMorphDelta.PositionDelta;
						TangentZDelta = TangentZDelta.SizeSquared() > ExistingMorphDelta.TangentZDelta.SizeSquared() ? TangentZDelta : ExistingMorphDelta.TangentZDelta;
					}
					//Update all MorphTarget that share the same position.
					for (uint32 ExistingMorphTargetIndex : *MorphTargetsIndexUsingPosition)
					{
						FMorphTargetDelta& ExistingMorphDelta = OutDeltas[ExistingMorphTargetIndex];
						ExistingMorphDelta.PositionDelta = PositionDelta;
						ExistingMorphDelta.TangentZDelta = TangentZDelta;
					}
					MatchMorphDelta.PositionDelta = PositionDelta;
					MatchMorphDelta.TangentZDelta = TangentZDelta;
					MorphTargetsIndexUsingPosition->Add(OutDeltas.Num());
				}
				else
				{
					MorphTargetPerPosition.Add(TargetVertices[TargetIndex].Position).Add(OutDeltas.Num());
				}
				OutDeltas.Add(MatchMorphDelta);
			}
		}
		return OutDeltas.Num() > 0;
	}
	return false;
}

bool UMeshOperationsLibrary::ApplyMorphTargetToLOD(USkeletalMesh* SkeletalMesh, const TArray<FMorphTargetDelta>& Deltas, int32 SourceLOD, int32 DestinationLOD, TArray<FMorphTargetDelta>& OutDeltas)
{
	check(SkeletalMesh);
	FSkeletalMeshModel* SkeletalMeshResource = SkeletalMesh->GetImportedModel();
	if (!SkeletalMeshResource ||
		!SkeletalMeshResource->LODModels.IsValidIndex(SourceLOD) ||
		!SkeletalMeshResource->LODModels.IsValidIndex(DestinationLOD) ||
		SourceLOD > DestinationLOD)
	{
		//Cannot reduce if the source model is missing or we reduce from a higher index LOD
		return false;
	}

	FSkeletalMeshLODModel& SourceLODModel = SkeletalMeshResource->LODModels[SourceLOD];

	const FSkeletalMeshLODModel& BaseLODModel = SkeletalMeshResource->LODModels[SourceLOD];
	const FSkeletalMeshLODModel& TargetLODModel = SkeletalMeshResource->LODModels[DestinationLOD];

	auto InternalGetSectionMaterialIndex = [](const FSkeletalMeshLODModel& LODModel, int32 SectionIndex)->int32
	{
		if (!LODModel.Sections.IsValidIndex(SectionIndex))
		{
			return 0;
		}
		return LODModel.Sections[SectionIndex].MaterialIndex;
	};

	auto GetBaseSectionMaterialIndex = [&BaseLODModel, &InternalGetSectionMaterialIndex](int32 SectionIndex)->int32
	{
		return InternalGetSectionMaterialIndex(BaseLODModel, SectionIndex);
	};

	auto GetTargetSectionMaterialIndex = [&TargetLODModel, &InternalGetSectionMaterialIndex](int32 SectionIndex)->int32
	{
		return InternalGetSectionMaterialIndex(TargetLODModel, SectionIndex);
	};

	//Make sure we have some morph for this LOD
	if (Deltas.Num() <= 0)
	{
		return false;
	}

	//We have to match target sections index with the correct base section index. Reduced LODs can contain a different number of sections than the base LOD
	TArray<int32> TargetSectionMatchBaseIndex;
	//Initialize the array to INDEX_NONE
	TargetSectionMatchBaseIndex.AddUninitialized(TargetLODModel.Sections.Num());
	for (int32 TargetSectionIndex = 0; TargetSectionIndex < TargetLODModel.Sections.Num(); ++TargetSectionIndex)
	{
		TargetSectionMatchBaseIndex[TargetSectionIndex] = INDEX_NONE;
	}
	TBitArray<> BaseSectionMatch;
	BaseSectionMatch.Init(false, BaseLODModel.Sections.Num());
	//Find corresponding section indices from Source LOD for Target LOD
	for (int32 TargetSectionIndex = 0; TargetSectionIndex < TargetLODModel.Sections.Num(); ++TargetSectionIndex)
	{
		int32 TargetSectionMaterialIndex = GetTargetSectionMaterialIndex(TargetSectionIndex);
		for (int32 BaseSectionIndex = 0; BaseSectionIndex < BaseLODModel.Sections.Num(); ++BaseSectionIndex)
		{
			if (BaseSectionMatch[BaseSectionIndex])
			{
				continue;
			}
			int32 BaseSectionMaterialIndex = GetBaseSectionMaterialIndex(BaseSectionIndex);
			if (TargetSectionMaterialIndex == BaseSectionMaterialIndex)
			{
				TargetSectionMatchBaseIndex[TargetSectionIndex] = BaseSectionIndex;
				BaseSectionMatch[BaseSectionIndex] = true;
				break;
			}
		}
	}
	//We should have match all the target sections
	check(!TargetSectionMatchBaseIndex.Contains(INDEX_NONE));
	TArray<FSoftSkinVertex> BaseVertices;
	TArray<FSoftSkinVertex> TargetVertices;
	BaseLODModel.GetVertices(BaseVertices);
	TargetLODModel.GetVertices(TargetVertices);
	//Create the base triangle indices per section
	TArray<TArray<uint32>> BaseTriangleIndices;
	int32 SectionCount = BaseLODModel.Sections.Num();
	BaseTriangleIndices.AddDefaulted(SectionCount);

	for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
	{
		const FSkelMeshSection& Section = BaseLODModel.Sections[SectionIndex];
		uint32 TriangleCount = Section.NumTriangles;
		BaseTriangleIndices[SectionIndex].SetNumZeroed(TriangleCount * 3);
	}

	for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
	{
		const FSkelMeshSection& Section = BaseLODModel.Sections[SectionIndex];
		uint32 TriangleCount = Section.NumTriangles;
		ParallelFor(TriangleCount, [&](int32 TriangleIndex)
		{
			for (uint32 PointIndex = 0; PointIndex < 3; PointIndex++)
			{
				const int32 Index = (TriangleIndex * 3) + PointIndex;
				uint32 IndexBufferValue = BaseLODModel.IndexBuffer[Section.BaseIndex + Index];
				BaseTriangleIndices[SectionIndex][Index] = IndexBufferValue;
			}
		});
	}
	//Every target vertices match a Base LOD triangle, we also want the barycentric weight of the triangle match. All this done using the UVs
	TArray<FMMTargetMatch> TargetMatchData;
	TargetMatchData.AddUninitialized(TargetVertices.Num());
	//Match all target vertices to a Base triangle Using UVs.
	ProjectTargetOnBase(BaseVertices, BaseTriangleIndices, TargetMatchData, TargetLODModel.Sections, TargetSectionMatchBaseIndex, *SkeletalMesh->GetName());
	//Helper to retrieve the FMorphTargetDelta from the BaseIndex
	TMap<uint32, uint32> PerMorphTargetBaseIndexToMorphTargetDelta;
	//Create a map from BaseIndex to a list of match target index for all base morph target point
	TMap<uint32, TArray<uint32>> BaseMorphIndexToTargetIndexList;

	if (Deltas.Num() > 0)
	{
		for (uint32 MorphDeltaIndex = 0; MorphDeltaIndex < (uint32)(Deltas.Num()); ++MorphDeltaIndex)
		{
			const FMorphTargetDelta& MorphDelta = Deltas[MorphDeltaIndex];
			PerMorphTargetBaseIndexToMorphTargetDelta.Add(MorphDelta.SourceIdx, MorphDeltaIndex);
			//Iterate the targetmatch data so we can store which target indexes is impacted by this morph delta.
			for (int32 TargetIndex = 0; TargetIndex < TargetMatchData.Num(); ++TargetIndex)
			{
				const FMMTargetMatch& TargetMatch = TargetMatchData[TargetIndex];
				if (TargetMatch.Indices[0] == INDEX_NONE)
				{
					//In case this vertex did not found a triangle match
					continue;
				}
				if (TargetMatch.Indices[0] == MorphDelta.SourceIdx || TargetMatch.Indices[1] == MorphDelta.SourceIdx || TargetMatch.Indices[2] == MorphDelta.SourceIdx)
				{
					TArray<uint32>& TargetIndexes = BaseMorphIndexToTargetIndexList.FindOrAdd(MorphDelta.SourceIdx);
					TargetIndexes.AddUnique(TargetIndex);
				}
			}
		}
		return CreateLODMorphTarget(Deltas, PerMorphTargetBaseIndexToMorphTargetDelta, BaseMorphIndexToTargetIndexList, TargetVertices, TargetMatchData, OutDeltas);
	}
	return false;

}

///END of LODUtilities code

bool UMeshOperationsLibrary::MergeMorphTargets(USkeletalMesh* SkeletalMesh, const TArray<FString>& MorphTargets, TArray<FMorphTargetDelta>& OutDeltas)
{
	if (SkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				const FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[0];

				FDynamicMesh3 OriginalMesh;
				SkeletalMeshToDynamicMesh(SkeletalMesh, OriginalMesh);

				FDynamicMesh3 MeshCopy = OriginalMesh;


				GWarn->StatusForceUpdate(1, 3, FText::FromString("Retrieving Morph Target Deltas ..."));
				for (auto& MorphTarget : MorphTargets)
				{
					TArray<FMorphTargetDelta> Deltas;
					UMeshOperationsLibrary::GetMorphTargetDeltas(SkeletalMesh, MorphTarget, Deltas);
					if (Deltas.Num() > 0)
					{
						ApplyDeltasToDynamicMesh(Deltas, MeshCopy);
					}
				}

				UMeshOperationsLibraryRT::GetMorphDeltas(OriginalMesh, MeshCopy, OutDeltas);

				return OutDeltas.Num() > 0;
			}
		}
	}
	return false;
}

bool UMeshOperationsLibrary::SetMorphTargetMagnitude(USkeletalMesh* SkeletalMesh, const FString& MorphTarget, const double& Magnitude, TArray<FMorphTargetDelta>& OutDeltas)
{
	if (SkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				const FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[0];

				FDynamicMesh3 OriginalMesh;
				SkeletalMeshToDynamicMesh(SkeletalMesh, OriginalMesh);

				FDynamicMesh3 MeshCopy = OriginalMesh;

				TArray<FMorphTargetDelta> Deltas;
				UMeshOperationsLibrary::GetMorphTargetDeltas(SkeletalMesh, MorphTarget, Deltas);

				const int32 Count = Deltas.Num();
				if(Count > 0)
				{
					const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
					const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
					const int32 LastChunkSize = Count - (ChunkSize * Cores);
					const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

					ParallelFor(Chunks, [&](const int32 ChunkIndex)
					{
						const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
						for (int X = 0; X < IterationSize; ++X)
						{
							const int32 Index = (ChunkIndex * ChunkSize) + X;
							Deltas[Index].PositionDelta *= FVector3f(Magnitude);
						}
					});
					ApplyDeltasToDynamicMesh(Deltas, MeshCopy);

					UMeshOperationsLibraryRT::GetMorphDeltas(OriginalMesh, MeshCopy, OutDeltas);
					return OutDeltas.Num() > 0;
				}
			}
		}
	}
	return false;
}


bool UMeshOperationsLibrary::CopyMorphTarget(USkeletalMesh* SkeletalMesh, const TArray<FString>& MorphTargets, USkeletalMesh* TargetSkeletalMesh, TArray<TArray<FMorphTargetDelta>>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength)
{
	if (SkeletalMesh && TargetSkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				FMeshMorpherWrapper Wrapper;
				Wrapper.VertexThreshold = Threshold;
				Wrapper.NormalIncompatibilityThreshold = NormalIncompatibilityThreshold;
				
				GWarn->StatusForceUpdate(1, 3, FText::FromString("Converting Skeletal Mesh to Dynamic Mesh ..."));
				if (!SkeletalMeshToDynamicMesh(SkeletalMesh, Wrapper.SourceMesh))
				{
					return false;
				}

				if (!SkeletalMeshToDynamicMesh(TargetSkeletalMesh, Wrapper.TargetMesh))
				{
					return false;
				}

				const bool bIdentical = FMeshMorpherWrapper::IsDynamicMeshIdentical(Wrapper.SourceMesh, Wrapper.TargetMesh);


				GWarn->StatusForceUpdate(2, 3, FText::FromString("Retrieving Morph Target Deltas ..."));
				for (const FString& MorphTarget : MorphTargets)
				{
					TArray<FMorphTargetDelta>& Deltas = OutDeltas.AddDefaulted_GetRef();
					GetMorphTargetDeltas(SkeletalMesh, MorphTarget, Deltas);
					if (bIdentical)
					{
						continue;
					}
					else {
						TArray<FMorphTargetDelta> LocalDelta;
						const bool bResult = Wrapper.ProjectDeltas(Deltas, false, Multiplier, SmoothIterations, SmoothStrength, LocalDelta);
						if(bResult)
						{
							Deltas = MoveTemp(LocalDelta);
						}
					}
				}

				return true;
			}
		}
	}
	return false;
}

void UMeshOperationsLibrary::ApplySourceDeltasToDynamicMesh(const FDynamicMesh3& SourceDynamicMesh, const FDynamicMesh3& DynamicMesh, const TArray<FMorphTargetDelta>& SourceDeltas, const TSet<int32>& IgnoreVertices, TArray<FMorphTargetDelta>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength, bool bCheckIdentical)
{

	OutDeltas.Empty();

	FMeshMorpherWrapper Wrapper;
	Wrapper.SourceMesh = SourceDynamicMesh;
	Wrapper.TargetMesh = DynamicMesh;
	Wrapper.VertexThreshold = Threshold;
	Wrapper.NormalIncompatibilityThreshold = NormalIncompatibilityThreshold;
	FDynamicMesh3 TargetDynamicMesh = DynamicMesh;

	const bool bIdentical = bCheckIdentical ? FMeshMorpherWrapper::IsDynamicMeshIdentical(Wrapper.SourceMesh, Wrapper.TargetMesh) : false;

	if (bIdentical)
	{
		OutDeltas = SourceDeltas;
		if (IgnoreVertices.Num() > 0)
		{
			for (int32 Idx = OutDeltas.Num() - 1; Idx >= 0; --Idx)
			{
				if (IgnoreVertices.Contains(OutDeltas[Idx].SourceIdx))
				{
					OutDeltas.RemoveAt(Idx);
				}
			}
		}
	}
	else {

		TArray<FMorphTargetDelta> LocalDelta;
		const bool bResult = Wrapper.ProjectDeltas(SourceDeltas, false, Multiplier, SmoothIterations, SmoothStrength, LocalDelta);

		if (IgnoreVertices.Num() > 0)
		{
			for (int32 Idx = LocalDelta.Num() - 1; Idx >= 0; --Idx)
			{
				if (IgnoreVertices.Contains(LocalDelta[Idx].SourceIdx))
				{
					LocalDelta.RemoveAt(Idx);
				}
			}
		}

		if(bResult)
		{
			OutDeltas = MoveTemp(LocalDelta);
		}

	}
}

bool UMeshOperationsLibrary::CreateMorphTargetFromPose(USkeletalMesh* SkeletalMesh, const FDynamicMesh3& PoseDynamicMesh, TArray<FMorphTargetDelta>& OutDeltas)
{
	if (SkeletalMesh)
	{
		OutDeltas.Empty();
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				const FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[0];

				FDynamicMesh3 BaseDynamicMesh;
				GWarn->StatusForceUpdate(1, 3, FText::FromString("Converting Skeletal Mesh to Dynamic Mesh ..."));
				if (!SkeletalMeshToDynamicMesh(SkeletalMesh, BaseDynamicMesh))
				{
					return false;
				}
				GWarn->StatusForceUpdate(2, 3, FText::FromString("Retrieving Morph Target Deltas ..."));
				UMeshOperationsLibraryRT::GetMorphDeltas(BaseDynamicMesh, PoseDynamicMesh, OutDeltas);
				return OutDeltas.Num() > 0;
			}
		}
	}

	return false;
}

bool UMeshOperationsLibrary::CreateMorphTargetFromMesh(USkeletalMesh* SkeletalMesh, USkeletalMesh* SourceSkeletalMesh, TArray<FMorphTargetDelta>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength)
{
	if (SkeletalMesh && SourceSkeletalMesh)
	{
		OutDeltas.Empty();
		SkeletalMesh->WaitForPendingInitOrStreaming();
		SourceSkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		const FSkeletalMeshRenderData* SourceResource = SourceSkeletalMesh->GetResourceForRendering();
		if (Resource && SourceResource)
		{
			FDynamicMesh3 BaseDynamicMesh;
			FDynamicMesh3 SourceDynamicMesh;
			int32 FoundLOD = INDEX_NONE;
			GWarn->StatusForceUpdate(1, 3, FText::FromString("Finding Matching LODs ..."));
			for (int32 LOD = 0; LOD < Resource->LODRenderData.Num(); ++LOD)
			{
				if (Resource->LODRenderData.IsValidIndex(LOD) && SourceResource->LODRenderData.IsValidIndex(LOD))
				{
					if (!SkeletalMeshToDynamicMesh(SkeletalMesh, BaseDynamicMesh, NULL, TArray<FFinalSkinVertex>(), LOD))
					{
						return false;
					}

					if (!SkeletalMeshToDynamicMesh(SourceSkeletalMesh, SourceDynamicMesh, NULL, TArray<FFinalSkinVertex>(), LOD))
					{
						return false;
					}
					
					if (BaseDynamicMesh.VertexCount() == SourceDynamicMesh.VertexCount())
					{
						FoundLOD = LOD;
						break;
					}

				}
			}
			GWarn->StatusForceUpdate(2, 3, FText::FromString("Retrieving Morph Target Deltas ..."));
			if (FoundLOD != INDEX_NONE)
			{
				if (FoundLOD > 0)
				{
					TArray<FMorphTargetDelta> Deltas;
					UMeshOperationsLibraryRT::GetMorphDeltas(BaseDynamicMesh, SourceDynamicMesh, Deltas);

					FDynamicMesh3 BaseDynamicMeshLOD0;
					if (!SkeletalMeshToDynamicMesh(SkeletalMesh, BaseDynamicMeshLOD0))
					{
						return false;
					}

					ApplySourceDeltasToDynamicMesh(BaseDynamicMesh, BaseDynamicMeshLOD0, Deltas, TSet<int32>(), OutDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength, true);

					return OutDeltas.Num() > 0;

				}
				else {
					UMeshOperationsLibraryRT::GetMorphDeltas(BaseDynamicMesh, SourceDynamicMesh, OutDeltas);
					return OutDeltas.Num() > 0;
				}

			} else
			{

				if (!SkeletalMeshToDynamicMesh(SkeletalMesh, BaseDynamicMesh))
				{
					return false;
				}

				if (!SkeletalMeshToDynamicMesh(SourceSkeletalMesh, SourceDynamicMesh))
				{
					return false;
				}
				
				FMeshMorpherWrapper Wrapper;
				Wrapper.SourceMesh = SourceDynamicMesh;
				Wrapper.TargetMesh = BaseDynamicMesh;
				Wrapper.VertexThreshold = Threshold;
				Wrapper.NormalIncompatibilityThreshold = NormalIncompatibilityThreshold;

				Wrapper.ProjectMesh(true, Multiplier, SmoothIterations, SmoothStrength, OutDeltas);
				return OutDeltas.Num() > 0;
				
			}
		}
	}
	return false;
}

void SubdivideMesh(FDynamicMesh3& Mesh)
{
	TArray<int> EdgesToProcess;
	for (int tid : Mesh.EdgeIndicesItr())
	{
		EdgesToProcess.Add(tid);
	}
	const int MaxTriangleID = Mesh.MaxTriangleID();

	TArray<int> TriSplitEdges;
	TriSplitEdges.Init(-1, Mesh.MaxTriangleID());

	for (const int eid : EdgesToProcess)
	{
		const FIndex2i EdgeTris = Mesh.GetEdgeT(eid);

		FDynamicMesh3::FEdgeSplitInfo SplitInfo;
		const EMeshResult result = Mesh.SplitEdge(eid, SplitInfo);
		if (result == EMeshResult::Ok)
		{
			if (EdgeTris.A < MaxTriangleID && TriSplitEdges[EdgeTris.A] == -1)
			{
				TriSplitEdges[EdgeTris.A] = SplitInfo.NewEdges.B;
			}
			if (EdgeTris.B != FDynamicMesh3::InvalidID)
			{
				if (EdgeTris.B < MaxTriangleID && TriSplitEdges[EdgeTris.B] == -1)
				{
					TriSplitEdges[EdgeTris.B] = SplitInfo.NewEdges.C;
				}
			}
		}
	}

	for (const int eid : TriSplitEdges)
	{
		if (eid != -1)
		{
			FDynamicMesh3::FEdgeFlipInfo FlipInfo;
			Mesh.FlipEdge(eid, FlipInfo);
		}
	}
}

int32 UMeshOperationsLibrary::CreateMorphTargetFromDynamicMeshes(USkeletalMesh* SkeletalMesh, const FDynamicMesh3& BaseMesh, const FDynamicMesh3& MorphedMesh, TArray<FMorphTargetDelta>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength)
{
	if (SkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				const FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[0];
				GWarn->StatusForceUpdate(1, 5, FText::FromString("Converting Skeletal Mesh to Dynamic Mesh ..."));
				FDynamicMesh3 DynamicMesh;
				if (SkeletalMeshToDynamicMesh(SkeletalMesh, DynamicMesh))
				{
					return CreateMorphTargetFromDynamicMeshes(DynamicMesh, BaseMesh, MorphedMesh, OutDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength);
				}
			}
		}
	}
	return -1;
}

int32 UMeshOperationsLibrary::CreateMorphTargetFromDynamicMeshes(const FDynamicMesh3& DynamicMesh, const FDynamicMesh3& BaseMesh, const FDynamicMesh3& MorphedMesh, TArray<FMorphTargetDelta>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength)
{
	GWarn->StatusForceUpdate(2, 5, FText::FromString("Checking Base Mesh has same Topology with Morphed Mesh ..."));
	if ((BaseMesh.VertexCount() == MorphedMesh.VertexCount()) && BaseMesh.VertexCount() > 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Passed vertex count: %d"), BaseObj.VertexCount());

		TArray<FMorphTargetDelta> InitialDeltas;
		GWarn->StatusForceUpdate(3, 5, FText::FromString("Retrieving Morph Target Deltas ..."));
		UMeshOperationsLibraryRT::GetMorphDeltas(BaseMesh, MorphedMesh, InitialDeltas);

		//UE_LOG(LogTemp, Warning, TEXT("Initial Deltas Found. %d\n"), InitialDeltas.Num());

		if (InitialDeltas.Num() <= 0)
		{
			return -2;
		}

		OutDeltas.Empty();
		GWarn->StatusForceUpdate(4, 5, FText::FromString("Applying Morph Target Deltas to Dynamic Mesh ..."));
		ApplySourceDeltasToDynamicMesh(BaseMesh, DynamicMesh, InitialDeltas, TSet<int32>(), OutDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength, true);


		//UE_LOG(LogTemp, Warning, TEXT("Out Deltas Found. %d\n"), OutDeltas.Num());

		if (OutDeltas.Num() <= 0)
		{
			return -3;
		}

		return 0;
	}

	return -1;
}

bool UMeshOperationsLibrary::CreateMorphTargetsFromMetaMorph(USkeletalMesh* SkeletalMesh, const TArray<UMetaMorph*>& MetaMorphs, TMap<FName, TMap<int32, FMorphTargetDelta>>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength, bool bMergeMoveDeltasToMorphTargets)
{
	if (SkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				FDynamicMesh3 DynamicMesh;
				FDynamicMesh3 WeldedDynamicMesh;
				if (!SkeletalMeshToDynamicMesh(SkeletalMesh, DynamicMesh, &WeldedDynamicMesh))
				{
					return false;
				}
				return CreateMorphTargetsFromMetaMorph(DynamicMesh, WeldedDynamicMesh, MetaMorphs, OutDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength, bMergeMoveDeltasToMorphTargets);
			}
		}
	}
	return false;
}


bool UMeshOperationsLibrary::CreateMorphTargetsFromMetaMorph(const FDynamicMesh3& DynamicMesh, const FDynamicMesh3& WeldedDynamicMesh, const TArray<UMetaMorph*>& MetaMorphs, TMap<FName, TMap<int32, FMorphTargetDelta>>& OutDeltas, double Threshold, double NormalIncompatibilityThreshold, double Multiplier, int32 SmoothIterations, double SmoothStrength, bool bMergeMoveDeltasToMorphTargets)
{
	for (UMetaMorph* MetaMorph : MetaMorphs)
	{
		if (MetaMorph)
		{
			TArray<uint8> Data = MetaMorph->Data;

			ParallelFor(Data.Num(), [&](int32 Index)
				{
					Data[Index] ^= XorKey[Index % XorKey.Len()];
				});

			FString RawData;
			for (int i = 0; i < Data.Num(); ++i)
			{
				RawData.AppendChar(Data[i]);
			}

			//UE_LOG(LogTemp, Warning, TEXT("%s"), *RawData);

			TArray<FString> LinesData;
			RawData.ParseIntoArrayLines(LinesData);

			FDynamicMesh3 BaseMesh;
			TMap<FName, TArray<FMorphTargetDelta>> LocalDeltas;
			TMap<FName, TArray<FMorphTargetDelta>> LocalMoveDeltas;

			TSet<int32> IgnoreVertices;
			bool bMeetRequirements = true;
			if (LinesData.Num() > 0)
			{
				if (LinesData[0].Equals(FString("#METAMORPH FILE")))
				{
					LinesData.RemoveAt(0);
					TArray<FMorphTargetDelta>* CurrentMorphTarget = NULL;
					TArray<FMorphTargetDelta>* CurrentMoveMorphTarget = NULL;
					for (const FString& Line : LinesData)
					{
						if (Line.Contains(FString("vertice ")))
						{
							FString LocalLine = Line.Replace(*FString("vertice "), *FString(""));
							FVector Position = FVector::ZeroVector;
							Position.InitFromString(LocalLine);

							//UE_LOG(LogTemp, Warning, TEXT("%s"), *Position.ToString());

							BaseMesh.AppendVertex(Position);

						}
						else if (Line.Contains(FString("triangle ")))
						{
							FString LocalLine = Line.Replace(*FString("triangle "), *FString(""));
							FVector TriangleVector = FVector::ZeroVector;
							TriangleVector.InitFromString(LocalLine);
							FIndex3i Triangle = FIndex3i(FIntVector(TriangleVector));

							//UE_LOG(LogTemp, Warning, TEXT("%s"), *FIntVector(TriangleVector).ToString());

							BaseMesh.AppendTriangle(Triangle);
						}
						else if (Line.Contains(FString("morphtarget ")))
						{
							FString LocalLine = Line.Replace(*FString("morphtarget "), *FString(""));
							//UE_LOG(LogTemp, Warning, TEXT("%s"), *LocalLine);
							CurrentMorphTarget = &LocalDeltas.FindOrAdd(FName(MetaMorph->GetName() + "_" + LocalLine));
						}
						else if (Line.Contains(FString("delta ")))
						{
							FString LocalLine = Line.Replace(*FString("delta "), *FString(""));

							TArray<FString> DeltaLine;
							LocalLine.ParseIntoArray(DeltaLine, *FString(":"));
							if (DeltaLine.Num() >= 2)
							{
								if (CurrentMorphTarget != NULL)
								{
									FMorphTargetDelta Delta;
									Delta.SourceIdx = FCString::Atoi(*DeltaLine[0]);
									Delta.PositionDelta = FVector3f::ZeroVector;
									Delta.PositionDelta.InitFromString(DeltaLine[1]);
									Delta.TangentZDelta = FVector3f::ZeroVector;
									CurrentMorphTarget->Add(Delta);

									//UE_LOG(LogTemp, Warning, TEXT("%s"), *Delta.PositionDelta.ToString());

								}
							}
						}
						else if (Line.Contains(FString("requirements ")))
						{
							FString LocalLine = Line.Replace(*FString("requirements "), *FString(""));
							TArray<FString> ReqLine;
							LocalLine.ParseIntoArray(ReqLine, *FString(":"));
							if (ReqLine.Num() >= 2)
							{
								if (ReqLine.Num() >= 3)
								{
									CurrentMoveMorphTarget = &LocalMoveDeltas.FindOrAdd(FName("Fix_" + ReqLine[2]));
									//UE_LOG(LogTemp, Warning, TEXT("%s"), *ReqLine[2]);
								}

								if (FCString::Atoi(*ReqLine[0]) == WeldedDynamicMesh.VertexCount() && FCString::Atoi(*ReqLine[1]) == WeldedDynamicMesh.TriangleCount())
								{
									//UE_LOG(LogTemp, Warning, TEXT("%s:%s"), *ReqLine[0], *ReqLine[1]);
									bMeetRequirements = true;
								}
								else {
									//UE_LOG(LogTemp, Warning, TEXT("%s:%s"), *ReqLine[0], *ReqLine[1]);
									bMeetRequirements = false;
								}
							}

						}
						else if (Line.Contains(FString("ignore ")) && bMeetRequirements)
						{
							FString LocalLine = Line.Replace(*FString("ignore "), *FString(""));
							IgnoreVertices.Add(FCString::Atoi(*LocalLine));
						}
						else if (Line.Contains(FString("move ")) && bMeetRequirements)
						{
							FString LocalLine = Line.Replace(*FString("move "), *FString(""));
							//UE_LOG(LogTemp, Warning, TEXT("%s"), *LocalLine);
							TArray<FString> ReqLine;
							LocalLine.ParseIntoArray(ReqLine, *FString(":"));
							if (ReqLine.Num() >= 2)
							{
								if (CurrentMoveMorphTarget != NULL)
								{
									FTransform Transform;
									Transform.InitFromString(ReqLine[1]);

									FMorphTargetDelta Delta;
									Delta.SourceIdx = FCString::Atoi(*ReqLine[0]);
									Delta.PositionDelta = FVector3f::ZeroVector;
									Delta.TangentZDelta = FVector3f::ZeroVector;

									if (WeldedDynamicMesh.IsVertex(Delta.SourceIdx))
									{
										FVector LocalVertex = WeldedDynamicMesh.GetVertex(Delta.SourceIdx);
										FVector NewLocalVertex = Transform.TransformPosition(LocalVertex);
										Delta.PositionDelta = FVector3f(NewLocalVertex - LocalVertex);
									}
									CurrentMoveMorphTarget->Add(Delta);
									//UE_LOG(LogTemp, Warning, TEXT("%s"), *Delta.PositionDelta.ToString());
								}
							}
						}
					}
				}
			}


			TMap<FName, TArray<FMorphTargetDelta>> ToMergeDeltas;

			for (auto& Deltas : LocalDeltas)
			{
				TArray<FMorphTargetDelta> WeldedDeltas;
				ApplySourceDeltasToDynamicMesh(BaseMesh, WeldedDynamicMesh, Deltas.Value, IgnoreVertices, WeldedDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength, true);

				TArray<FMorphTargetDelta>& FinalDeltas = ToMergeDeltas.FindOrAdd(Deltas.Key);
				ApplySourceDeltasToDynamicMesh(WeldedDynamicMesh, DynamicMesh, WeldedDeltas, TSet<int32>(), FinalDeltas, 1.0, 0.5,1.0, 0, 0.0, true);

			}

			TMap<FName, TArray<FMorphTargetDelta>> MoveDeltas;

			for (auto& Deltas : LocalMoveDeltas)
			{
				TArray<FMorphTargetDelta>& FinalDeltas = MoveDeltas.FindOrAdd(Deltas.Key);
				ApplySourceDeltasToDynamicMesh(WeldedDynamicMesh, DynamicMesh, Deltas.Value, TSet<int32>(), FinalDeltas, 1.0, 0.5, 1.0, 0, 0.0, true);
			}


			TMap<FName, TMap<int32, FMorphTargetDelta>> FinalMergeDeltas;

			for (auto& MergeMorphTarget : ToMergeDeltas)
			{
				TMap<int32, FMorphTargetDelta>& Deltas = FinalMergeDeltas.FindOrAdd(MergeMorphTarget.Key);
				for (auto& Delta : MergeMorphTarget.Value)
				{
					Deltas.Add(Delta.SourceIdx, Delta);
				}
			}

			TMap<FName, TMap<int32, FMorphTargetDelta>> FinalMoveDeltas;

			for (auto& MoveMorphTarget : MoveDeltas)
			{
				TMap<int32, FMorphTargetDelta>& Deltas = FinalMoveDeltas.FindOrAdd(MoveMorphTarget.Key);
				for (auto& Delta : MoveMorphTarget.Value)
				{
					Deltas.Add(Delta.SourceIdx, Delta);
				}
			}

			if (bMergeMoveDeltasToMorphTargets)
			{
				for (auto& MergeMorphTarget : FinalMergeDeltas)
				{
					for (auto& MoveMorphTarget : FinalMoveDeltas)
					{
						for (auto& Delta : MoveMorphTarget.Value)
						{
							if (MergeMorphTarget.Value.Contains(Delta.Key))
							{
								MergeMorphTarget.Value[Delta.Key].PositionDelta += Delta.Value.PositionDelta;
							}
							else {
								MergeMorphTarget.Value.Add(Delta.Key, Delta.Value);
							}
						}
					}
				}

				OutDeltas = FinalMergeDeltas;
			}
			else {
				OutDeltas.Append(FinalMergeDeltas);
				OutDeltas.Append(FinalMoveDeltas);
			}

		}
	}

	return OutDeltas.Num() > 0;
}


bool UMeshOperationsLibrary::CreateMetaMorphAssetFromMorphTargets(USkeletalMesh* SkeletalMesh, const TArray<FString>& MorphTargets, const TArray< UStandaloneMaskSelection*>& IgnoreMasks, const TArray< UStandaloneMaskSelection*>& MoveMasks)
{
	if (SkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		FDynamicMesh3 DynamicMesh;
		FDynamicMesh3 WeldedDynamicMesh;
		if (!SkeletalMeshToDynamicMesh(SkeletalMesh, DynamicMesh, &WeldedDynamicMesh))
		{
			return false;
		}

		const FString InitialName = MorphTargets.Num() == 1 ? MorphTargets[0] : SkeletalMesh->GetName();

		FString PackageName = FString(TEXT("/Game/MetaMorphs/")) + InitialName;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(FText::FromString(FString("Choose New Meta Morph Location")))
			.DefaultAssetPath(FText::FromString(PackageName));
		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			FName MetaMorphName(*FPackageName::GetLongPackageAssetName(UserPackageName));

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MetaMorphName == NAME_None)
			{
				// Use the defaults that were already generated.
				UserPackageName = PackageName;
				MetaMorphName = *Name;
			}

			GWarn->BeginSlowTask(FText::FromString("Create Meta Morph from Mesh Files ..."), true);
			if (!GWarn->GetScopeStack().IsEmpty())
			{
				GWarn->GetScopeStack().Last()->MakeDialog(false, true);
			}
			GWarn->UpdateProgress(0, 6);

			FString Data;

			Data.Append(FString("#METAMORPH FILE") + LINE_TERMINATOR);

			GWarn->StatusForceUpdate(1, 6, FText::FromString("Writing Vertice Data ..."));
			for (const FVector3d& Vertice : WeldedDynamicMesh.VerticesItr())
			{
				Data.Append(FString::Printf(TEXT("vertice %s"), *Vertice.ToString()) + LINE_TERMINATOR);
			}

			GWarn->StatusForceUpdate(2, 6, FText::FromString("Writing Triangle Data ..."));
			for (const FIndex3i& Triangle : WeldedDynamicMesh.TrianglesItr())
			{
				Data.Append(FString::Printf(TEXT("triangle %s"), *FIntVector(Triangle).ToString()) + LINE_TERMINATOR);
			}

			GWarn->StatusForceUpdate(3, 6, FText::FromString("Writing Delta Data ..."));
			for (const FString& MorphTarget : MorphTargets)
			{
				FString MorphTargetLine = FString::Printf(TEXT("morphtarget %s"), *MorphTarget);

				TArray<FMorphTargetDelta> RawDeltas;
				UMeshOperationsLibrary::GetMorphTargetDeltas(SkeletalMesh, MorphTarget, RawDeltas);

				TArray<FMorphTargetDelta> WeldedDeltas;
				ApplySourceDeltasToDynamicMesh(DynamicMesh, WeldedDynamicMesh, RawDeltas, TSet<int32>(), WeldedDeltas, 1.0, 1.0, 0, 0.0, true);

				if (WeldedDeltas.Num())
				{
					Data.Append(MorphTargetLine + LINE_TERMINATOR);

					for (auto& Delta : WeldedDeltas)
					{
						Data.Append(FString::Printf(TEXT("delta %d:%s"), Delta.SourceIdx, *Delta.PositionDelta.ToString()) + LINE_TERMINATOR);
					}
				}
			}

			GWarn->StatusForceUpdate(4, 6, FText::FromString("Writing Ignore Masks ..."));
			for (UStandaloneMaskSelection* IgnoreMask : IgnoreMasks)
			{
				if (IgnoreMask)
				{
					FString Requirements = FString::Printf(TEXT("requirements %d:%d"), IgnoreMask->MeshVertexCount, IgnoreMask->MeshTriangleCount);
					Data.Append(Requirements + LINE_TERMINATOR);

					for (const int32& Vertice : IgnoreMask->Vertices)
					{
						Data.Append(FString::Printf(TEXT("ignore %d"), Vertice) + LINE_TERMINATOR);
					}
				}
			}

			GWarn->StatusForceUpdate(5, 6, FText::FromString("Writing Move Masks ..."));
			for (UStandaloneMaskSelection* MoveMask : MoveMasks)
			{
				if (MoveMask)
				{
					FString Requirements = FString::Printf(TEXT("requirements %d:%d:%s"), MoveMask->MeshVertexCount, MoveMask->MeshTriangleCount, *MoveMask->GetName());
					Data.Append(Requirements + LINE_TERMINATOR);

					for (const int32& Vertice : MoveMask->Vertices)
					{
						Data.Append(FString::Printf(TEXT("move %d:%s"), Vertice, *MoveMask->Transform.ToString()) + LINE_TERMINATOR);
					}
				}
			}
			Data.Append(FString("#END") + LINE_TERMINATOR);

			// Then find/create it.
			UPackage* Package = CreatePackage(*UserPackageName);
			check(Package);

			// Create SelectionObject object
			UMetaMorph* MetaMorph = NewObject<UMetaMorph>(Package, MetaMorphName, RF_Public | RF_Standalone);

			GWarn->StatusForceUpdate(6, 6, FText::FromString("Store Data ..."));
			MetaMorph->Data.SetNumUninitialized(Data.Len());
			ParallelFor(MetaMorph->Data.Num(), [&](int32 Index)
				{
					MetaMorph->Data[Index] = Data[Index] ^ XorKey[Index % XorKey.Len()];
				});


			MetaMorph->PostLoad();
			MetaMorph->MarkPackageDirty();
			// Notify asset registry of new asset
			FAssetRegistryModule::AssetCreated(MetaMorph);

			GWarn->EndSlowTask();

		}

		return true;
	}
	return false;
}


bool UMeshOperationsLibrary::CreateMetaMorphAssetFromDynamicMeshes(const FDynamicMesh3& BaseMesh, const FDynamicMesh3& MorphedMesh, FString MorphName, const TArray< UStandaloneMaskSelection*>& IgnoreMasks, const TArray< UStandaloneMaskSelection*>& MoveMasks)
{
	if ((BaseMesh.VertexCount() == MorphedMesh.VertexCount()) && BaseMesh.VertexCount() > 0)
	{
		FString PackageName = FString(TEXT("/Game/MetaMorphs/")) + MorphName;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(FText::FromString(FString("Choose New Meta Morph Location")))
			.DefaultAssetPath(FText::FromString(PackageName));
		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			FName MetaMorphName(*FPackageName::GetLongPackageAssetName(UserPackageName));

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MetaMorphName == NAME_None)
			{
				// Use the defaults that were already generated.
				UserPackageName = PackageName;
				MetaMorphName = *Name;
			}

			GWarn->BeginSlowTask(FText::FromString("Create Meta Morph from Mesh Files ..."), true);
			if (!GWarn->GetScopeStack().IsEmpty())
			{
				GWarn->GetScopeStack().Last()->MakeDialog(false, true);
			}
			GWarn->UpdateProgress(0, 6);


			FString Data;

			Data.Append(FString("#METAMORPH FILE") + LINE_TERMINATOR);

			GWarn->StatusForceUpdate(1, 6, FText::FromString("Writing Vertice Data ..."));
			for (const FVector3d& Vertice : BaseMesh.VerticesItr())
			{
				Data.Append(FString::Printf(TEXT("vertice %s"), *Vertice.ToString()) + LINE_TERMINATOR);
			}

			GWarn->StatusForceUpdate(2, 6, FText::FromString("Writing Triangle Data ..."));
			for (const FIndex3i& Triangle : BaseMesh.TrianglesItr())
			{
				Data.Append(FString::Printf(TEXT("triangle %s"), *FIntVector(Triangle).ToString()) + LINE_TERMINATOR);
			}

			GWarn->StatusForceUpdate(3, 6, FText::FromString("Writing Delta Data ..."));


			TArray<FMorphTargetDelta> InitialDeltas;
			UMeshOperationsLibraryRT::GetMorphDeltas(BaseMesh, MorphedMesh, InitialDeltas);

			Data.Append(FString::Printf(TEXT("morphtarget %s"), *MorphName) + LINE_TERMINATOR);

			for (auto& Delta : InitialDeltas)
			{
				Data.Append(FString::Printf(TEXT("delta %d:%s"), Delta.SourceIdx, *Delta.PositionDelta.ToString()) + LINE_TERMINATOR);
			}


			GWarn->StatusForceUpdate(4, 6, FText::FromString("Writing Ignore Masks ..."));
			for (UStandaloneMaskSelection* IgnoreMask : IgnoreMasks)
			{
				if (IgnoreMask)
				{
					FString Requirements = FString::Printf(TEXT("requirements %d:%d"), IgnoreMask->MeshVertexCount, IgnoreMask->MeshTriangleCount);
					Data.Append(Requirements + LINE_TERMINATOR);

					for (const int32& Vertice : IgnoreMask->Vertices)
					{
						Data.Append(FString::Printf(TEXT("ignore %d"), Vertice) + LINE_TERMINATOR);
					}
				}
			}

			GWarn->StatusForceUpdate(5, 6, FText::FromString("Writing Move Masks ..."));
			for (UStandaloneMaskSelection* MoveMask : MoveMasks)
			{
				if (MoveMask)
				{
					FString Requirements = FString::Printf(TEXT("requirements %d:%d:%s"), MoveMask->MeshVertexCount, MoveMask->MeshTriangleCount, *MoveMask->GetName());
					Data.Append(Requirements + LINE_TERMINATOR);

					for (const int32& Vertice : MoveMask->Vertices)
					{
						Data.Append(FString::Printf(TEXT("move %d:%s"), Vertice, *MoveMask->Transform.ToString()) + LINE_TERMINATOR);
					}
				}
			}


			GWarn->StatusForceUpdate(6, 6, FText::FromString("Store Data ..."));
			Data.Append(FString("#END") + LINE_TERMINATOR);

			// Then find/create it.
			UPackage* Package = CreatePackage(*UserPackageName);
			check(Package);

			// Create SelectionObject object
			UMetaMorph* MetaMorph = NewObject<UMetaMorph>(Package, MetaMorphName, RF_Public | RF_Standalone);


			MetaMorph->Data.SetNumUninitialized(Data.Len());
			ParallelFor(MetaMorph->Data.Num(), [&](int32 Index)
				{
					MetaMorph->Data[Index] = Data[Index] ^ XorKey[Index % XorKey.Len()];
				});


			MetaMorph->PostLoad();
			MetaMorph->MarkPackageDirty();
			// Notify asset registry of new asset
			FAssetRegistryModule::AssetCreated(MetaMorph);

			GWarn->EndSlowTask();
			return true;
		}
	}
	return false;
}


bool UMeshOperationsLibrary::AppenedMeshes(USkeletalMesh* SkeletalMesh, TArray<USkeletalMesh*> AdditionalSkeletalMeshes, const bool bWeldMesh, double MergeVertexTolerance, double MergeSearchTolerance, bool OnlyUniquePairs, bool bCreateAdditionalMeshesGroups, FDynamicMesh3& Output)
{
	if (!SkeletalMeshToDynamicMesh(SkeletalMesh, Output))
	{
		return false;
	}

	int32 InitialGroupsCount = 0;

	FDynamicMeshMaterialAttribute* OutputMaterialID = NULL;
	if(bCreateAdditionalMeshesGroups)
	{
		if (Output.HasAttributes() && Output.Attributes()->HasMaterialID())
		{
			OutputMaterialID = Output.Attributes()->GetMaterialID();
			for(const int32 TriangleID : Output.TriangleIndicesItr())
			{
				int32 TriangleGroup = 0;
				OutputMaterialID->GetValue(TriangleID, &TriangleGroup);
				if(TriangleGroup > InitialGroupsCount)
				{
					InitialGroupsCount = TriangleGroup;
				}
			}
		}
		InitialGroupsCount++;
		//UE_LOG(LogTemp, Warning, TEXT("Triangle Groups: %d"), InitialGroupsCount);
	}
	
	FDynamicMeshEditor MainMeshEditor(&Output);

	for(USkeletalMesh* AdditionalSkeketalMesh : AdditionalSkeletalMeshes)
	{
		FDynamicMesh3 CurrentMesh;
		if (SkeletalMeshToDynamicMesh(AdditionalSkeketalMesh, CurrentMesh))
		{
		
			FMeshIndexMappings IndexMappingOriginal;
			MainMeshEditor.AppendMesh(&CurrentMesh, IndexMappingOriginal);
			Output.CompactInPlace();

			if(bCreateAdditionalMeshesGroups)
			{
				int32 LocalGroupsCount = 0;
				FDynamicMeshMaterialAttribute* CurrentMaterialID = NULL;

				if (CurrentMesh.HasAttributes() && CurrentMesh.Attributes()->HasMaterialID())
				{
					CurrentMaterialID = CurrentMesh.Attributes()->GetMaterialID();
				}
			
				for(const int32 TriangleID : CurrentMesh.TriangleIndicesItr())
				{
					const int32 NewTriID = IndexMappingOriginal.GetNewTriangle(TriangleID);

					int32 TriangleGroup = 0;
					if (CurrentMaterialID && OutputMaterialID)
					{
						CurrentMaterialID->GetValue(TriangleID, &TriangleGroup);
						OutputMaterialID->SetValue(NewTriID, &InitialGroupsCount + TriangleGroup);
					}

					if(TriangleGroup > LocalGroupsCount)
					{
						LocalGroupsCount = TriangleGroup;
					}
				}
				InitialGroupsCount = InitialGroupsCount + LocalGroupsCount + 1;
				//UE_LOG(LogTemp, Warning, TEXT("Final Triangle Groups: %d"), InitialGroupsCount);
			}
		}
	}

	

	if(bWeldMesh)
	{
		FMergeCoincidentMeshEdges Merger(&Output);
		Merger.MergeVertexTolerance = MergeVertexTolerance;
		Merger.MergeSearchTolerance = MergeSearchTolerance;
		Merger.OnlyUniquePairs = OnlyUniquePairs;
		if (Merger.Apply())
		{
			Output.CompactInPlace();
		}
	}

	if (FDynamicMeshNormalOverlay* Normals = Output.HasAttributes() ? Output.Attributes()->PrimaryNormals() : nullptr)
	{
		FMeshNormals::InitializeOverlayToPerVertexNormals(Normals);
	}

	return true;
}

bool UMeshOperationsLibrary::SkeletalMeshToSolidDynamicMesh(USkeletalMesh* SkeletalMesh, const FMeshMorpherSolidifyOptions& Options, FDynamicMesh3& OutMesh, const uint32 SubdivisionSteps, const bool bWeldMesh, const double MergeVertexTolerance, const double MergeSearchTolerance, const bool OnlyUniquePairs)
{
	OutMesh.Clear();
	if(!SkeletalMesh)
	{
		return false;
	}

	SkeletalMesh->WaitForPendingInitOrStreaming();

	if (!SkeletalMeshToDynamicMesh(SkeletalMesh, OutMesh))
	{
		return false;
	}

	return ConvertDynamicMeshToSolidDynamicMesh(OutMesh, Options, SubdivisionSteps, bWeldMesh, MergeVertexTolerance, MergeSearchTolerance, OnlyUniquePairs);
}

bool UMeshOperationsLibrary::ConvertDynamicMeshToSolidDynamicMesh(FDynamicMesh3& DynamicMesh, const FMeshMorpherSolidifyOptions& Options, const uint32 SubdivisionSteps, const bool bWeldMesh, const double MergeVertexTolerance, const double MergeSearchTolerance, const bool OnlyUniquePairs)
{
	FDynamicMesh3 SourceVoxelMesh = DynamicMesh;
	{
		FDynamicMeshAABBTree3 Spatial(&SourceVoxelMesh);
		TFastWindingTree<FDynamicMesh3> FastWinding(&Spatial);
		TImplicitSolidify<FDynamicMesh3> Solidify(&SourceVoxelMesh, &Spatial, &FastWinding);
		int32 ClampResolution = FMath::Clamp(Options.GridResolution, 4, Options.GridResolution);
		Solidify.SetCellSizeAndExtendBounds(Spatial.GetBoundingBox(), Options.ExtendBounds, ClampResolution);
		Solidify.WindingThreshold = Options.WindingThreshold;
		Solidify.SurfaceSearchSteps = Options.SurfaceSearchSteps;
		Solidify.bSolidAtBoundaries = Options.bSolidAtBoundaries;
		Solidify.ExtendBounds = Options.ExtendBounds;
		DynamicMesh.Copy(&Solidify.Generate());
	}

	if(bWeldMesh)
	{
		FMergeCoincidentMeshEdges Merger(&SourceVoxelMesh);
		Merger.MergeVertexTolerance = MergeVertexTolerance;
		Merger.MergeSearchTolerance = MergeSearchTolerance;
		Merger.OnlyUniquePairs = OnlyUniquePairs;
		if (Merger.Apply())
		{
			SourceVoxelMesh.CompactInPlace();
		}
	}
	
	for(uint32 X = 0; X < SubdivisionSteps; ++X)
	{
		SubdivideMesh(DynamicMesh);
		DynamicMesh.CompactInPlace();
	}

	return DynamicMesh.VertexCount() > 0;
}

bool UMeshOperationsLibrary::MorphMesh(USkeletalMesh* SourceSkeletalMesh, USkeletalMesh* TargetSkeletalMesh, TArray<FMorphTargetDelta>& OutDeltas)
{
	if(!SourceSkeletalMesh)
	{
		return false;
	}
	
	if(!TargetSkeletalMesh)
	{
		return false;
	}
	
	SourceSkeletalMesh->WaitForPendingInitOrStreaming();
	TargetSkeletalMesh->WaitForPendingInitOrStreaming();

	FDynamicMesh3 SourceMesh;
	if (!SkeletalMeshToDynamicMesh(SourceSkeletalMesh, SourceMesh))
	{
		return false;
	}

	FDynamicMesh3 TargetMesh;
	if (!SkeletalMeshToDynamicMesh(TargetSkeletalMesh, TargetMesh))
	{
		return false;
	}

	FAxisAlignedBox3d SourceBounds = SourceMesh.GetBounds();
	FAxisAlignedBox3d TargetBounds = TargetMesh.GetBounds();
	
	uint32 SubdivisionSteps = 1;
	FMeshMorpherSolidifyOptions Options;
	
	FDynamicMesh3 SourceVoxelMesh = SourceMesh;
	{
		FDynamicMeshAABBTree3 Spatial(&SourceVoxelMesh);
		TFastWindingTree<FDynamicMesh3> FastWinding(&Spatial);
		TImplicitSolidify<FDynamicMesh3> Solidify(&SourceVoxelMesh, &Spatial, &FastWinding);
		int32 ClampResolution = FMath::Clamp(Options.GridResolution, 4, Options.GridResolution);
		Solidify.SetCellSizeAndExtendBounds(Spatial.GetBoundingBox(), Options.ExtendBounds, ClampResolution);
		Solidify.WindingThreshold = Options.WindingThreshold;
		Solidify.SurfaceSearchSteps = Options.SurfaceSearchSteps;
		Solidify.bSolidAtBoundaries = Options.bSolidAtBoundaries;
		Solidify.ExtendBounds = Options.ExtendBounds;
		SourceVoxelMesh.Copy(&Solidify.Generate());
	}
	
	FDynamicMesh3 TargetVoxelMesh = TargetMesh;
	{
		FDynamicMeshAABBTree3 Spatial(&TargetVoxelMesh);
		TFastWindingTree<FDynamicMesh3> FastWinding(&Spatial);
		TImplicitSolidify<FDynamicMesh3> Solidify(&TargetVoxelMesh, &Spatial, &FastWinding);
		int32 ClampResolution = FMath::Clamp(Options.GridResolution, 4, Options.GridResolution);
		Solidify.SetCellSizeAndExtendBounds(Spatial.GetBoundingBox(), Options.ExtendBounds, ClampResolution);
		Solidify.WindingThreshold = Options.WindingThreshold;
		Solidify.SurfaceSearchSteps = Options.SurfaceSearchSteps;
		Solidify.bSolidAtBoundaries = Options.bSolidAtBoundaries;
		Solidify.ExtendBounds = Options.ExtendBounds;
		TargetVoxelMesh.Copy(&Solidify.Generate());
	}


	for(uint32 X =0; X < SubdivisionSteps; ++X)
	{
		SubdivideMesh(SourceVoxelMesh);
		SourceVoxelMesh.CompactInPlace();

		SubdivideMesh(TargetVoxelMesh);
		TargetVoxelMesh.CompactInPlace();
	}
	
	FDynamicMesh3 MergedSourceVoxelMesh = SourceVoxelMesh;
	FMeshIndexMappings IndexMappingVoxelTarget;
	FDynamicMeshEditor VoxelEditor(&MergedSourceVoxelMesh);
	VoxelEditor.AppendMesh(&TargetVoxelMesh, IndexMappingVoxelTarget);
	MergedSourceVoxelMesh.CompactInPlace();

	FMergeCoincidentMeshEdges Merger(&MergedSourceVoxelMesh);
	Merger.MergeVertexTolerance = FMathd::ZeroTolerance;
	Merger.MergeSearchTolerance = 2 * Merger.MergeVertexTolerance;
	Merger.OnlyUniquePairs = false;
	if (Merger.Apply())
	{
		MergedSourceVoxelMesh.CompactInPlace();
	}

	FDynamicMesh3 MergedTargetVoxelMesh = MergedSourceVoxelMesh;

	FDynamicMeshAABBTree3 TargetSpatial(&TargetVoxelMesh);
	FMeshProjectionTarget TargetMeshProjection(&TargetVoxelMesh, &TargetSpatial);

	for(const int& VertexID : MergedTargetVoxelMesh.VertexIndicesItr())
	{
		const FVector3d Position = MergedTargetVoxelMesh.GetVertex(VertexID);
		MergedTargetVoxelMesh.SetVertex(VertexID, TargetMeshProjection.Project(Position), false);
	}


	double NormalIncompatibilityThreshold = 0.5;
	double Threshold = 20;
	double Multiplier = 1.0;
	int32 SmoothIterations = 0;
	double SmoothStrength = 1.0;
	
	TArray<FMorphTargetDelta> FirstDeltas;
	UMeshOperationsLibraryRT::GetMorphDeltas(MergedSourceVoxelMesh, MergedTargetVoxelMesh, FirstDeltas);

	TArray<FMorphTargetDelta> SecondDeltas;
	ApplySourceDeltasToDynamicMesh(MergedSourceVoxelMesh, SourceVoxelMesh, FirstDeltas, TSet<int32>(), SecondDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength);

	OutDeltas.Empty();
	ApplySourceDeltasToDynamicMesh(SourceVoxelMesh, SourceMesh, SecondDeltas, TSet<int32>(), OutDeltas, Threshold, NormalIncompatibilityThreshold, Multiplier, SmoothIterations, SmoothStrength);

	return OutDeltas.Num() > 0;
	
}



#undef LOCTEXT_NAMESPACE