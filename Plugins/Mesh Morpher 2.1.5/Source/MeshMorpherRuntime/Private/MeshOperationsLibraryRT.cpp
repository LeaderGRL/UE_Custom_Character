#include "MeshOperationsLibraryRT.h"
#include "StaticMeshAttributes.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMesh/Operations/MergeCoincidentMeshEdges.h"
#include "StaticMeshOperations.h"
#include "DynamicMesh/MeshNormals.h"
#include "Components/MeshMorpherMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "MeshMorpherSettings.h"

void UMeshOperationsLibraryRT::GetUsedMaterials(USkeletalMesh* SkeletalMesh, int32 LOD, TArray<FSkeletalMaterial>& OutMaterials)
{
	OutMaterials.Empty();
	SkeletalMesh->WaitForPendingInitOrStreaming();
	const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
	if (Resource)
	{
		if (Resource->LODRenderData.IsValidIndex(LOD))
		{
			const FSkeletalMeshLODInfo& SrcLODInfo = *(SkeletalMesh->GetLODInfo(LOD));
			const FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[LOD];
			const int32 NumSections = LODModel.RenderSections.Num();
			TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();

			for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
			{
				const FSkelMeshRenderSection& SkelMeshSection = LODModel.RenderSections[SectionIndex];
				int32 SectionMaterialIndex = SkelMeshSection.MaterialIndex;
				// use the remapping of material indices if there is a valid value
				if (SrcLODInfo.LODMaterialMap.IsValidIndex(SectionIndex) && SrcLODInfo.LODMaterialMap[SectionIndex] != INDEX_NONE)
				{
					SectionMaterialIndex = FMath::Clamp<int32>(SrcLODInfo.LODMaterialMap[SectionIndex], 0, SkeletalMaterials.Num());
				}
				OutMaterials.Add(SkeletalMaterials[SectionMaterialIndex]);
			}
		}
	}
}

