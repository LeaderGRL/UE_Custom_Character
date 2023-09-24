// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "MeshMorpherMeshComponent.h"
#include "Rendering/StaticMeshVertexBuffer.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/ColorVertexBuffer.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/Material.h"
#include "Engine/Engine.h"
#include "DynamicMeshBuilder.h"
#include "StaticMeshResources.h"
#include "SceneManagement.h"   // DrawCircle
#include "BaseGizmos/GizmoMath.h"

#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif

#include "Components/LineBatchComponent.h"

DECLARE_STATS_GROUP(TEXT("MeshMorpherSculptToolOctree"), STATGROUP_MeshMorpherSculptToolOctree, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("MeshMorpherSelection"), STATGROUP_MeshMorpherMeshMorpherSelection, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MeshMorpherSculptToolOctree_UpdateExistingBuffer"), STAT_MeshMorpherSculptToolOctree_UpdateExistingBuffer, STATGROUP_MeshMorpherSculptToolOctree);
DECLARE_CYCLE_STAT(TEXT("MeshMorpherSculptToolOctree_UpdateFromDecomp"), STAT_MeshMorpherSculptToolOctree_UpdateFromDecomp, STATGROUP_MeshMorpherSculptToolOctree);
DECLARE_CYCLE_STAT(TEXT("MeshMorpherSculptToolOctree_InitializeBufferFromOverlay"), STAT_MeshMorpherSculptToolOctree_InitializeBufferFromOverlay, STATGROUP_MeshMorpherSculptToolOctree);
DECLARE_CYCLE_STAT(TEXT("MeshMorpherSculptToolOctree_BufferUpload"), STAT_MeshMorpherSculptToolOctree_BufferUpload, STATGROUP_MeshMorpherSculptToolOctree);
DECLARE_CYCLE_STAT(TEXT("MeshMorpherSelection_BufferUpload"), STATGROUP_MeshMorpherSelection_BufferUpload, STATGROUP_MeshMorpherMeshMorpherSelection);
DECLARE_CYCLE_STAT(TEXT("MeshMorpherSelection_BufferUpdate"), STATGROUP_MeshMorpherSelection_BufferUpdate, STATGROUP_MeshMorpherMeshMorpherSelection);

struct FMeshMorpherMeshRenderData
{
	TArray<FDynamicMeshVertex> VertexData;
	TArray<uint32> Indices;
	TMap<uint32, FIndex3i> TriangleMapping;
};


class FMeshMorpherMeshRenderBuffer final
{
public:
	UMaterialInterface* Material = UMaterial::GetDefaultMaterial(MD_Surface);
	int32 TriangleCount = 0;

	/** The buffer containing vertex data. */
	FStaticMeshVertexBuffers VertexBuffers;
	FDynamicMeshIndexBuffer32 IndexBuffer;
	FLocalVertexFactory VertexFactory;

	explicit FMeshMorpherMeshRenderBuffer(const ERHIFeatureLevel::Type FeatureLevelType) : VertexFactory(FeatureLevelType, "FMeshMorpherMeshRenderBuffer")
	{

	}

	~FMeshMorpherMeshRenderBuffer()
	{
		
		check(IsInRenderingThread());
		if (TriangleCount > 0)
		{
			VertexBuffers.PositionVertexBuffer.ReleaseResource();
			VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			VertexBuffers.ColorVertexBuffer.ReleaseResource();
			VertexFactory.ReleaseResource();
			if (IndexBuffer.IsInitialized())
			{
				IndexBuffer.ReleaseResource();
			}
		}
	}

	void Upload(const FMeshMorpherMeshRenderData& Data)
	{
		check(IsInRenderingThread());
		
		if (TriangleCount > 0)
		{
			VertexBuffers.PositionVertexBuffer.ReleaseResource();
			VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			VertexBuffers.ColorVertexBuffer.ReleaseResource();
			IndexBuffer.ReleaseResource();
		}

		if (Data.VertexData.Num() > 0)
		{
			VertexBuffers.PositionVertexBuffer.Init(Data.VertexData.Num());
			VertexBuffers.StaticMeshVertexBuffer.Init(Data.VertexData.Num(), 1);
			VertexBuffers.ColorVertexBuffer.Init(Data.VertexData.Num());

			for (int32 i = 0; i < Data.VertexData.Num(); i++)
			{
				const FDynamicMeshVertex& Vertex = Data.VertexData[i];

				VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
				VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX.ToFVector3f(), Vertex.GetTangentY(), Vertex.TangentZ.ToFVector3f());
				VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
				VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
			}
		}
		TriangleCount = 0;
		if (Data.Indices.Num() > 0)
		{

			InitOrUpdateResource(&VertexBuffers.PositionVertexBuffer);
			InitOrUpdateResource(&VertexBuffers.StaticMeshVertexBuffer);
			InitOrUpdateResource(&VertexBuffers.ColorVertexBuffer);

			FLocalVertexFactory::FDataType FactoryData;
			VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(&VertexFactory, FactoryData);
			VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(&VertexFactory, FactoryData);
			VertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&VertexFactory, FactoryData);
			VertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(&VertexFactory, FactoryData);
			VertexFactory.SetData(FactoryData);

			TriangleCount = Data.Indices.Num() / 3;
			IndexBuffer.Indices = Data.Indices;
			IndexBuffer.InitResource();
			InitOrUpdateResource(&VertexFactory);
		}
	}

	/**
	 * Fast path to update various vertex buffers. This path does not support changing the
	 * size/counts of any of the sub-buffers, a direct memcopy from the CPU-side buffer to the RHI buffer is used.
	 * @warning This can only be called on the Rendering Thread.
	 */
	void TransferVertexUpdateToGPU(bool bPositions, bool bNormals, bool bTexCoords, bool bColors)
	{
		check(IsInRenderingThread());
		if (TriangleCount == 0)
		{
			return;
		}

		if (bPositions)
		{
			FPositionVertexBuffer& VertexBuffer = VertexBuffers.PositionVertexBuffer;
			void* VertexBufferData = RHILockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockBuffer(VertexBuffer.VertexBufferRHI);
		}
		if (bNormals)
		{
			FStaticMeshVertexBuffer& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
			RHIUnlockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
		}
		if (bColors)
		{
			FColorVertexBuffer& VertexBuffer = VertexBuffers.ColorVertexBuffer;
			void* VertexBufferData = RHILockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
			RHIUnlockBuffer(VertexBuffer.VertexBufferRHI);
		}
		if (bTexCoords)
		{
			FStaticMeshVertexBuffer& VertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
			void* VertexBufferData = RHILockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
			RHIUnlockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
		}
	}	

	// copied from StaticMesh.cpp
	static void InitOrUpdateResource(FRenderResource* Resource)
	{
		check(IsInRenderingThread());

		if (!Resource->IsInitialized())
		{
			Resource->InitResource();
		}
		else
		{
			Resource->UpdateRHI();
		}
	}

