// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

#include "Components/MeshMorpherMeshComponent.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/CollisionProfile.h"

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMesh/MeshTransforms.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMeshToMeshDescription.h"

#include "Changes/MeshVertexChange.h"
#include "Changes/MeshChange.h"
#include "DynamicMesh/DynamicMeshChangeTracker.h"

#include "Util/BufferUtil.h"

#include "BaseTools/MeshSurfacePointTool.h"
#include "InteractiveToolManager.h"

#include "Components/MeshOctree.h"

// default proxy for this component
#include "Components/CustomMeshSceneProxy.h"

using namespace UE::Geometry;

UMeshMorpherMeshComponent::UMeshMorpherMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	InitializeNewMesh();
}

void UMeshMorpherMeshComponent::InitializeMesh(FMeshDescription* MeshDescription)
{
	FMeshDescriptionToDynamicMesh Converter;
	Converter.bPrintDebugMessages = true;
	Mesh->Clear();
	Converter.Convert(MeshDescription, *Mesh);

	Octree->RootDimension = Mesh->GetBounds().MaxDim() * 0.5;
	Octree->Initialize(Mesh.Get());
	
	SpatialData.SetMesh(Mesh.Get(), true);

	SpatialNormals.SetMesh(Mesh.Get());
	SpatialNormals.ComputeVertexNormals();
	
	NotifyMeshUpdated(TArray<int32>(), true);
}

void UMeshMorpherMeshComponent::InitializeMesh(FDynamicMesh3* DynamicMesh)
{
	Mesh->Clear();
	Mesh->Copy(*DynamicMesh);

	Octree->RootDimension = Mesh->GetBounds().MaxDim() * 0.5;
	Octree->Initialize(Mesh.Get());
	
	NotifyMeshUpdated(TArray<int32>(), true);
}



TUniquePtr<FDynamicMesh3> UMeshMorpherMeshComponent::ExtractMesh(bool bNotifyUpdate)
{
	TUniquePtr<FDynamicMesh3> CurMesh = MoveTemp(Mesh);
	InitializeNewMesh();
	if (bNotifyUpdate)
	{
		NotifyMeshUpdated(TArray<int32>());
	}
	return CurMesh;
}


void UMeshMorpherMeshComponent::InitializeNewMesh()
{
	Mesh = MakeUnique<FDynamicMesh3>();
	// discard any attributes/etc initialized by default
	Mesh->Clear();

	Octree = MakeShareable(new FMeshMorpherMeshOctree());
	Octree->Initialize(GetMesh());
	
	SpatialData.SetMesh(Mesh.Get(), true);

	SpatialNormals.SetMesh(Mesh.Get());
	SpatialNormals.ComputeVertexNormals();
}


void UMeshMorpherMeshComponent::Bake(FMeshDescription* MeshDescription, const bool bHaveModifiedTopology, const FConversionToMeshDescriptionOptions& ConversionOptions) const
{
	FDynamicMeshToMeshDescription Converter(ConversionOptions);
	if (bHaveModifiedTopology == false && Converter.HaveMatchingElementCounts(Mesh.Get(), MeshDescription))
	{
		Converter.Update(Mesh.Get(), *MeshDescription);
	}
	else
	{
		Converter.Convert(Mesh.Get(), *MeshDescription);

		//UE_LOG(LogTemp, Warning, TEXT("MeshDescription has %d instances"), MeshDescription->VertexInstances().Num());
	}
}

void UMeshMorpherMeshComponent::UpdateSectionVisibility()
{
	if (CurrentProxy != nullptr)
	{
		if (CurrentProxy != nullptr)
		{
			CurrentProxy->CreateOrUpdateRenderData(TSet<int32>(), true);
		}
	}
}

void UMeshMorpherMeshComponent::CreateOrUpdateSelection()
{
	if (CurrentProxy != nullptr)
	{
		if (CurrentProxy != nullptr)
		{
			CurrentProxy->CreateOrUpdateSelection(true);
		}
	}
}

void UMeshMorpherMeshComponent::NotifyMeshUpdated(const TArray<int32>& VertArray, const bool bUpdateSpatialData)
{
	if (CurrentProxy != nullptr)
	{
		{
			SCOPE_CYCLE_COUNTER(STAT_MeshMorpherSculptToolOctree_UpdateFromDecomp);
			TSet<int32> TrianglesToUpdate;
			if(VertArray.Num() > 0)
			{
				const int32 Cores = VertArray.Num() > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
				const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(VertArray.Num()) / static_cast<double>(Cores)));
				const int32 LastChunkSize = VertArray.Num() - (ChunkSize * Cores);
				const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

				FCriticalSection Lock;
				
				ParallelFor(Chunks, [&](const int32 ChunkIndex)
				{
					TSet<int32> LocalTrianglesToUpdate;
					const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
					for (int X = 0; X < IterationSize; ++X)
					{
						const int32 Index = (ChunkIndex * ChunkSize) + X;
						const int32 vid = VertArray[Index];

						for (int32 tid : Mesh->VtxTrianglesItr(vid))
						{
							LocalTrianglesToUpdate.Add(tid);
						}
					}

					Lock.Lock();
					Octree->ReinsertTriangles(LocalTrianglesToUpdate);
					TrianglesToUpdate.Append(LocalTrianglesToUpdate);
					Lock.Unlock();
					
				});
			}
			
			UpdateNormals();
			Octree->ResetModifiedBounds();
			CurrentProxy->CreateOrUpdateRenderData(TrianglesToUpdate);

			if(TrianglesToUpdate.Num())
			{
				Mesh->UpdateChangeStamps(true, false);
				bMeshChanged = true;
				OnMeshChanged.Broadcast();
			}
			if(bUpdateSpatialData)
			{
				SpatialMesh.Copy(*GetMesh(), false, false, false, false);
				SpatialData.SetMesh(&SpatialMesh, true);

				SpatialNormals.SetMesh(&SpatialMesh);
				SpatialNormals.ComputeVertexNormals();
			}
		}
	}
}

FPrimitiveSceneProxy* UMeshMorpherMeshComponent::CreateSceneProxy()
{
	CurrentProxy = nullptr;
	if (Mesh->TriangleCount() > 0)
	{
		CurrentProxy = new FMeshMorpherMeshSceneProxy(this);

		if (TriangleColorFunc != nullptr)
		{
			CurrentProxy->bUsePerTriangleColor = true;
			CurrentProxy->PerTriangleColorFunc = [this](const int32 TriangleID) { return GetTriangleColor(TriangleID); };
		}
		NotifyMeshUpdated(TArray<int32>(), true);
		//CurrentProxy->CreateOrUpdateRenderData(TArray<int32>(), true);
	}
	return CurrentProxy;
}

int32 UMeshMorpherMeshComponent::GetNumMaterials() const
{
	return OverrideMaterials.Num();
}

void UMeshMorpherMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, const bool bGetDebugMaterials) const
{
	OutMaterials = OverrideMaterials;
}

void UMeshMorpherMeshComponent::ClearMaterials()
{
	OverrideMaterials.Empty();
}

void UMeshMorpherMeshComponent::SetMaterials(const TArray<UMaterialInterface*> Materials)
{
	OverrideMaterials = Materials;
}