bool UMeshOperationsLibraryRT::SkeletalMeshToDynamicMesh_RenderData(USkeletalMesh* SkeletalMesh, FDynamicMesh3& IdenticalDynamicMesh, FDynamicMesh3* WeldedDynamicMesh, const TArray<FFinalSkinVertex>& FinalVertices, int32 LOD)
{
	if (SkeletalMesh)
	{
		SkeletalMesh->WaitForPendingInitOrStreaming();
		const FSkeletalMeshRenderData* Resource = SkeletalMesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(LOD))
			{
				FMeshDescription IdenticalMeshDescription;
				IdenticalMeshDescription.Empty();
				FStaticMeshAttributes StaticMeshAttributesIdentical(IdenticalMeshDescription);
				StaticMeshAttributesIdentical.Register();
				IdenticalDynamicMesh.Clear();

				const FSkeletalMeshLODInfo& SrcLODInfo = *(SkeletalMesh->GetLODInfo(LOD));
				const FSkeletalMeshLODRenderData& LODModel = Resource->LODRenderData[LOD];

				const int32 Numverts = LODModel.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
				bool bUseFinalVertices = Numverts == FinalVertices.Num();

				const int32 NumSections = LODModel.RenderSections.Num();
				int32 TotalTriangles = 0;
				for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
				{
					TotalTriangles += LODModel.RenderSections[SectionIndex].NumTriangles;
				}
				const int32 TotalCorners = TotalTriangles * 3;

				const int32 UVTexNum = bUseFinalVertices ? MAX_TEXCOORDS : LODModel.GetNumTexCoords();

				TPolygonGroupAttributesRef<FName> PolygonGroupImportedMaterialSlotNames = StaticMeshAttributesIdentical.GetPolygonGroupMaterialSlotNames();
				TVertexAttributesRef<FVector3f> VertexPositions = StaticMeshAttributesIdentical.GetVertexPositions();
				TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = StaticMeshAttributesIdentical.GetVertexInstanceTangents();
				TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = StaticMeshAttributesIdentical.GetVertexInstanceBinormalSigns();
				TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = StaticMeshAttributesIdentical.GetVertexInstanceNormals();
				TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = StaticMeshAttributesIdentical.GetVertexInstanceColors();
				TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = StaticMeshAttributesIdentical.GetVertexInstanceUVs();

				IdenticalMeshDescription.ReserveNewVertices(Numverts);
				IdenticalMeshDescription.ReserveNewPolygons(TotalTriangles);
				IdenticalMeshDescription.ReserveNewVertexInstances(TotalCorners);
				IdenticalMeshDescription.ReserveNewEdges(TotalCorners);


				const bool bCreatedWeldedDynamicMesh = WeldedDynamicMesh != NULL;


				VertexInstanceUVs.SetNumChannels(UVTexNum);

				TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();

				for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
				{
					const FSkelMeshRenderSection& SkelMeshSection = LODModel.RenderSections[SectionIndex];
					int32 SectionMaterialIndex = SkelMeshSection.MaterialIndex;

					// use the remapping of material indices if there is a valid value
					if (SrcLODInfo.LODMaterialMap.IsValidIndex(SectionIndex) && SrcLODInfo.LODMaterialMap[SectionIndex] != INDEX_NONE)
					{
						SectionMaterialIndex = FMath::Clamp<int32>(SrcLODInfo.LODMaterialMap[SectionIndex], 0, SkeletalMaterials.Num());
					}


					FName ImportedMaterialSlotName = *FString("MAT" + FString::FromInt(SectionMaterialIndex));
					const FPolygonGroupID SectionPolygonGroupID(SectionIndex);
					if (!IdenticalMeshDescription.IsPolygonGroupValid(SectionPolygonGroupID))
					{
						IdenticalMeshDescription.CreatePolygonGroupWithID(SectionPolygonGroupID);
						PolygonGroupImportedMaterialSlotNames[SectionPolygonGroupID] = ImportedMaterialSlotName;
					}
					for (uint32 SectionTriangleIndex = 0; SectionTriangleIndex < SkelMeshSection.NumTriangles; ++SectionTriangleIndex)
					{
						TArray<FVertexInstanceID> VertexInstanceIDs;
						VertexInstanceIDs.SetNum(3);

						for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
						{
							const int32 Index = SkelMeshSection.BaseIndex + ((SectionTriangleIndex * 3) + CornerIndex);
							const int32 WedgeIndex = LODModel.MultiSizeIndexContainer.GetIndexBuffer()->Get(Index);

							const FVertexID VertexID(WedgeIndex);
							if (!IdenticalMeshDescription.IsVertexValid(VertexID))
							{
								const FVector3f Position = bUseFinalVertices ? FinalVertices[WedgeIndex].Position : LODModel.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(WedgeIndex);
								IdenticalMeshDescription.CreateVertexWithID(VertexID);
								VertexPositions[VertexID] = Position;
							}

							FVertexInstanceID VertexInstanceID = IdenticalMeshDescription.CreateVertexInstance(VertexID);
							VertexInstanceIDs[CornerIndex] = VertexInstanceID;

							for (int32 UVLayerIndex = 0; UVLayerIndex < UVTexNum; UVLayerIndex++)
							{
								const FVector2f UV = bUseFinalVertices ? FVector2f(FinalVertices[WedgeIndex].TextureCoordinates[UVLayerIndex].X, FinalVertices[WedgeIndex].TextureCoordinates[UVLayerIndex].Y) : LODModel.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(WedgeIndex, UVLayerIndex);
								VertexInstanceUVs.Set(VertexInstanceID, UVLayerIndex, UV);
							}

							const FVector3f TangentX = bUseFinalVertices ? FinalVertices[WedgeIndex].TangentX.ToFVector3f() : FVector3f(LODModel.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(WedgeIndex));
							const FVector3f TangentY = bUseFinalVertices ? FinalVertices[WedgeIndex].GetTangentY() : LODModel.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentY(WedgeIndex);
							const FVector4f TangentZ = bUseFinalVertices ? FVector4f(FinalVertices[WedgeIndex].TangentZ.ToFVector4()) : LODModel.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(WedgeIndex);

							VertexInstanceTangents[VertexInstanceID] = TangentX;
							VertexInstanceNormals[VertexInstanceID] = TangentZ;
							VertexInstanceBinormalSigns[VertexInstanceID] = GetBasisDeterminantSign(FVector(TangentX.GetSafeNormal()), FVector(TangentY.GetSafeNormal()), FVector(TangentZ.GetSafeNormal())); //HERE

							if ((uint32)WedgeIndex < LODModel.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices())
							{
								VertexInstanceColors[VertexInstanceID] = FLinearColor(LODModel.StaticVertexBuffers.ColorVertexBuffer.VertexColor(WedgeIndex));
							}
							else {
								VertexInstanceColors[VertexInstanceID] = FLinearColor::White;
							}
						}
						const FPolygonID NewPolygonID = IdenticalMeshDescription.CreatePolygon(SectionPolygonGroupID, VertexInstanceIDs);
					}
				}

				FMeshDescriptionToDynamicMesh Converter;
				Converter.bEnableOutputGroups = true;
				Converter.Convert(&IdenticalMeshDescription, IdenticalDynamicMesh);

				if (bCreatedWeldedDynamicMesh)
				{
					WeldedDynamicMesh->Copy(IdenticalDynamicMesh);
					FMergeCoincidentMeshEdges Merger(WeldedDynamicMesh);
					
					UMeshMorpherSettings* Settings = GetMutableDefault<UMeshMorpherSettings>();
					if(Settings)
					{
						Merger.MergeVertexTolerance = Settings->MergeVertexTolerance == 0.0 ? FMathd::ZeroTolerance : Settings->MergeVertexTolerance;
						Merger.MergeSearchTolerance = Settings->MergeSearchTolerance;
						Merger.OnlyUniquePairs = Settings->OnlyUniquePairs;
					} else
					{
						Merger.MergeVertexTolerance = FMathd::ZeroTolerance;
						Merger.MergeSearchTolerance = 2 * Merger.MergeVertexTolerance;
						Merger.OnlyUniquePairs = false;
					}
					if (Merger.Apply())
					{
						WeldedDynamicMesh->CompactInPlace();

						FDynamicMeshNormalOverlay* Normals = WeldedDynamicMesh->HasAttributes() ? WeldedDynamicMesh->Attributes()->PrimaryNormals() : nullptr;

						if (Normals)
						{
							FMeshNormals::InitializeOverlayToPerVertexNormals(Normals);
						}
					}

				}
				return true;
			}
		}
	}
	return false;
}