protected:
	friend class FMeshMorpherMeshSceneProxy;

	/**
	 * Enqueue a command on the Render Thread to destroy the passed in buffer set.
	 * At this point the buffer set should be considered invalid.
	 */
	static void DestroyRenderBufferSet(FMeshMorpherMeshRenderBuffer* Buffer)
	{
		if (Buffer->TriangleCount == 0)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND(FMeshMorpherMeshRenderBufferDestroy)([Buffer](FRHICommandListImmediate& RHICmdList)
		{
			delete Buffer;
		});
	}

};

inline FVector TransformP(const FVector& Point, const FTransform& TotalTransform)
{
	return TotalTransform.TransformPosition(Point);
}


inline FVector TransformN(const FVector& Normal, const FTransform& TotalTransform)
{
	const FVector& S = TotalTransform.GetScale3D();
	const double DetSign = FMath::Sign(S.X * S.Y * S.Z); // we only need to multiply by the sign of the determinant, rather than divide by it, since we normalize later anyway
	const FVector SafeInvS(S.Y * S.Z * DetSign, S.X * S.Z * DetSign, S.X * S.Y * DetSign);
	return TotalTransform.TransformVectorNoScale((SafeInvS * Normal).GetSafeNormal());
}

inline void InternalDrawCircle(FPrimitiveDrawInterface* PDI, const FVector& Position, const FVector& Normal, const float Radius, const int Steps, const FLinearColor& Color, const float LineThicknessIn, const bool bDepthTestedIn)
{
	if (PDI)
	{
		const FTransform TotalTransform;

		FVector Tan1, Tan2;
		GizmoMath::MakeNormalPlaneBasis((FVector)TransformN(Normal, TotalTransform), Tan1, Tan2);
		Tan1.Normalize(); Tan2.Normalize();

		// this function is from SceneManagement.h
		::DrawCircle(PDI, TransformP(Position, TotalTransform), (FVector)Tan1, (FVector)Tan2,
			Color, Radius, Steps,
			(bDepthTestedIn) ? SDPG_World : SDPG_Foreground,
			LineThicknessIn, 0.0f, true);
	}
}

const int MMBoxFaces[6][4] =
{
	{ 0, 1, 2, 3 },     // back, -z
	{ 5, 4, 7, 6 },     // front, +z
	{ 4, 0, 3, 7 },     // left, -x
	{ 1, 5, 6, 2 },     // right, +x,
	{ 4, 5, 1, 0 },     // bottom, -y
	{ 3, 2, 6, 7 }      // top, +y
};


inline void InternalDrawTransformedLine(FPrimitiveDrawInterface* PDI, const FVector& A, const FVector& B, const FLinearColor& ColorIn, const float LineThicknessIn, const bool bDepthTestedIn)
{
	if (PDI)
	{
		PDI->DrawLine(A, B, ColorIn, (bDepthTestedIn) ? SDPG_World : SDPG_Foreground, LineThicknessIn, 0.0f, true);
	}
}