void UMeshMorpherMeshComponent::SetSkeletalMaterials(const TArray<FSkeletalMaterial> Materials)
{
	int32 ID = 0;
	for(const FSkeletalMaterial& Material : Materials)
	{
		SetMaterial(ID, Material.MaterialInterface);
		ID++;
	}
}

FColor UMeshMorpherMeshComponent::GetTriangleColor(const int32 TriangleID) const
{
	if (TriangleColorFunc != nullptr)
	{
		return TriangleColorFunc(TriangleID);
	}
	else
	{
		return (TriangleID % 2 == 0) ? FColor::Red : FColor::White;
	}
}

int32 UMeshMorpherMeshComponent::GetMaxGroupID() const
{
	int32 InitialGroupsCount = 0;

	if(Mesh)
	{
		if (Mesh->HasAttributes() && Mesh->Attributes()->HasMaterialID())
		{
			FDynamicMeshMaterialAttribute* OutputMaterialID = Mesh->Attributes()->GetMaterialID();
			for(const int32 TriangleID : Mesh->TriangleIndicesItr())
			{
				int32 TriangleGroup = 0;
				OutputMaterialID->GetValue(TriangleID, &TriangleGroup);
				if(TriangleGroup > InitialGroupsCount)
				{
					InitialGroupsCount = TriangleGroup;
				}
			}
		}
		//InitialGroupsCount++;
		//UE_LOG(LogTemp, Warning, TEXT("Triangle Groups: %d"), InitialGroupsCount);
	}
	return InitialGroupsCount;
}

void UMeshMorpherMeshComponent::GetTriangleROI(const TArray<int32>& VertexROI, TSet<int32>& TriangleROI) const
{
	TriangleROI.Empty();

	const int32 NumVerts = VertexROI.Num();
	if(NumVerts > 0)
	{
		TriangleROI.Reserve((NumVerts < 5) ? NumVerts * 6 : NumVerts * 4);
		const int32 Cores = NumVerts > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(NumVerts) / static_cast<double>(Cores)));
		const int32 LastChunkSize = NumVerts - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		FCriticalSection Lock;
	
		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			TSet<int32> LocalTriangleROI;
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				const int32 vid = VertexROI[Index];

				if(SelectedVertices.Contains(vid) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
				{
					continue;
				}
			
				if(!SelectedVertices.Contains(vid) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
				{
					continue;
				}
				Mesh->EnumerateVertexEdges(vid, [&](const int32 eid)
				{
					const FIndex2i EdgeT = Mesh->GetEdgeT(eid);

					if((SelectedTriangles.Contains(EdgeT.A) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED) || (!SelectedTriangles.Contains(EdgeT.A) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED))
					{
						//do something
					} else
					{
						LocalTriangleROI.Add(EdgeT.A);
					}
	
					if (EdgeT.B != IndexConstants::InvalidID)
					{
						if((SelectedTriangles.Contains(EdgeT.B) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED) || (!SelectedTriangles.Contains(EdgeT.B) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED))
						{
							//do something
						} else
						{
							LocalTriangleROI.Add(EdgeT.B);
						}
					}
				});
			}

			Lock.Lock();
			TriangleROI.Append(LocalTriangleROI);
			Lock.Unlock();
		
		});
	}
}

void UMeshMorpherMeshComponent::GetTriangleROI2(const TSet<int32>& VertexROI, TSet<int32>& TriangleROI) const
{
	TriangleROI.Empty();

	// for a TSet it is more efficient to just try to add each triangle twice, than it is to
	// try to avoid duplicate adds with more complex mesh queries
	const int32 NumVerts = VertexROI.Num();
	TriangleROI.Reserve((NumVerts < 5) ? NumVerts * 6 : NumVerts * 4);

	for (const int32 vid : VertexROI)
	{
		if(SelectedVertices.Contains(vid) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
		{
			continue;
		}
			
		if(!SelectedVertices.Contains(vid) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
		{
			continue;
		}
		Mesh->EnumerateVertexEdges(vid, [&](const int32 eid)
		{
			const FIndex2i EdgeT = Mesh->GetEdgeT(eid);

			if((SelectedTriangles.Contains(EdgeT.A) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED) || (!SelectedTriangles.Contains(EdgeT.A) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED))
			{
				//do something
			} else
			{
				TriangleROI.Add(EdgeT.A);
			}
	
			if (EdgeT.B != IndexConstants::InvalidID)
			{
				if((SelectedTriangles.Contains(EdgeT.B) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED) || (!SelectedTriangles.Contains(EdgeT.B) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED))
				{
					//do something
				} else
				{
					TriangleROI.Add(EdgeT.B);
				}
			}
		});
	}
}

void UMeshMorpherMeshComponent::GetSelectedFromSections(const TArray<int32>& Sections, TSet<int32>& OutSelectedVertices, TSet<int32>& OutSelectedTriangles) const
{
	const FDynamicMeshMaterialAttribute* MaterialID = nullptr;

	OutSelectedVertices.Empty();
	OutSelectedTriangles.Empty();
	const TSet<int32> SectionsSet(Sections);

	OutSelectedVertices.Remove(-1);
	GetTriangleROI2(OutSelectedVertices, OutSelectedTriangles);

	if (Mesh)
	{
		bool bHasMaterial = false;
		if (const bool bHasAttributes = Mesh->HasAttributes())
		{
			bHasMaterial = Mesh->Attributes()->HasMaterialID();
		}

		if (bHasMaterial)
		{
			MaterialID = Mesh->Attributes()->GetMaterialID();
		}

		if (bHasMaterial && MaterialID)
		{
			FCriticalSection Lock;

			ParallelFor(Mesh->VertexCount(), [&](const int32 Index)
			{
				TArray<int32> Triangles;
				Mesh->GetVtxTriangles(Index, Triangles);
				for (const int32& Tri : Triangles)
				{
					int32 TriangleGroup = 0;
					MaterialID->GetValue(Tri, &TriangleGroup);

					if (TriangleGroup >= 0 && SectionsSet.Contains(TriangleGroup))
					{
						Lock.Lock();
						OutSelectedVertices.Add(Index);
						OutSelectedTriangles.Add(Tri);
						Lock.Unlock();
					}
				}
			});
		}
	}
}

FBoxSphereBounds UMeshMorpherMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// Bounds are tighter if the box is generated from pre-transformed vertices.
	FBox BoundingBox(ForceInit);
	for (FVector3d Vertex : Mesh->VerticesItr())
	{
		BoundingBox += LocalToWorld.TransformPosition(Vertex);
	}

	FBoxSphereBounds NewBounds;
	NewBounds.BoxExtent = BoundingBox.GetExtent();
	NewBounds.Origin = BoundingBox.GetCenter();
	NewBounds.SphereRadius = NewBounds.BoxExtent.Size();

	return NewBounds;
}


void UMeshMorpherMeshComponent::ApplyChanges(const TArray<int32>& VertArray1, const TArray<int32>& VertArray2, const bool bUpdateSpatialData)
{
	if(VertArray1.Num() > 0)
	{
		NotifyMeshUpdated(VertArray1, bUpdateSpatialData);
	}

	if(VertArray1.Num() > 0)
	{
		NotifyMeshUpdated(VertArray2, bUpdateSpatialData);
	}
}

void UMeshMorpherMeshComponent::ApplyChange(const FMeshVertexChange* Change, const bool bRevert)
{
	const int32 NV = Change->Vertices.Num();
	if(NV)
	{
		const TArray<FVector3d>& Positions = (bRevert) ? Change->OldPositions : Change->NewPositions;

		const int32 Cores = NV > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(NV) / static_cast<double>(Cores)));
		const int32 LastChunkSize = NV - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				Mesh->SetVertex(Change->Vertices[Index], Positions[Index], false);
			}
		});

		NotifyMeshUpdated(Change->Vertices, true);
	}
}