void UMeshOperationsLibraryRT::CreateEmptyLODModel(FMorphTargetLODModel& LODModel)
{
	FMorphTargetDelta EmptyDelta;
	EmptyDelta.PositionDelta = FVector3f::ZeroVector;
	EmptyDelta.TangentZDelta = FVector3f::ZeroVector;
	EmptyDelta.SourceIdx = 0;
	TArray<FMorphTargetDelta> Deltas;
	Deltas.Add(EmptyDelta);
	LODModel.Vertices = Deltas;
	LODModel.SectionIndices.Add(0);
	LODModel.NumBaseMeshVerts = 1;
	LODModel.bGeneratedByEngine = false;
}

UMorphTarget* UMeshOperationsLibraryRT::FindMorphTarget(USkeletalMesh* Mesh, FString MorphTargetName)
{
	if (Mesh)
	{
		TArray<UMorphTarget*>& LocalMorphTargets = Mesh->GetMorphTargets();
		for (UMorphTarget* MorphTargetObj : LocalMorphTargets)
		{
			if (MorphTargetObj && !MorphTargetObj->IsUnreachable())
			{
				if (MorphTargetObj->GetName().Equals(MorphTargetName))
				{
					return MorphTargetObj;
				}
			}
		}
	}
	return nullptr;
}


void UMeshOperationsLibraryRT::CreateMorphTargetObj(USkeletalMesh* Mesh, FString MorphTargetName, bool bInvalidateRenderData)
{
	if (Mesh)
	{
		const FSkeletalMeshRenderData* Resource = Mesh->GetResourceForRendering();
		if (Resource)
		{
			if (Resource->LODRenderData.IsValidIndex(0))
			{
				
				UMorphTarget* MorphTargetObj = FindMorphTarget(Mesh, MorphTargetName);
				bool bCreated = false;
				if (!MorphTargetObj)
				{
#if WITH_EDITOR
					Mesh->Modify();
					Mesh->InvalidateDeriveDataCacheGUID();
#endif
					MorphTargetObj = NewObject<UMorphTarget>(Mesh, FName(*FString::Printf(TEXT("%s"), *MorphTargetName)));
				}

				MorphTargetObj->GetMorphLODModels().Empty();

				auto& MorphData = MorphTargetObj->GetMorphLODModels().AddDefaulted_GetRef();
				FMorphTargetDelta EmptyDelta;
				EmptyDelta.PositionDelta = FVector3f::ZeroVector;
				EmptyDelta.TangentZDelta = FVector3f::ZeroVector;
				EmptyDelta.SourceIdx = Resource->LODRenderData[0].RenderSections[0].BaseVertexIndex;
				TArray<FMorphTargetDelta> Deltas;
				Deltas.Add(EmptyDelta);
				MorphData.Vertices = Deltas;
				MorphData.SectionIndices.Add(0);
				MorphData.NumBaseMeshVerts = Resource->LODRenderData[0].GetNumVertices();
				MorphData.bGeneratedByEngine = false;
				MorphTargetObj->BaseSkelMesh = Mesh;

				bool bRegistered = Mesh->RegisterMorphTarget(MorphTargetObj, bInvalidateRenderData);
				MorphTargetObj->MarkPackageDirty();
			}
		}
	}
}