inline void InternalDrawWireBox(FPrimitiveDrawInterface* PDI, const FBox& Box, const FLinearColor& ColorIn, const float LineThicknessIn, const bool bDepthTestedIn)
{
	const FTransform TotalTransform;
	// corners [ (-x,-y), (x,-y), (x,y), (-x,y) ], -z, then +z
	const FVector Corners[8] =
	{
		TransformP(Box.Min, TotalTransform),
		TransformP(FVector(Box.Max.X, Box.Min.Y, Box.Min.Z), TotalTransform),
		TransformP(FVector(Box.Max.X, Box.Max.Y, Box.Min.Z), TotalTransform),
		TransformP(FVector(Box.Min.X, Box.Max.Y, Box.Min.Z), TotalTransform),
		TransformP(FVector(Box.Min.X, Box.Min.Y, Box.Max.Z), TotalTransform),
		TransformP(FVector(Box.Max.X, Box.Min.Y, Box.Max.Z), TotalTransform),
		TransformP(Box.Max, TotalTransform),
		TransformP(FVector(Box.Min.X, Box.Max.Y, Box.Max.Z), TotalTransform)
	};
	for (int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
	{
		for (int Last = 3, Cur = 0; Cur < 4; Last = Cur++)
		{
			InternalDrawTransformedLine(PDI, Corners[MMBoxFaces[FaceIdx][Last]], Corners[MMBoxFaces[FaceIdx][Cur]],
				ColorIn, LineThicknessIn, bDepthTestedIn);
		}
	}
}

/**
 * Scene Proxy for a mesh buffer.
 *
 * Based on FProceduralMeshSceneProxy but simplified in various ways.
 *
 * Supports wireframe-on-shaded rendering.
 *
 */
class FMeshMorpherMeshSceneProxy final : public FPrimitiveSceneProxy
{
private:
	FMaterialRelevance MaterialRelevance;

public:
	/** Component that created this proxy (is there a way to look this up?) */
	UMeshMorpherMeshComponent* ParentComponent;
	UMaterial* SelectionMaterial = nullptr;

	FColor ConstantVertexColor = FColor::White;
	bool bIgnoreVertexColors = false;
	bool bIgnoreVertexNormals = false;


	bool bUsePerTriangleColor = false;
	TFunction<FColor(int32)> PerTriangleColorFunc = nullptr;

	TArray<FMeshMorpherMeshRenderBuffer*> SectionBuffers;
	TArray<FMeshMorpherMeshRenderData> SectionData;
	FMeshMorpherMeshRenderBuffer* SelectionBuffer = nullptr;
	FMeshMorpherMeshRenderData SelectionData;
	FBox SelectionBox;

	int32 LastTriCount = 0;
	
	FMeshMorpherMeshRenderBuffer* AllocateNewRenderBuffer() const
	{
		// should we hang onto these and destroy them in constructor? leaving to subclass seems risky?
		FMeshMorpherMeshRenderBuffer* RenderBuffer = new FMeshMorpherMeshRenderBuffer(GetScene().GetFeatureLevel());
		return RenderBuffer;
	}

	explicit FMeshMorpherMeshSceneProxy(UMeshMorpherMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		// This is an assumption we are currently making. We do not necessarily require this
		// but if this check is hit then possibly an assumption is wrong
		check(IsInGameThread());
		ParentComponent = Component;

		SelectionMaterial = GEngine->ShadedLevelColorationUnlitMaterial;

		SelectionBuffer = AllocateNewRenderBuffer();
	}

	virtual ~FMeshMorpherMeshSceneProxy() override
	{
		// we are assuming in code below that this is always called from the rendering thread
		check(IsInRenderingThread());

		// destroy all existing renderbuffers
		for (FMeshMorpherMeshRenderBuffer* BufferSet : SectionBuffers)
		{
			FMeshMorpherMeshRenderBuffer::DestroyRenderBufferSet(BufferSet);
		}

		FMeshMorpherMeshRenderBuffer::DestroyRenderBufferSet(SelectionBuffer);
	}

	void CreateOrUpdateSelection(bool GeometryChanged = false)
	{
		SelectionBox = FBox(ForceInit);

		if(SelectionData.Indices.Num() <= 0)
		{
			GeometryChanged = true;
		}
		
		if(GeometryChanged)
		{
			SelectionData = FMeshMorpherMeshRenderData();
		}
			
		if (ParentComponent && ParentComponent->MaskingBehaviour != EMaskingBehaviour::HIDESELECTED)
		{
			FDynamicMesh3* Mesh = ParentComponent->GetMesh();
			FDynamicMeshUVOverlay* UVOverlay = nullptr;
			FDynamicMeshNormalOverlay* NormalOverlay = nullptr;

			bool bHaveColors = Mesh->HasVertexColors() && (bIgnoreVertexColors == false);

			bool bHasMaterial = false;
			FDynamicMeshMaterialAttribute* MaterialID = NULL;
			const bool bHasAttributes = Mesh->HasAttributes();

			if (bHasAttributes)
			{
				UVOverlay = Mesh->Attributes()->PrimaryUV();
				NormalOverlay = Mesh->Attributes()->PrimaryNormals();
			}

			if (bHasAttributes)
			{
				bHasMaterial = Mesh->Attributes()->HasMaterialID();
			}

			if (bHasMaterial)
			{
				MaterialID = Mesh->Attributes()->GetMaterialID();
			}

			for (const int32& TriangleID : ParentComponent->SelectedTriangles)
			{
				
				const FIndex3i Tri = Mesh->GetTriangle(TriangleID);
				const FIndex3i TriUV = (UVOverlay != nullptr) ? UVOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();
				const FIndex3i TriNormal = (NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();

				FColor TriColor = ConstantVertexColor;
				if (bUsePerTriangleColor && PerTriangleColorFunc != nullptr)
				{
					TriColor = PerTriangleColorFunc(TriangleID);
					bHaveColors = false;
				}
				
				FIndex3i* FoundTriangle = SelectionData.TriangleMapping.Find(TriangleID);

				uint32 Indices[3];

				if (FoundTriangle)
				{
					Indices[0] = FoundTriangle->A;
					Indices[1] = FoundTriangle->B;
					Indices[2] = FoundTriangle->C;
				}
				else {
					if(GeometryChanged)
					{
						SelectionData.VertexData.AddZeroed(3); 
						SelectionData.Indices.AddZeroed(3);

						const int32 TriIndex0 = SelectionData.Indices.Num() - 3;
						const int32 TriIndex1 = SelectionData.Indices.Num() - 2;
						const int32 TriIndex2 = SelectionData.Indices.Num() - 1;

						SelectionData.Indices[TriIndex0] = SelectionData.VertexData.Num() - 3;
						SelectionData.Indices[TriIndex1] = SelectionData.VertexData.Num() - 2;
						SelectionData.Indices[TriIndex2] = SelectionData.VertexData.Num() - 1;

						Indices[0] = SelectionData.Indices[TriIndex0];
						Indices[1] = SelectionData.Indices[TriIndex1];
						Indices[2] = SelectionData.Indices[TriIndex2];

						SelectionData.TriangleMapping.Add(TriangleID, FIndex3i(Indices[0], Indices[1], Indices[2]));
					}

				}

				if(!GeometryChanged)
				{
					if(FoundTriangle)
					{
						for (int32 j = 0; j < 3; ++j)
						{
							FVector3f TangentX, TangentY;
							SelectionBuffer->VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[j]) = static_cast<FVector3f>(Mesh->GetVertex(Tri[j]));
							FVector3f Normal = (NormalOverlay != nullptr && TriNormal[j] != FDynamicMesh3::InvalidID) ?	NormalOverlay->GetElement(TriNormal[j]) : Mesh->GetVertexNormal(Tri[j]);

							VectorUtil::MakePerpVectors(Normal, TangentX, TangentY);
							SelectionBox += Mesh->GetVertex(Tri[j]);
							SelectionBuffer->VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(Indices[j], TangentX, TangentY, Normal);
							SelectionBuffer->VertexBuffers.ColorVertexBuffer.VertexColor(Indices[j]) = (bHaveColors) ? FLinearColor(Mesh->GetVertexColor(Tri[j])).ToFColor(false) : TriColor;
						
							const FVector2f UV = (UVOverlay != nullptr && TriUV[j] != FDynamicMesh3::InvalidID) ?	UVOverlay->GetElement(TriUV[j]) : FVector2f::Zero();
							SelectionBuffer->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(Indices[j], 0, UV);
						}
					}
				} else
				{
					for (int32 j = 0; j < 3; ++j)
					{
						FVector3f TangentX, TangentY;
						auto &Vertex = SelectionData.VertexData[Indices[j]];
						Vertex.Position = FVector3f(Mesh->GetVertex(Tri[j]));
						SelectionBox += Mesh->GetVertex(Tri[j]);
						FVector3f Normal = (NormalOverlay != nullptr && TriNormal[j] != FDynamicMesh3::InvalidID) ?	NormalOverlay->GetElement(TriNormal[j]) : Mesh->GetVertexNormal(Tri[j]);

						// calculate a nonsense tangent
						VectorUtil::MakePerpVectors(Normal, TangentX, TangentY);
						Vertex.SetTangents(TangentX, TangentY, Normal);

						const FVector2f UV = (UVOverlay != nullptr && TriUV[j] != FDynamicMesh3::InvalidID) ?	UVOverlay->GetElement(TriUV[j]) : FVector2f::Zero();

						Vertex.TextureCoordinate[0] = UV;
				
						Vertex.Color = (bHaveColors) ? FLinearColor(Mesh->GetVertexColor(Tri[j])).ToFColor(false) : TriColor;
					}
				}
			}
		} else
		{
			SelectionData = FMeshMorpherMeshRenderData();
		}

		if(GeometryChanged)
		{
			FMeshMorpherMeshRenderData& LocalSelection = SelectionData;
			ENQUEUE_RENDER_COMMAND(FCreateOrUpdateSelectionSceneProxyUpload)([this, LocalSelection](FRHICommandListImmediate& RHICmdList)
			{
				SCOPE_CYCLE_COUNTER(STATGROUP_MeshMorpherSelection_BufferUpload);
				SelectionBuffer->Upload(LocalSelection);
			});
		} else
		{
			ENQUEUE_RENDER_COMMAND(FCreateOrUpdateSelectionSceneProxyUpload)([this](FRHICommandListImmediate& RHICmdList)
			{
				SCOPE_CYCLE_COUNTER(STATGROUP_MeshMorpherSelection_BufferUpdate);
				SelectionBuffer->TransferVertexUpdateToGPU(true, true, false, false);
			});
		}
	}
	
	void CreateOrUpdateRenderData(const TSet<int32>& Tris, bool GeometryChanged = false)
	{
		FDynamicMesh3* Mesh = ParentComponent->GetMesh();
		FDynamicMeshUVOverlay* UVOverlay = nullptr;
		FDynamicMeshNormalOverlay* NormalOverlay = nullptr;
		if (Mesh->HasAttributes())
		{
			UVOverlay = Mesh->Attributes()->PrimaryUV();
			NormalOverlay = Mesh->Attributes()->PrimaryNormals();
		}

		if(SectionData.Num() == 0 && SectionBuffers.Num() == 0)
		{
			GeometryChanged = true;
		}
		
		if (GeometryChanged)
		{
			const int32 MaxGroupID = ParentComponent->GetMaxGroupID() + 1;
			SectionData.Empty();
			for (FMeshMorpherMeshRenderBuffer* BufferSet : SectionBuffers)
			{
				FMeshMorpherMeshRenderBuffer::DestroyRenderBufferSet(BufferSet);
			}
			SectionBuffers.Empty();
			SectionData.SetNum(MaxGroupID);
			SectionBuffers.SetNum(MaxGroupID);
			
			for (auto& SectionBuffer : SectionBuffers)
			{
				SectionBuffer = AllocateNewRenderBuffer();
			}

			const TArray<UMaterialInterface*> LocalMaterials = ParentComponent->GetMaterials();

			UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/MeshMorpher/SculptMaterial"));
			if (Material == nullptr)
			{
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
			}

			for (int32 MatIndex = 0; MatIndex < SectionBuffers.Num(); ++MatIndex)
			{
				if (SectionBuffers.IsValidIndex(MatIndex) && LocalMaterials.IsValidIndex(MatIndex) && LocalMaterials[MatIndex] != nullptr)
				{
					SectionBuffers[MatIndex]->Material = LocalMaterials[MatIndex];
				} else
				{
					SectionBuffers[MatIndex]->Material = Material;
				}
			}
		}

		TSet<uint32> ModifiedSections;

		if (Tris.Num() > 0)
		{
			InitializeBuffersFromOverlays(Mesh, Tris, GeometryChanged, UVOverlay, NormalOverlay, ModifiedSections);
		}
		else {
			InitializeBuffersFromOverlays(Mesh, GeometryChanged, UVOverlay, NormalOverlay, ModifiedSections);
		}

		FMeshMorpherMeshSceneProxy* Proxy = this;

		if(GeometryChanged)
		{
			for (int32 Section = 0; Section < SectionData.Num(); ++Section)
			{
				if(Proxy->SectionBuffers.IsValidIndex(Section) && Proxy->SectionBuffers[Section])
				{
					const FMeshMorpherMeshRenderData& RenderData = Proxy->SectionData[Section];
					ENQUEUE_RENDER_COMMAND(FOctreeDynamicMeshSceneProxyInitialize)([Proxy, Section, RenderData](FRHICommandListImmediate& RHICmdList)
					{
						SCOPE_CYCLE_COUNTER(STAT_MeshMorpherSculptToolOctree_BufferUpload);
						Proxy->SectionBuffers[Section]->Upload(RenderData);
					});	
				}
			}
		} else
		{
			for (const uint32& Section : ModifiedSections)
			{
				if(Proxy->SectionBuffers.IsValidIndex(Section) && Proxy->SectionBuffers[Section])
				{
					ENQUEUE_RENDER_COMMAND(FOctreeDynamicMeshSceneProxyUpdate)([Proxy, Section](FRHICommandListImmediate& RHICmdList)
					{
						SCOPE_CYCLE_COUNTER(STAT_MeshMorpherSculptToolOctree_UpdateExistingBuffer);
						Proxy->SectionBuffers[Section]->TransferVertexUpdateToGPU(true, true, false, false);
					});	
				}
			}
		}

		CreateOrUpdateSelection(GeometryChanged);
	}

protected:

	/**
	 * Initialize rendering buffers from given attribute overlays.
	 * Creates three vertices per triangle, IE no shared vertices in buffers.
	 */
	void InitializeBuffersFromOverlays(FDynamicMesh3* Mesh,	const TSet<int32>& Tris, const bool GeometryChanged, FDynamicMeshUVOverlay* UVOverlay, FDynamicMeshNormalOverlay* NormalOverlay, TSet<uint32>& ModifiedSections)
	{
		SCOPE_CYCLE_COUNTER(STAT_MeshMorpherSculptToolOctree_InitializeBufferFromOverlay);

		for(const int32 TriID : Tris)
		{
			
			if(ParentComponent->SelectedTriangles.Contains(TriID) && ParentComponent->MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
			{
				continue;
			}
			
			if(!ParentComponent->SelectedTriangles.Contains(TriID) && ParentComponent->MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
			{
				continue;
			}

			InitializeBuffersPerTriangle(Mesh, TriID, GeometryChanged, UVOverlay, NormalOverlay, ModifiedSections);
		}
	}

	void InitializeBuffersFromOverlays(FDynamicMesh3* Mesh, const bool GeometryChanged, FDynamicMeshUVOverlay* UVOverlay, FDynamicMeshNormalOverlay* NormalOverlay,	TSet<uint32>& ModifiedSections)
	{
		SCOPE_CYCLE_COUNTER(STAT_MeshMorpherSculptToolOctree_InitializeBufferFromOverlay);

		for(const int32 TriID : Mesh->TriangleIndicesItr())
		{
			if(ParentComponent->SelectedTriangles.Contains(TriID) && ParentComponent->MaskingBehaviour == EMaskingBehaviour::HIDESELECTED)
			{
				continue;
			}
			
			if(!ParentComponent->SelectedTriangles.Contains(TriID) && ParentComponent->MaskingBehaviour == EMaskingBehaviour::HIDEUNSELECTED)
			{
				continue;
			}

			InitializeBuffersPerTriangle(Mesh, TriID, GeometryChanged, UVOverlay, NormalOverlay, ModifiedSections);
		}
	}

	void InitializeBuffersPerTriangle(FDynamicMesh3* Mesh,	const int32 TriangleID, const bool GeometryChanged, FDynamicMeshUVOverlay* UVOverlay, FDynamicMeshNormalOverlay* NormalOverlay,	TSet<uint32>& ModifiedSections)
	{
		bool bHaveColors = Mesh->HasVertexColors() && (bIgnoreVertexColors == false);

		bool bHasMaterial = false;
		FDynamicMeshMaterialAttribute* MaterialID = nullptr;

		if (const bool bHasAttributes = Mesh->HasAttributes())
		{
			bHasMaterial = Mesh->Attributes()->HasMaterialID();
		}

		if (bHasMaterial)
		{
			MaterialID = Mesh->Attributes()->GetMaterialID();
		}

		const FIndex3i Tri = Mesh->GetTriangle(TriangleID);
		const FIndex3i TriUV = (UVOverlay != nullptr) ? UVOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();
		const FIndex3i TriNormal = (NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(TriangleID) : FIndex3i::Zero();

		int32 TriangleGroup = 0;
		if (SectionData.Num() > 1 && bHasMaterial && MaterialID)
		{
			MaterialID->GetValue(TriangleID, &TriangleGroup);
		}

		FColor TriColor = ConstantVertexColor;
		if (bUsePerTriangleColor && PerTriangleColorFunc != nullptr)
		{
			TriColor = PerTriangleColorFunc(TriangleID);
			bHaveColors = false;
		}

		if(GeometryChanged)
		{
			if (!SectionData.IsValidIndex(TriangleGroup) || SectionData.Num() <= 0)
			{
				return;
				SectionData.SetNum(TriangleGroup + 1);
			}
		}

		FIndex3i* FoundTriangle = SectionData[TriangleGroup].TriangleMapping.Find(TriangleID);

		uint32 Indices[3];

		if (FoundTriangle)
		{
			Indices[0] = FoundTriangle->A;
			Indices[1] = FoundTriangle->B;
			Indices[2] = FoundTriangle->C;
		}
		else {
			if(GeometryChanged)
			{
				SectionData[TriangleGroup].VertexData.AddZeroed(3); 
				SectionData[TriangleGroup].Indices.AddZeroed(3);

				const int32 TriIndex0 = SectionData[TriangleGroup].Indices.Num() - 3;
				const int32 TriIndex1 = SectionData[TriangleGroup].Indices.Num() - 2;
				const int32 TriIndex2 = SectionData[TriangleGroup].Indices.Num() - 1;

				SectionData[TriangleGroup].Indices[TriIndex0] = SectionData[TriangleGroup].VertexData.Num() - 3;
				SectionData[TriangleGroup].Indices[TriIndex1] = SectionData[TriangleGroup].VertexData.Num() - 2;
				SectionData[TriangleGroup].Indices[TriIndex2] = SectionData[TriangleGroup].VertexData.Num() - 1;

				Indices[0] = SectionData[TriangleGroup].Indices[TriIndex0];
				Indices[1] = SectionData[TriangleGroup].Indices[TriIndex1];
				Indices[2] = SectionData[TriangleGroup].Indices[TriIndex2];

				SectionData[TriangleGroup].TriangleMapping.Add(TriangleID, FIndex3i(Indices[0], Indices[1], Indices[2]));
			}

		}

		ModifiedSections.Add(TriangleGroup);

		if(!GeometryChanged)
		{
			if(FoundTriangle)
			{
				if(SectionBuffers.IsValidIndex(TriangleGroup))
				{
					for (int32 j = 0; j < 3; ++j)
					{
						FVector3f TangentX, TangentY;
						SectionBuffers[TriangleGroup]->VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[j]) = static_cast<FVector3f>(Mesh->GetVertex(Tri[j]));
						FVector3f Normal = (NormalOverlay != nullptr && TriNormal[j] != FDynamicMesh3::InvalidID) ?	NormalOverlay->GetElement(TriNormal[j]) : Mesh->GetVertexNormal(Tri[j]);

						VectorUtil::MakePerpVectors(Normal, TangentX, TangentY);
						
						SectionBuffers[TriangleGroup]->VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(Indices[j], TangentX, TangentY, Normal);
						SectionBuffers[TriangleGroup]->VertexBuffers.ColorVertexBuffer.VertexColor(Indices[j]) = (bHaveColors) ? FLinearColor(Mesh->GetVertexColor(Tri[j])).ToFColor(false) : TriColor;
						
						const FVector2f UV = (UVOverlay != nullptr && TriUV[j] != FDynamicMesh3::InvalidID) ?	UVOverlay->GetElement(TriUV[j]) : FVector2f::Zero();
						SectionBuffers[TriangleGroup]->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(Indices[j], 0, UV);
					}
				}
			}
		} else
		{
			for (int32 j = 0; j < 3; ++j)
			{
				FVector3f TangentX, TangentY;
				auto &Vertex = SectionData[TriangleGroup].VertexData[Indices[j]];
				Vertex.Position = FVector3f(Mesh->GetVertex(Tri[j]));

				FVector3f Normal = (NormalOverlay != nullptr && TriNormal[j] != FDynamicMesh3::InvalidID) ?	NormalOverlay->GetElement(TriNormal[j]) : Mesh->GetVertexNormal(Tri[j]);

				// calculate a nonsense tangent
				VectorUtil::MakePerpVectors(Normal, TangentX, TangentY);
				Vertex.SetTangents(TangentX, TangentY, Normal);

				const FVector2f UV = (UVOverlay != nullptr && TriUV[j] != FDynamicMesh3::InvalidID) ?	UVOverlay->GetElement(TriUV[j]) : FVector2f::Zero();

				Vertex.TextureCoordinate[0] = UV;
				
				Vertex.Color = (bHaveColors) ? FLinearColor(Mesh->GetVertexColor(Tri[j])).ToFColor(false) : TriColor;
			}
		}

	}


public:

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_OctreeDynamicMeshSceneProxy_GetDynamicMeshElements);

		const bool bWireframe = (AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe)	|| ParentComponent->bExplicitShowWireframe;


		//ESceneDepthPriorityGroup DepthPriority = (ParentComponent->bDrawOnTop) ? SDPG_Foreground : SDPG_World;
		ESceneDepthPriorityGroup DepthPriority = SDPG_World;


		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{

			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				bool bHasPrecomputedVolumetricLightmap;
				FMatrix PreviousLocalToWorld;
				int32 SingleCaptureIndex;
				bool bOutputVelocity;
				GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

				// Draw the mesh.
				for (int32 SectionIndex = 0; SectionIndex < SectionBuffers.Num(); ++SectionIndex)
				{
					if (SectionBuffers.IsValidIndex(SectionIndex) && SectionBuffers[SectionIndex] && SectionBuffers[SectionIndex]->TriangleCount > 0)
					{
						const FMeshMorpherMeshRenderBuffer& SectionBuffer = *SectionBuffers[SectionIndex];

						FMaterialRenderProxy* MaterialProxy = SectionBuffers[SectionIndex]->Material ? SectionBuffers[SectionIndex]->Material->GetRenderProxy() : UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface)->GetRenderProxy();

						// do we need separate one of these for each MeshRenderBufferSet?
						FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
						DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);

						DrawBatch(Collector, SectionBuffer, MaterialProxy, false, SDPG_World, ViewIndex, DynamicPrimitiveUniformBuffer);
						if (bWireframe)
						{
							FColoredMaterialRenderProxy* WireframeMaterialInstance = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr, ParentComponent->WireframeColor);

							Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

							FMaterialRenderProxy* WireframeMaterialProxy = WireframeMaterialInstance;
							DrawBatch(Collector, SectionBuffer, WireframeMaterialProxy, true, SDPG_World, ViewIndex, DynamicPrimitiveUniformBuffer);
						}
					}
				}

				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				if (SelectionBuffer && SelectionBuffer->TriangleCount > 0 && ParentComponent->MaskingBehaviour != EMaskingBehaviour::HIDESELECTED)
				{
					const bool bSelectionWireframe = (AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe) || ParentComponent->bDrawMaskSelectionWireframe;

					const FMeshMorpherMeshRenderBuffer& LocalSelectionBuffer = *SelectionBuffer;

					// do we need separate one of these for each MeshRenderBufferSet?
					FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
					DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);

					if (PDI)
					{
						if (ParentComponent->bDrawSelectionPoints)
						{
							for (uint32 i = 0; i < LocalSelectionBuffer.VertexBuffers.PositionVertexBuffer.GetNumVertices(); i++)
							{
								PDI->DrawPoint(FVector(LocalSelectionBuffer.VertexBuffers.PositionVertexBuffer.VertexPosition(i)), ParentComponent->MaskSelectionColor, ParentComponent->SelectionPrimitiveThickness, SDPG_Foreground);
							}
						}

						if(ParentComponent->bDrawSelectionBoundingBox)
						{
							InternalDrawWireBox(PDI, SelectionBox, ParentComponent->MaskSelectionColor, ParentComponent->SelectionPrimitiveThickness, true);
						}
					}

					if (ParentComponent->bDrawMaskSelection)
					{

						auto MaterialProxy = SelectionMaterial->GetRenderProxy();
						auto ColoredMaterialProxy = new FColoredMaterialRenderProxy(MaterialProxy, ParentComponent->MaskSelectionColor);

						Collector.RegisterOneFrameMaterialProxy(ColoredMaterialProxy);
						DrawBatch(Collector, LocalSelectionBuffer, ColoredMaterialProxy, false, SDPG_Foreground, ViewIndex, DynamicPrimitiveUniformBuffer);
					}

					if (bSelectionWireframe)
					{
						FColoredMaterialRenderProxy* WireframeMaterialInstance = new FColoredMaterialRenderProxy(GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr, ParentComponent->MaskSelectionWireframeColor);

						Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

						FMaterialRenderProxy* WireframeMaterialProxy = WireframeMaterialInstance;
						DrawBatch(Collector, LocalSelectionBuffer, WireframeMaterialProxy, true, SDPG_Foreground, ViewIndex, DynamicPrimitiveUniformBuffer);
					}
				}

				if (ParentComponent && PDI)
				{
					if (ParentComponent->bShowIndicators)
					{
						InternalDrawCircle(PDI, ParentComponent->IndicatorPosition.Location, ParentComponent->IndicatorPosition.Normal, ParentComponent->IndicatorRadius, ParentComponent->IndicatorSteps, ParentComponent->IndicatorColor, ParentComponent->IndicatorLineThickness, ParentComponent->bIndicatorDepthTested);
						InternalDrawCircle(PDI, ParentComponent->IndicatorPosition.Location, ParentComponent->IndicatorPosition.Normal, ParentComponent->IndicatorFalloff, ParentComponent->IndicatorSteps, ParentComponent->IndicatorColor, ParentComponent->IndicatorLineThickness, ParentComponent->bIndicatorDepthTested);
						PDI->DrawLine(ParentComponent->IndicatorPosition.Location, ParentComponent->IndicatorPosition.Location + ParentComponent->IndicatorRadius * ParentComponent->IndicatorPosition.Normal, ParentComponent->IndicatorColor, (ParentComponent->bIndicatorDepthTested) ? SDPG_World : SDPG_Foreground, ParentComponent->IndicatorLineThickness, 0.0, true);

						if (ParentComponent->bShowSymmetricIndicator)
						{
							InternalDrawCircle(PDI, ParentComponent->SymmetricIndicatorPosition.Location, ParentComponent->SymmetricIndicatorPosition.Normal, ParentComponent->IndicatorRadius, ParentComponent->IndicatorSteps, ParentComponent->IndicatorColor, ParentComponent->IndicatorLineThickness, ParentComponent->bIndicatorDepthTested);
							InternalDrawCircle(PDI, ParentComponent->SymmetricIndicatorPosition.Location, ParentComponent->SymmetricIndicatorPosition.Normal, ParentComponent->IndicatorFalloff, ParentComponent->IndicatorSteps, ParentComponent->IndicatorColor, ParentComponent->IndicatorLineThickness, ParentComponent->bIndicatorDepthTested);
							PDI->DrawLine(ParentComponent->SymmetricIndicatorPosition.Location, ParentComponent->SymmetricIndicatorPosition.Location + ParentComponent->IndicatorRadius * ParentComponent->SymmetricIndicatorPosition.Normal, ParentComponent->IndicatorColor, (ParentComponent->bIndicatorDepthTested) ? SDPG_World : SDPG_Foreground, ParentComponent->IndicatorLineThickness, 0.0, true);

						}
					}
				}
			}
		}
	}


	void DrawBatch(FMeshElementCollector& Collector,
		const FMeshMorpherMeshRenderBuffer& SectionBuffer,
		FMaterialRenderProxy* UseMaterial,
		const bool bWireframe,
		const ESceneDepthPriorityGroup DepthPriority,
		const int32 ViewIndex,
		FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer) const
	{
		FMeshBatch& Mesh = Collector.AllocateMesh();
		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		BatchElement.IndexBuffer = &SectionBuffer.IndexBuffer;
		Mesh.bWireframe = bWireframe;
		Mesh.VertexFactory = &SectionBuffer.VertexFactory;
		Mesh.MaterialRenderProxy = UseMaterial;

		BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = SectionBuffer.IndexBuffer.Indices.Num() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = SectionBuffer.VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
		Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		Mesh.Type = PT_TriangleList;
		Mesh.DepthPriorityGroup = DepthPriority;
		Mesh.bCanApplyViewModeOverrides = false;
		Collector.AddMesh(ViewIndex, Mesh);
	}



	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;

		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;

		return Result;
	}


	virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const override
	{
		FPrimitiveSceneProxy::GetLightRelevance(LightSceneProxy, bDynamic, bRelevant, bLightMapped, bShadowMapped);
	}


	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
};