void UMeshMorpherMeshComponent::ApplyChange(const FMeshChange* Change, const bool bRevert)
{
	TArray<int32> RemoveTriangles, AddTriangles;
	const bool bRemoveOld = !bRevert;
	Change->DynamicMeshChange->GetSavedTriangleList(RemoveTriangles, bRemoveOld);
	Change->DynamicMeshChange->GetSavedTriangleList(AddTriangles, !bRemoveOld);

	
	Octree->RemoveTriangles(RemoveTriangles);

	Change->DynamicMeshChange->Apply(Mesh.Get(), bRevert);

	Octree->InsertTriangles(AddTriangles);

	NotifyMeshUpdated(TArray<int32>(), true);
}

bool UMeshMorpherMeshComponent::HasVertexNormals() const
{
	
	return Mesh.Get()->HasVertexNormals();
}

bool UMeshMorpherMeshComponent::HasAttributes() const
{
	
	return Mesh.Get()->HasAttributes();
}

bool UMeshMorpherMeshComponent::IsVertex(const int32 VertexID) const
{
	
	return Mesh.Get()->IsVertex(VertexID);
}


bool UMeshMorpherMeshComponent::IsReferencedVertex(const int32 VertexID) const
{
	
	return Mesh.Get()->IsReferencedVertex(VertexID);
}

bool UMeshMorpherMeshComponent::IsTriangle(const int32 TriangleID) const
{
	
	return Mesh.Get()->IsTriangle(TriangleID);
}

bool UMeshMorpherMeshComponent::IsEdge(const int32 EdgeID) const
{
	
	return Mesh.Get()->IsEdge(EdgeID);
}

bool UMeshMorpherMeshComponent::GetVertex(const int32 VertexID, FVector& Vertex) const
{
	
	if(Mesh.Get()->IsVertex(VertexID))
	{
		Vertex = Mesh.Get()->GetVertex(VertexID);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::SetVertex(const int32 VertexID, const FVector& vNewPos)
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		Mesh.Get()->SetVertex(VertexID, vNewPos, false);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriangle(const int32 TriangleID, int32& A, int32& B, int32& C) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		const FIndex3i Indices = Mesh.Get()->GetTriangle(TriangleID);
		A = Indices[0];
		B = Indices[1];
		C = Indices[2];
		return true;
	}

	return false;
}

bool UMeshMorpherMeshComponent::GetTriEdges(const int32 TriangleID, int32& EdgeA, int32& EdgeB, int32& EdgeC) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		const FIndex3i Indices = Mesh.Get()->GetTriEdges(TriangleID);
		EdgeA = Indices[0];
		EdgeB = Indices[1];
		EdgeC = Indices[2];
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriNeighbourTris(const int32 TriangleID, int32& TriangleA, int32& TriangleB, int32& TriangleC) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		const FIndex3i Indices = Mesh.Get()->GetTriNeighbourTris(TriangleID);
		TriangleA = Indices[0];
		TriangleB = Indices[1];
		TriangleC = Indices[2];
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriVertices(const int32 TriangleID, FVector& v0, FVector& v1, FVector& v2) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		FVector3d vert0, vert1, vert2;
		Mesh.Get()->GetTriVertices(TriangleID, vert0, vert1, vert2);
		v0 = vert0;
		v1 = vert1;
		v2 = vert2;
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetEdgeV(const int32 EdgeID, int32& A, int32& B) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		const FIndex2i Indices = Mesh.Get()->GetEdgeV(EdgeID);
		A = Indices[0];
		B = Indices[1];
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetEdgeT(const int32 EdgeID, int32& TriangleA, int32& TriangleB) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		const FIndex2i Indices = Mesh.Get()->GetEdgeT(EdgeID);
		TriangleA = Indices[0];
		TriangleB = Indices[1];
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetOrientedBoundaryEdgeV(const int32 EdgeID, int32& A, int32& B) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		const FIndex2i Indices = Mesh.Get()->GetOrientedBoundaryEdgeV(EdgeID);
		A = Indices[0];
		B = Indices[1];
		return true;
	}
	return false;
}

void UMeshMorpherMeshComponent::EnableVertexNormals(const FVector& InitialNormal)
{
	
	Mesh.Get()->EnableVertexNormals(FVector3f(InitialNormal));
}

void UMeshMorpherMeshComponent::DiscardVertexNormals()
{
	
	Mesh.Get()->DiscardVertexNormals();
}