void UMeshOperationsLibraryRT::GetMorphTargetDeltas(USkeletalMesh* Mesh, UMorphTarget* MorphTarget, TArray<FMorphTargetDelta>& Deltas, int32 LOD)
{
	if (Mesh && MorphTarget)
	{
		if (MorphTarget->HasDataForLOD(LOD))
		{
			Deltas = MorphTarget->GetMorphLODModels()[LOD].Vertices;
		}
	}
}

void UMeshOperationsLibraryRT::GetMorphTargetNames(USkeletalMesh* Mesh, TArray<FString>& MorphTargets)
{
	if (Mesh)
	{
		MorphTargets.Empty();
		TArray<UMorphTarget*>& LocalMorphTargets = Mesh->GetMorphTargets();
		for (UMorphTarget* MorphTarget : LocalMorphTargets)
		{
			if (MorphTarget)
			{
				MorphTargets.AddUnique(MorphTarget->GetName());
			}
		}

	}
}


void UMeshOperationsLibraryRT::GetMorphDeltas(const FDynamicMesh3& Original, const FDynamicMesh3& Changed, TArray<FMorphTargetDelta>& Deltas)
{
	Deltas.Empty();

	FCriticalSection Lock;

	const int32 Count = Changed.VertexCount();
	if(Count > 0)
	{
		const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
		const int32 LastChunkSize = Count - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			TArray<FMorphTargetDelta> LocalDeltas;
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				if (Original.IsVertex(Index))
				{
					const FVector ChangedLocation = Changed.GetVertex(Index);
					const FVector OriginalLocation = Original.GetVertex(Index);

					if (!ChangedLocation.Equals(OriginalLocation))
					{
						const FVector NewPosition = ChangedLocation - OriginalLocation;
						if (NewPosition.SizeSquared() > FMath::Square(DOUBLE_THRESH_POINTS_ARE_NEAR))
						{
							const FVector ChangedNormal = FMeshNormals::ComputeVertexNormal(Changed, Index);
							const FVector OriginalNormal = FMeshNormals::ComputeVertexNormal(Original, Index);
							FMorphTargetDelta& NewMorphDelta = LocalDeltas.AddZeroed_GetRef();
							NewMorphDelta.PositionDelta = FVector3f(NewPosition);
							NewMorphDelta.TangentZDelta = FVector3f(ChangedNormal - OriginalNormal);
							NewMorphDelta.SourceIdx = Index;
						}
					}
				}
			}
			if(LocalDeltas.Num() > 0)
			{
				Lock.Lock();
				Deltas.Append(LocalDeltas);
				Lock.Unlock();
			}
		});
	}
}

bool UMeshOperationsLibraryRT::CopyMeshComponent(UMeshMorpherMeshComponent* Source, UMeshMorpherMeshComponent* Target)
{
	if (Source && Target)
	{
		FDynamicMesh3* Mesh = Source->GetMesh();

		if (Mesh)
		{
			Target->InitializeMesh(Mesh);

			for (int32 i = 0; i < Source->OverrideMaterials.Num(); ++i)
			{
				Target->SetMaterial(i, Source->OverrideMaterials[i]);
			}
			return true;
		}
	}

	return false;
}

bool UMeshOperationsLibraryRT::SkeletalMeshToDynamicMesh(USkeletalMesh* SkeletalMesh, int32 LOD, UMeshMorpherMeshComponent* IdenticalMeshComponent, UMeshMorpherMeshComponent* WeldedMeshComponent)
{
	if (IdenticalMeshComponent)
	{
		FDynamicMesh3 IdenticalMesh, WeldedMesh;
		if (WeldedMeshComponent)
		{
			const bool bResult = SkeletalMeshToDynamicMesh_RenderData(SkeletalMesh, IdenticalMesh, &WeldedMesh, TArray<FFinalSkinVertex>(), LOD);
			IdenticalMeshComponent->InitializeMesh(&IdenticalMesh);
			WeldedMeshComponent->InitializeMesh(&WeldedMesh);

			if (SkeletalMesh->GetMaterials().Num() > 0)
			{
				WeldedMeshComponent->OverrideMaterials.Empty();
				IdenticalMeshComponent->OverrideMaterials.Empty();
				for (int32 i = 0; i < SkeletalMesh->GetMaterials().Num(); ++i)
				{
					WeldedMeshComponent->SetMaterial(i, SkeletalMesh->GetMaterials()[i].MaterialInterface);
					IdenticalMeshComponent->SetMaterial(i, SkeletalMesh->GetMaterials()[i].MaterialInterface);
				}
			}

			return bResult;
		}
		else {
			const bool bResult = SkeletalMeshToDynamicMesh_RenderData(SkeletalMesh, IdenticalMesh, NULL, TArray<FFinalSkinVertex>(), LOD);
			IdenticalMeshComponent->InitializeMesh(&IdenticalMesh);
			if (SkeletalMesh->GetMaterials().Num() > 0)
			{
				IdenticalMeshComponent->OverrideMaterials.Empty();
				for (int32 i = 0; i < SkeletalMesh->GetMaterials().Num(); ++i)
				{
					IdenticalMeshComponent->SetMaterial(i, SkeletalMesh->GetMaterials()[i].MaterialInterface);
				}
			}

			return bResult;
		}
	}
	return false;
}

bool UMeshOperationsLibraryRT::SkeletalMeshComponentToDynamicMesh(USkeletalMeshComponent* SkeletalMeshComponent, int32 LOD, bool bUseSkinnedVertices, UMeshMorpherMeshComponent* IdenticalMeshComponent, UMeshMorpherMeshComponent* WeldedMeshComponent)
{
	if (IdenticalMeshComponent && SkeletalMeshComponent->SkeletalMesh)
	{
		TArray<FFinalSkinVertex> Vertexes;

		if (bUseSkinnedVertices)
		{
			SkeletalMeshComponent->GetCPUSkinnedVertices(Vertexes, LOD);
		}

		FDynamicMesh3 IdenticalMesh, WeldedMesh;
		if (WeldedMeshComponent)
		{
			const bool bResult = SkeletalMeshToDynamicMesh_RenderData(SkeletalMeshComponent->SkeletalMesh, IdenticalMesh, &WeldedMesh, Vertexes, LOD);
			IdenticalMeshComponent->InitializeMesh(&IdenticalMesh);
			WeldedMeshComponent->InitializeMesh(&WeldedMesh);
			TArray<UMaterialInterface*> OutMaterials;
			SkeletalMeshComponent->GetUsedMaterials(OutMaterials);
			if (OutMaterials.Num() > 0)
			{
				WeldedMeshComponent->OverrideMaterials.Empty();
				IdenticalMeshComponent->OverrideMaterials.Empty();
				for (int32 i = 0; i < OutMaterials.Num(); ++i)
				{
					WeldedMeshComponent->SetMaterial(i, OutMaterials[i]);
					IdenticalMeshComponent->SetMaterial(i, OutMaterials[i]);
				}
			}

			return bResult;
		}
		else {
			const bool bResult = SkeletalMeshToDynamicMesh_RenderData(SkeletalMeshComponent->SkeletalMesh, IdenticalMesh, NULL, Vertexes, LOD);
			IdenticalMeshComponent->InitializeMesh(&IdenticalMesh);
			TArray<UMaterialInterface*> OutMaterials;
			SkeletalMeshComponent->GetUsedMaterials(OutMaterials);
			if (OutMaterials.Num() > 0)
			{
				IdenticalMeshComponent->OverrideMaterials.Empty();
				for (int32 i = 0; i < OutMaterials.Num(); ++i)
				{
					IdenticalMeshComponent->SetMaterial(i, OutMaterials[i]);
				}
			}

			return bResult;
		}
	}
	return false;
}