bool UMeshMorpherMeshComponent::GetVertexNormal(const int32 VertexID, FVector& Normal) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		Normal = FVector(Mesh.Get()->GetVertexNormal(VertexID));
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::SetVertexNormal(const int32 VertexID, const FVector& vNewNormal)
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		Mesh.Get()->SetVertexNormal(VertexID, FVector3f(vNewNormal));
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsBoundaryEdge(const int32 EdgeID) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		return Mesh.Get()->IsBoundaryEdge(EdgeID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsBoundaryVertex(const int32 VertexID) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		return Mesh.Get()->IsBoundaryVertex(VertexID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsBoundaryTriangle(const int32 TriangleID) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		return Mesh.Get()->IsBoundaryTriangle(TriangleID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::FindEdge(const int32 VertexA, const int32 VertexB, int32& EdgeID) const
{
	
	if (Mesh.Get()->IsVertex(VertexA) && Mesh.Get()->IsVertex(VertexB))
	{
		EdgeID = Mesh.Get()->FindEdge(VertexA, VertexB);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::FindEdgeFromTri(const int32 VertexA, const int32 VertexB, const int32 TriangleID, int32& EdgeID) const
{
	
	if (Mesh.Get()->IsVertex(VertexA) && Mesh.Get()->IsVertex(VertexB) && Mesh.Get()->IsTriangle(TriangleID))
	{
		EdgeID = Mesh.Get()->FindEdgeFromTri(VertexA, VertexB, TriangleID);
		return true;
	}
	return false;
}


bool UMeshMorpherMeshComponent::FindEdgeFromTriPair(const int32 TriangleA, const int32 TriangleB, int32& EdgeID) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleA) && Mesh.Get()->IsTriangle(TriangleB))
	{
		EdgeID = Mesh.Get()->FindEdgeFromTriPair(TriangleA, TriangleB);
		return true;
	}
	return false;
}


bool UMeshMorpherMeshComponent::FindTriangle(const int32 A, const int32 B, const int32 C, int32& TriangleID) const
{
	
	if (Mesh.Get()->IsVertex(A) && Mesh.Get()->IsVertex(B) && Mesh.Get()->IsVertex(C))
	{
		TriangleID = Mesh.Get()->FindTriangle(A, B, C);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetEdgeOpposingV(const int32 EdgeID, int32& A, int32& B) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		FIndex2i Indices = Mesh.Get()->GetEdgeOpposingV(EdgeID);
		A = Indices[0];
		B = Indices[1];
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetVtxBoundaryEdges(const int32 VertexID, int32& Edge0Out, int32& Edge1Out, int32& Count) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		Count = Mesh.Get()->GetVtxBoundaryEdges(VertexID, Edge0Out, Edge1Out);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetAllVtxBoundaryEdges(const int32 VertexID, TArray<int32>& EdgeListOut, int32& Count) const
{
	
	EdgeListOut.Empty();
	if (Mesh.Get()->IsVertex(VertexID))
	{
		Count = Mesh.Get()->GetAllVtxBoundaryEdges(VertexID, EdgeListOut);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetVtxTriangleCount(const int32 VertexID, int32& Count) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		Count = Mesh.Get()->GetVtxTriangleCount(VertexID);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetVtxTriangles(const int32 VertexID, TArray<int32>& TrianglesOut) const
{
	
	TrianglesOut.Empty();
	if (Mesh.Get()->IsVertex(VertexID))
	{
		const EMeshResult Result = Mesh.Get()->GetVtxTriangles(VertexID, TrianglesOut);
		return Result == EMeshResult::Ok ? true : false;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetVtxContiguousTriangles(const int32 VertexID, TArray<int32>& TrianglesOut, TArray<int32>& ContiguousGroupLengths, TArray<bool>& GroupIsLoop) const
{
	
	TrianglesOut.Empty();
	ContiguousGroupLengths.Empty();
	GroupIsLoop.Empty();
	if (Mesh.Get()->IsVertex(VertexID))
	{
		const EMeshResult Result = Mesh.Get()->GetVtxContiguousTriangles(VertexID, TrianglesOut, ContiguousGroupLengths, GroupIsLoop);
		return Result == EMeshResult::Ok ? true : false;
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsGroupBoundaryEdge(const int32 EdgeID) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		return Mesh.Get()->IsGroupBoundaryEdge(EdgeID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsGroupBoundaryVertex(const int32 VertexID) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		return Mesh.Get()->IsGroupBoundaryVertex(VertexID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsGroupJunctionVertex(const int32 VertexID) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		return Mesh.Get()->IsGroupJunctionVertex(VertexID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetAllVertexGroups(const int32 VertexID, TArray<int32>& GroupsOut) const
{
	
	GroupsOut.Empty();
	if (Mesh.Get()->IsVertex(VertexID))
	{
		return Mesh.Get()->GetAllVertexGroups(VertexID, GroupsOut);
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsBowtieVertex(const int32 VertexID) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		return Mesh.Get()->IsBowtieVertex(VertexID);
	}
	return false;
}

bool UMeshMorpherMeshComponent::IsCompact() const
{
	
	return Mesh.Get()->IsCompact();
}

bool UMeshMorpherMeshComponent::IsCompactV() const
{
	
	return Mesh.Get()->IsCompactV();
}

bool UMeshMorpherMeshComponent::IsCompactT() const
{
	
	return Mesh.Get()->IsCompactT();
}

double UMeshMorpherMeshComponent::CompactMetric() const
{
	
	return Mesh.Get()->CompactMetric();
}

bool UMeshMorpherMeshComponent::IsClosed() const
{
	
	return Mesh.Get()->IsClosed();
}

FBox UMeshMorpherMeshComponent::GetBounds(FVector& Origin) const
{
	
	const FAxisAlignedBox3d LocalBounds = Mesh.Get()->GetBounds();
	Origin = LocalBounds.Center();
	return FBox(LocalBounds.Min, LocalBounds.Max);
}

bool UMeshMorpherMeshComponent::GetTriNormal(const int32 TriangleID, FVector& Normal) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		Normal = FVector(Mesh.Get()->GetTriNormal(TriangleID));
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriArea(const int32 TriangleID, double& Area) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		Area = Mesh.Get()->GetTriArea(TriangleID);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriInfo(const int32 TriangleID, FVector& Normal, double& Area, FVector& Centroid) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		FVector3d LocalNormal, LocalCentroid;
		double LocalArea;
		Mesh.Get()->GetTriInfo(TriangleID, LocalNormal, LocalArea, LocalCentroid);
		Normal = LocalNormal;
		Centroid = LocalCentroid;
		Area = LocalArea;
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriCentroid(const int32 TriangleID, FVector& Centroid) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		Centroid = Mesh.Get()->GetTriCentroid(TriangleID);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriBounds(const int32 TriangleID, FBox& TriBounds) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		const FAxisAlignedBox3d LocalBounds = Mesh.Get()->GetTriBounds(TriangleID);
		TriBounds = FBox(LocalBounds.Min, LocalBounds.Max);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriSolidAngle(const int32 TriangleID, const FVector& Point, double& Angle) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		Angle = Mesh.Get()->GetTriSolidAngle(TriangleID, FVector3d(Point));
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetTriInternalAnglesR(const int32 TriangleID, double& A, double& B, double& C) const
{
	
	if (Mesh.Get()->IsTriangle(TriangleID))
	{
		FVector3d Angles = Mesh.Get()->GetTriInternalAnglesR(TriangleID);
		A = Angles[0];
		B = Angles[1];
		C = Angles[2];
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetEdgeNormal(const int32 EdgeID, FVector& Normal) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		Normal = Mesh.Get()->GetEdgeNormal(EdgeID);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetEdgePoint(const int32 EdgeID, const double ParameterT, FVector& Point) const
{
	
	if (Mesh.Get()->IsEdge(EdgeID))
	{
		Point = Mesh.Get()->GetEdgePoint(EdgeID, ParameterT);
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetVtxOneRingCentroid(const int32 VertexID, FVector& CentroidOut) const
{
	
	if (Mesh.Get()->IsVertex(VertexID))
	{
		FVector3d Centroid;
		Mesh.Get()->GetVtxOneRingCentroid(VertexID, Centroid);
		CentroidOut = Centroid;
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetUniformCentroid(const int32 VertexID, FVector& CentroidOut) const
{
	return GetVtxOneRingCentroid(VertexID, CentroidOut);
}


bool UMeshMorpherMeshComponent::GetMeanValueCentroid(const int32 VertexID, FVector& CentroidOut) const
{
	
	// based on equations in https://www.inf.usi.ch/hormann/papers/Floater.2006.AGC.pdf (formula 9)
	// refer to that paper for variable names/etc

	if (Mesh.Get()->IsVertex(VertexID))
	{
		FVector3d vSum = FVector3d::Zero();
		double wSum = 0;
		const FVector3d Vi = Mesh.Get()->GetVertex(VertexID);

		int32 v_j = FDynamicMesh3::InvalidID, opp_v1 = FDynamicMesh3::InvalidID, opp_v2 = FDynamicMesh3::InvalidID;
		int32 t1 = FDynamicMesh3::InvalidID, t2 = FDynamicMesh3::InvalidID;
		for (const int32 eid : Mesh.Get()->VtxEdgesItr(VertexID))
		{
			opp_v2 = FDynamicMesh3::InvalidID;
			Mesh.Get()->GetVtxNbrhood(eid, VertexID, v_j, opp_v1, opp_v2, t1, t2);

			FVector3d Vj = Mesh.Get()->GetVertex(v_j);
			FVector3d vVj = (Vj - Vi);
			const double len_vVj = vVj.Normalize();
			// [RMS] is this the right thing to do? if vertices are coincident,
			//   weight of this vertex should be very high!
			if (len_vVj < FMathd::ZeroTolerance)
				continue;
			FVector3d vVdelta = (Mesh.Get()->GetVertex(opp_v1) - Vi).GetSafeNormal();
			double w_ij = VectorUtil::VectorTanHalfAngle(vVj, vVdelta);

			if (opp_v2 != FDynamicMesh3::InvalidID) 
			{
				FVector3d vVgamma = (Mesh.Get()->GetVertex(opp_v2) - Vi).GetSafeNormal();
				w_ij += VectorUtil::VectorTanHalfAngle(vVj, vVgamma);
			}

			w_ij /= len_vVj;

			vSum += w_ij * Vj;
			wSum += w_ij;
		}
		if (wSum < FMathd::ZeroTolerance)
		{
			CentroidOut = Vi;
		}
		else {
			CentroidOut = vSum / wSum;
		}
		return true;
	}
	return false;
}



bool UMeshMorpherMeshComponent::GetCotanCentroid(const int32 VertexID, FVector& CentroidOut) const
{
	
	// based on equations in http://www.geometry.caltech.edu/pubs/DMSB_III.pdf
	if (Mesh.Get()->IsVertex(VertexID))
	{
		FVector3d vSum = FVector3d::Zero();
		double wSum = 0;
		const FVector3d Vi = Mesh.Get()->GetVertex(VertexID);

		int32 v_j = FDynamicMesh3::InvalidID, opp_v1 = FDynamicMesh3::InvalidID, opp_v2 = FDynamicMesh3::InvalidID;
		int32 t1 = FDynamicMesh3::InvalidID, t2 = FDynamicMesh3::InvalidID;
		bool bAborted = false;
		for (const int32 eid : Mesh.Get()->VtxEdgesItr(VertexID))
		{
			opp_v2 = FDynamicMesh3::InvalidID;
			Mesh.Get()->GetVtxNbrhood(eid, VertexID, v_j, opp_v1, opp_v2, t1, t2);
			FVector3d Vj = Mesh.Get()->GetVertex(v_j);

			FVector3d Vo1 = Mesh.Get()->GetVertex(opp_v1);
			const double cot_alpha_ij = VectorUtil::VectorCot(
				(Vi - Vo1).GetSafeNormal(), (Vj - Vo1).GetSafeNormal());
			if (cot_alpha_ij == 0) {
				bAborted = true;
				break;
			}
			double w_ij = cot_alpha_ij;

			if (opp_v2 != FDynamicMesh3::InvalidID) {
				FVector3d Vo2 = Mesh.Get()->GetVertex(opp_v2);
				const double cot_beta_ij = VectorUtil::VectorCot(
					(Vi - Vo2).GetSafeNormal(), (Vj - Vo2).GetSafeNormal());
				if (cot_beta_ij == 0) {
					bAborted = true;
					break;
				}
				w_ij += cot_beta_ij;
			}

			vSum += w_ij * Vj;
			wSum += w_ij;
		}
		if (bAborted || fabs(wSum) < FMathd::ZeroTolerance)
		{
			CentroidOut = Vi;
		}
		else {
			CentroidOut = vSum / wSum;
		}
		return true;
	}
	return false;
}

double UMeshMorpherMeshComponent::CalculateWindingNumber(const FVector& QueryPoint) const
{
	
	return Mesh.Get()->CalculateWindingNumber(QueryPoint);
}

int32 UMeshMorpherMeshComponent::GetVertexCount() const
{
	
	return Mesh.Get()->VertexCount();
}

int32 UMeshMorpherMeshComponent::GetTriangleCount() const
{
	
	return Mesh.Get()->TriangleCount();
}

int32 UMeshMorpherMeshComponent::GetEdgeCount() const
{
	return Mesh.Get()->EdgeCount();
}

FVector3d GetVertexWeightsOnTriangle1(const FDynamicMesh3* Mesh, const int TriID, const double TriArea, const bool bWeightByArea, const bool bWeightByAngle)
{
	FVector3d TriNormalWeights = FVector3d::One();
	if (bWeightByAngle)
	{
		TriNormalWeights = Mesh->GetTriInternalAnglesR(TriID); // component-wise multiply by per-vertex internal angles
	}
	if (bWeightByArea)
	{
		TriNormalWeights *= TriArea;
	}
	return TriNormalWeights;
}


void UMeshMorpherMeshComponent::ComputeVertexNormal(const int32 VertexID, const bool bWeightByArea, const bool bWeightByAngle, FVector& Normal) const
{
	FVector3d SumNormal = FVector3d::Zero();
	for (const int TriIdx : Mesh.Get()->VtxTrianglesItr(VertexID))
	{
		FVector3d TriNormal, TriCentroid; double TriArea;
		Mesh.Get()->GetTriInfo(TriIdx, TriNormal, TriArea, TriCentroid);
		FVector3d TriNormalWeights = GetVertexWeightsOnTriangle1(Mesh.Get(), TriIdx, TriArea, bWeightByArea, bWeightByAngle);

		FIndex3i Triangle = Mesh.Get()->GetTriangle(TriIdx);
		const int32 j = IndexUtil::FindTriIndex(VertexID, Triangle);
		SumNormal += TriNormal * TriNormalWeights[j];
	}
	Normal = SumNormal.GetSafeNormal();
}

bool UMeshMorpherMeshComponent::GetBrushPositionOnMesh(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const bool bHitBackFaces, const bool bNeedsSymmetry, const FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition) const
{
	const FVector Origin = GetComponentTransform().InverseTransformPosition(RayOrigin);
	FVector Direction = GetComponentTransform().InverseTransformVectorNoScale(RayDirection);
	Direction.Normalize();


	const int32 HitTID = FindHitSpatialMeshTriangle(Origin, Direction, EyePosition, bHitBackFaces);

	if (HitTID != IndexConstants::InvalidID)
	{
		FVector v0, v1, v2;
		GetTriVertices(HitTID, v0, v1, v2);

		double Distance = 0.0;
		UMeshMorpherToolHelper::FindTriangleIntersectionPoint(Origin, Direction, v0, v1, v2, Distance);

		FVector Normal;
		GetTriNormal(HitTID, Normal);

		FVector r = Origin + Distance * Direction;

		BrushPosition = FBrushPosition(GetComponentTransform().TransformPosition(r), GetComponentTransform().TransformVectorNoScale(Normal));

		if (bNeedsSymmetry)
		{
			FVector SymLocation = BrushPosition.Location.RotateAngleAxis(180, SymmetryAxis);
			SymLocation = FVector(SymLocation.X, SymLocation.Y, BrushPosition.Location.Z);

			FVector SymLocation2 = RayOrigin.RotateAngleAxis(180, SymmetryAxis);
			SymLocation2 = FVector(SymLocation2.X, SymLocation2.Y, RayOrigin.Z);

			const FVector SymOrigin = GetComponentTransform().InverseTransformPosition(SymLocation2);
			FVector SymDirection = GetComponentTransform().InverseTransformVectorNoScale((SymLocation - SymLocation2));
			SymDirection.Normalize();

			const int32 HitTIDSym = FindHitSpatialMeshTriangle(SymOrigin, SymDirection, EyePosition, bHitBackFaces);

			if (HitTIDSym != IndexConstants::InvalidID)
			{
				GetTriVertices(HitTIDSym, v0, v1, v2);

				Distance = 0.0;
				UMeshMorpherToolHelper::FindTriangleIntersectionPoint(SymOrigin, SymDirection, v0, v1, v2, Distance);

				GetTriNormal(HitTIDSym, Normal);

				r = SymOrigin + Distance * SymDirection;

				SymmetricBrushPosition = FBrushPosition(GetComponentTransform().TransformPosition(r), GetComponentTransform().TransformVectorNoScale(Normal));
				return true;
			}
		}
		else {
			return true;
		}
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetBrushPositionOnMeshPlane(const FVector& CurrentBrushPosition, const FVector& RayOrigin, const FVector& RayDirection, const FVector& PlaneOrigin, const FVector& PlaneNormal, const FVector& EyePosition, const FQuat& EyeRotation, const bool bHitBackFaces, const bool bNeedsSymmetry, const FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition) const
{
	FFrame3d BrushPlane(CurrentBrushPosition, EyeRotation.GetForwardVector());
	FVector3d NewHitPosWorld;
	BrushPlane.RayPlaneIntersection(RayOrigin, RayDirection, 2, NewHitPosWorld);

	FFrame3d ActiveDragPlane = FFrame3d(PlaneOrigin, PlaneNormal);

	BrushPosition = FBrushPosition(NewHitPosWorld, ActiveDragPlane.Z());

	if (bNeedsSymmetry)
	{
		FVector SymLocation = BrushPosition.Location.RotateAngleAxis(180, SymmetryAxis);
		SymLocation = FVector(SymLocation.X, SymLocation.Y, BrushPosition.Location.Z);

		FVector SymLocation2 = RayOrigin.RotateAngleAxis(180, SymmetryAxis);
		SymLocation2 = FVector(SymLocation2.X, SymLocation2.Y, RayOrigin.Z);

		FVector Direction = (SymLocation - SymLocation2);

		FVector SymOrigin = GetComponentTransform().InverseTransformPosition(SymLocation2);
		FVector SymDirection = GetComponentTransform().InverseTransformVectorNoScale(Direction);
		SymDirection.Normalize();

		const int32 HitTIDSym = FindHitSpatialMeshTriangle(SymOrigin, SymDirection, EyePosition, bHitBackFaces);


		if (HitTIDSym != IndexConstants::InvalidID)
		{
			FVector v0, v1, v2;
			GetTriVertices(HitTIDSym, v0, v1, v2);

			double Distance = 0.0;
			UMeshMorpherToolHelper::FindTriangleIntersectionPoint(SymOrigin, SymDirection, v0, v1, v2, Distance);

			FVector Normal;
			GetTriNormal(HitTIDSym, Normal);

			FVector r = SymOrigin + Distance * SymDirection;

			SymmetricBrushPosition = FBrushPosition(GetComponentTransform().TransformPosition(r), GetComponentTransform().TransformVectorNoScale(Normal));
			return true;
		}
	}
	else {
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetBrushPositionOnMeshAndPlane(const FVector& RayOrigin, const FVector& RayDirection, const FVector& PlaneOrigin, const FVector& PlaneNormal, const FVector& EyePosition, const bool bHitBackFaces, const bool bNeedsSymmetry, const FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition) const
{
	FVector3d NewHitPosWorld;
	FFrame3d ActiveDragPlane = FFrame3d(PlaneOrigin, PlaneNormal);
	ActiveDragPlane.RayPlaneIntersection(RayOrigin, RayDirection, 2, NewHitPosWorld);

	BrushPosition = FBrushPosition(NewHitPosWorld, ActiveDragPlane.Z());

	if (bNeedsSymmetry)
	{
		FVector SymLocation = BrushPosition.Location.RotateAngleAxis(180, SymmetryAxis);
		SymLocation = FVector(SymLocation.X, SymLocation.Y, BrushPosition.Location.Z);

		FVector SymLocation2 = RayOrigin.RotateAngleAxis(180, SymmetryAxis);
		SymLocation2 = FVector(SymLocation2.X, SymLocation2.Y, RayOrigin.Z);

		FVector Direction = (SymLocation - SymLocation2);

		FVector SymOrigin = GetComponentTransform().InverseTransformPosition(SymLocation2);
		FVector SymDirection = GetComponentTransform().InverseTransformVectorNoScale(Direction);
		SymDirection.Normalize();

		int32 HitTIDSym = FindHitSpatialMeshTriangle(SymOrigin, SymDirection, EyePosition, bHitBackFaces);

		if (HitTIDSym != IndexConstants::InvalidID)
		{
			FVector v0, v1, v2;
			GetTriVertices(HitTIDSym, v0, v1, v2);

			double Distance = 0.0;
			UMeshMorpherToolHelper::FindTriangleIntersectionPoint(SymOrigin, SymDirection, v0, v1, v2, Distance);

			FVector Normal;
			GetTriNormal(HitTIDSym, Normal);

			FVector r = SymOrigin + Distance * SymDirection;

			SymmetricBrushPosition = FBrushPosition(GetComponentTransform().TransformPosition(r), GetComponentTransform().TransformVectorNoScale(Normal));
			return true;
		}
	}
	else {
		return true;
	}
	return false;
}

bool UMeshMorpherMeshComponent::GetDragPlane(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const double BrushSize, const double Depth, const bool bHitBackFaces, FVector& PlaneOrigin, FVector& PlaneNormal) const
{
	FHitResult OutHit;

	if (HitTest(RayOrigin, RayDirection, EyePosition, bHitBackFaces, OutHit))
	{
		FVector r = RayOrigin + OutHit.Distance * RayDirection;
		FVector BrushStartCenterWorld = r + Depth * BrushSize * RayDirection;
		PlaneOrigin = BrushStartCenterWorld;
		PlaneNormal = -RayDirection;
		return true;
	}

	return false;
}

bool UMeshMorpherMeshComponent::HitTest(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const bool bHitBackFaces, FHitResult& OutHit) const
{
	const FVector Origin = GetComponentTransform().InverseTransformPosition(RayOrigin);
	FVector Direction = GetComponentTransform().InverseTransformVectorNoScale(RayDirection);
	Direction.Normalize();

	const int32 HitTID = FindHitSpatialMeshTriangle(Origin, Direction, EyePosition, bHitBackFaces);

	if (HitTID != IndexConstants::InvalidID)
	{
		FVector v0, v1, v2;
		GetTriVertices(HitTID, v0, v1, v2);

		double Distance = 0.0;
		UMeshMorpherToolHelper::FindTriangleIntersectionPoint(Origin, Direction, v0, v1, v2, Distance);

		FVector Normal;
		GetTriNormal(HitTID, Normal);

		const FVector r = Origin + Distance * Direction;

		OutHit.FaceIndex = HitTID;
		OutHit.Distance = Distance;
		OutHit.Normal = Normal;
		OutHit.ImpactPoint = GetComponentTransform().TransformPosition(r);
		return true;
	}
	return false;
}


void UMeshMorpherMeshComponent::GetVertexROIAtLocationInRadius(const FVector& EyePosition, const FVector& BrushPos, const double& BrushSize, const bool bOnlyFacingCamera, TArray<int32>& ROI, TArray<int32>& TriangleROI, FVector& AverageNormal) const
{
	ROI.Empty();
	TriangleROI.Empty();
	AverageNormal = FVector::ZeroVector;
	const double RadiusSqr = (BrushSize * BrushSize);

	const FVector3d LocalBrushPos = FVector3d(GetComponentTransform().InverseTransformPosition(BrushPos));
	const FVector LocalEyePosition(GetComponentTransform().InverseTransformPosition(EyePosition));
	const FAxisAlignedBox3d BrushBox(LocalBrushPos - BrushSize * FVector3d::One(), LocalBrushPos + BrushSize * FVector3d::One());

	TSet<int32> VertexSetBuffer;
	VertexSetBuffer.Reset();
	FVector AvgNormal = FVector::ZeroVector;

	Octree->ParallelRangeQueryArray(BrushBox, TriangleROI, bOnlyFacingCamera, LocalEyePosition, SelectedTriangles, MaskingBehaviour);

	const int32 Count = TriangleROI.Num();
	if(Count > 0)
	{
		FCriticalSection Lock;
		const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
		const int32 LastChunkSize = Count - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			TSet<int32> LocalVertices;
			FVector LocalAvgNormal;
			
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				const int32& TriIdx = TriangleROI[Index];

				FIndex3i TriV = Mesh->GetTriangle(TriIdx);
			
				FVector Normal, Centroid;
				double Area;
				GetTriInfo(TriIdx, Normal, Area, Centroid);
				LocalAvgNormal += Normal;

				for (int32 j = 0; j < 3; ++j)
				{
					if(SelectedVertices.Contains(TriV[j]) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
					{
						continue;
					}
			
					if(!SelectedVertices.Contains(TriV[j]) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
					{
						continue;
					}

					FVector3d Position = Mesh->GetVertex(TriV[j]);
					if ((Position - LocalBrushPos).SquaredLength() < RadiusSqr)
					{
						LocalVertices.Add(TriV[j]);
					}
				}
			}

			Lock.Lock();
			VertexSetBuffer.Append(LocalVertices);
			AvgNormal += LocalAvgNormal;
			Lock.Unlock();
		});
		AverageNormal = AvgNormal / TriangleROI.Num();
		AverageNormal.Normalize();
	}

	ROI.SetNum(0, false);
	BufferUtil::AppendElements(ROI, VertexSetBuffer);
}

void UMeshMorpherMeshComponent::ComputeROIBrushPlane(const TArray<int32>& TriangleROI, const FVector& BrushCenter, const double BrushSize, const double FalloffAmount, const double Depth, const bool bIgnoreDepth, FVector& PlaneOrigin, FVector& PlaneNormal) const
{
	FVector AverageNormal(0, 0, 0);
	FVector AveragePos(0, 0, 0);
	double WeightSum = 0.0;

	const int32 Count = TriangleROI.Num();
	if(Count > 0)
	{
		FCriticalSection Lock;
		const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
		const int32 LastChunkSize = Count - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			FVector LocalAverageNormal(0, 0, 0);
			FVector LocalAveragePos(0, 0, 0);
			double LocalWeightSum = 0.0;
			
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				const int32& TriIdx = TriangleROI[Index];
				FVector Centroid;
				GetTriCentroid(TriIdx, Centroid);

				const double Weight = UMeshMorpherToolHelper::CalculateBrushFalloff(FVector::Distance(BrushCenter, Centroid), BrushSize, FalloffAmount);

				FVector Normal;
				GetTriNormal(TriIdx, Normal);

				LocalAverageNormal += Weight * Normal;
				LocalAveragePos += Weight * Centroid;
				LocalWeightSum += Weight;
			}
			
			Lock.Lock();
			AverageNormal += LocalAverageNormal;
			AveragePos += LocalAveragePos;
			WeightSum += LocalWeightSum;
			Lock.Unlock();
			
		});
	}
	
	AverageNormal.Normalize();
	AveragePos /= WeightSum;

	FFrame3d Result = FFrame3d(AveragePos, AverageNormal);
	if (bIgnoreDepth == false)
	{
		Result.Origin -= (Depth * BrushSize) * Result.Z();
	}
	PlaneOrigin = Result.Origin;
	PlaneNormal = AverageNormal;
}

int32 UMeshMorpherMeshComponent::FindHitSpatialMeshTriangle(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const bool bHitBackFaces) const
{
	const UE::Geometry::TRay<double>& Ray = FRay(RayOrigin, RayDirection);

	if (!bHitBackFaces)
	{
		return Octree->FindNearestHitObject(Ray, [this](const int32 TriangleID)
		{
			if(SelectedTriangles.Contains(TriangleID) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
			{
				return false;
			}

			if(!SelectedTriangles.Contains(TriangleID) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
			{
				return false;
			}

			return true;
		});
	}
	else
	{
		FVector LocalEyePosition(GetComponentTransform().InverseTransformPosition(EyePosition));
		const int32 HitTID = Octree->FindNearestHitObject(Ray, [this, &LocalEyePosition](const int32 TriangleID)
		{
			if(SelectedTriangles.Contains(TriangleID) && MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
			{
				return false;
			}

			if(!SelectedTriangles.Contains(TriangleID) && MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
			{
				return false;
			}
			FVector Normal, Centroid;
			double Area;
			GetTriInfo(TriangleID, Normal, Area, Centroid);
			return FVector::DotProduct(Normal, (Centroid - LocalEyePosition)) < 0.0;
		});
		return HitTID;
	}
}

void UMeshMorpherMeshComponent::RecalculateNormals_PerVertex()
{
	TArray<int32> TrianglesBuffer;
	TrianglesBuffer.Reset();
	Octree->ParallelRangeQuery(Octree->ModifiedBounds, TrianglesBuffer);

	if(TrianglesBuffer.Num() > 0)
	{
		const int32 Cores = TrianglesBuffer.Num() > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(TrianglesBuffer.Num()) / static_cast<double>(Cores)));
		const int32 LastChunkSize = TrianglesBuffer.Num() - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				const FIndex3i TriElems = Mesh.Get()->GetTriangle(TrianglesBuffer[Index]);

				for (int32 j = 0; j < 3; ++j)
				{
					const FVector3d NewNormal = FMeshNormals::ComputeVertexNormal(*Mesh.Get(), TriElems[j]);
					Mesh.Get()->SetVertexNormal(TriElems[j], static_cast<FVector3f>(NewNormal));
				}
			}
		});
	}
}

void UMeshMorpherMeshComponent::RecalculateNormals_Overlay()
{
	FDynamicMeshNormalOverlay* Normals = Mesh.Get()->HasAttributes() ? Mesh.Get()->Attributes()->PrimaryNormals() : nullptr;

	if (!Normals)
	{
		return;
	}

	TArray<int32> TrianglesBuffer;
	TrianglesBuffer.Reset();
	Octree->ParallelRangeQuery(Octree->ModifiedBounds, TrianglesBuffer);

	if(TrianglesBuffer.Num() > 0)
	{
		const int32 Cores = TrianglesBuffer.Num() > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(TrianglesBuffer.Num()) / static_cast<double>(Cores)));
		const int32 LastChunkSize = TrianglesBuffer.Num() - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				const int32 Index = (ChunkIndex * ChunkSize) + X;
				const FIndex3i TriElems = Normals->GetTriangle(TrianglesBuffer[Index]);

				for (int32 j = 0; j < 3; ++j)
				{
					const FVector3d NewNormal = FMeshNormals::ComputeOverlayNormal(*Mesh.Get(), Normals, TriElems[j]);
					Normals->SetElement(TriElems[j], static_cast<FVector3f>(NewNormal));
				}
			}
		});
	}
}


void UMeshMorpherMeshComponent::UpdateNormals()
{
	if (Mesh->HasAttributes() && Mesh->Attributes()->PrimaryNormals() != nullptr)
	{
		RecalculateNormals_Overlay();
	}
	else
	{
		RecalculateNormals_PerVertex();
	}
}

double UMeshMorpherMeshComponent::BrushSizeToBrushRadius(const double BrushSize)
{
	const double MaxDimension = GetMesh()->GetBounds().MaxDim();
	const FInterval1d BrushRelativeSizeRange = FInterval1d(MaxDimension * 0.01, MaxDimension);
	return 0.5 * BrushRelativeSizeRange.Interpolate(BrushSize);
}

void UMeshMorpherMeshComponent::BeginChange()
{
	if (!ActiveVertexChange.IsValid())
	{
		ActiveVertexChange = MakeShareable(new FMeshVertexChangeBuilder());
	}
}


void UMeshMorpherMeshComponent::EndChange()
{
	if (ActiveVertexChange.IsValid())
	{
		UMeshSurfacePointTool* Tool = Cast<UMeshSurfacePointTool>(GetOuter());
		if (Tool)
		{
			Tool->GetToolManager()->EmitObjectChange(this, MoveTemp(ActiveVertexChange->Change), FText::FromString("Brush Stroke"));

		}
		ActiveVertexChange.Reset();
	}
}

void UMeshMorpherMeshComponent::UpdateSavedVertex(const int32 VertexID, const FVector& OldPosition, const FVector& NewPosition)
{
	if (ActiveVertexChange.IsValid())
	{
		ActiveVertexChange->UpdateVertex(VertexID, OldPosition, NewPosition);
	}
}

FTransform UMeshMorpherMeshComponent::ApplyTransform(const FTransform& NewTransform, bool bInvertMask)
{

	FTransform Temp = NewTransform.GetRelativeTransform(LastTransform);

	if (NewTransform.GetScale3D().Equals(LastTransform.GetScale3D()) && NewTransform.GetRotation().Equals(LastTransform.GetRotation()) && NewTransform.GetLocation().Equals(LastTransform.GetLocation()))
	{
		return Temp;
	}

	LastTransform = NewTransform;
	
	FTransformSRT3d Transform3d = FTransformSRT3d(Temp);


	FDynamicMesh3* LocalMesh = GetMesh();

	if (SelectedVertices.Num() > 0 && !bInvertMask)
	{
		for (const int32& Vertice : SelectedVertices)
		{
			const FVector3d OriginalPos = LocalMesh->GetVertex(Vertice);
			const FVector3d NewPos = Transform3d.TransformPosition(OriginalPos);
			LocalMesh->SetVertex(Vertice, NewPos, false);
		}
	}
	else
	{
		if(LocalMesh->VertexCount())
		{
			const int32 Cores = LocalMesh->VertexCount() > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
			const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(LocalMesh->VertexCount()) / static_cast<double>(Cores)));
			const int32 LastChunkSize = LocalMesh->VertexCount() - (ChunkSize * Cores);
			const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

			FCriticalSection BoundsLock;
			ParallelFor(Chunks, [&](int ChunkIndex)
			{
				const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
				FAxisAlignedBox3d ChunkBounds = FAxisAlignedBox3d::Empty();
				for (int X = 0; X < IterationSize; ++X)
				{
					const int32 Index = (ChunkIndex * ChunkSize) + X;
					if (const bool bDoChange = (SelectedVertices.Num() > 0 && bInvertMask && SelectedVertices.Contains(Index)) ? false : true)
					{
						const FVector3d OriginalPos = LocalMesh->GetVertex(Index);
						const FVector3d NewPos = Transform3d.TransformPosition(OriginalPos);
						LocalMesh->SetVertex(Index, NewPos, false);
					}
				}
			});
		}
	}
	return Temp;
}

bool UMeshMorpherMeshComponent::MakeTransformGizmo(const FTransform InitialTransform, const FOnGizmoUpdateTransform& OnTransformUpdate)
{

	UMeshSurfacePointTool* Tool = Cast<UMeshSurfacePointTool>(GetOuter());
	if (Tool && Tool->GetToolManager() && !Transformable.TransformGizmo)
	{
		Transformable.TransformProxy = NewObject<UMeshMorpherTransformProxy>(this);
		Transformable.TransformProxy->Callback = OnTransformUpdate;
		Transformable.TransformProxy->AddMeshComponent(this);

		Transformable.TransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(Tool->GetToolManager()->GetPairedGizmoManager(), ETransformGizmoSubElements::FullTranslateRotateScale, this);
		Transformable.TransformGizmo->SetActiveTarget(Transformable.TransformProxy, Tool->GetToolManager());

		Transformable.TransformGizmo->bUseContextCoordinateSystem = false;
		Transformable.TransformGizmo->CurrentCoordinateSystem = EToolContextCoordinateSystem::Local;

		Transformable.TransformGizmo->SetNewGizmoTransform(InitialTransform);
		return true;
	}
	return false;
}

void UMeshMorpherMeshComponent::DestroyTransformGizmo()
{
	if (Transformable.TransformGizmo)
	{
		UMeshSurfacePointTool* Tool = Cast<UMeshSurfacePointTool>(GetOuter());
		if (Tool && Tool->GetToolManager())
		{
			Tool->GetToolManager()->GetPairedGizmoManager()->DestroyGizmo(Transformable.TransformGizmo);
		}
	}
	Transformable = FMeshMorpherPivotTarget();

}

void UMeshMorpherMeshComponent::SetTransformGizmoVisibility(const bool bNewVisibility)
{
	if (Transformable.TransformGizmo)
	{
		Transformable.TransformGizmo->SetVisibility(bNewVisibility);
	}
}

void UMeshMorpherMeshComponent::SetGizmoTransform(const FTransform Transform, const bool bNoCallback)
{
	if (Transformable.TransformGizmo)
	{
		if (bNoCallback)
		{
			Transformable.TransformGizmo->ReinitializeGizmoTransform(Transform, true);
		}
		else {
			Transformable.TransformGizmo->SetNewGizmoTransform(Transform, true);
		}

	}
}

bool UMeshMorpherMeshComponent::FindNearestTriangleOnSpatialMesh(const FVector& Position, float SearchRadius, int32& TriangleID, FVector& TargetPosOut, FVector& TargetNormalOut)
{
	double fDistSqr;
	TriangleID = SpatialData.FindNearestTriangle(FVector3d(Position), fDistSqr, (float)SearchRadius);
	if (TriangleID <= 0)
	{
		return false;
	}
	FTriangle3d Triangle;
	SpatialMesh.GetTriVertices(TriangleID, Triangle.V[0], Triangle.V[1], Triangle.V[2]);
	FDistPoint3Triangle3d Query(Position, Triangle);
	Query.Get();
	const FIndex3i Tri = SpatialMesh.GetTriangle(TriangleID);
	TargetNormalOut = FVector(Query.TriangleBaryCoords.X * SpatialNormals[Tri.A] + Query.TriangleBaryCoords.Y * SpatialNormals[Tri.B] + Query.TriangleBaryCoords.Z * SpatialNormals[Tri.C]);
	TargetNormalOut.Normalize();
	TargetPosOut = FVector(Query.ClosestTrianglePoint);
	return true;
}