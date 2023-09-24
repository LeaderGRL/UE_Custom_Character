// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "InteractiveToolObjects.h"
#include "Changes/MeshVertexChange.h"
#include "Changes/MeshChange.h"
#include "Templates/SharedPointer.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MeshConversionOptions.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "DynamicMesh/MeshNormals.h"

#include "MeshMorpherTransformProxy.h"
#include "BaseGizmos/GizmoComponents.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "Engine/SkeletalMesh.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif
#include "MeshMorpherToolHelper.h"
#include "MeshMorpherMeshComponent.generated.h"

// predecl
struct FMeshDescription;
class UStandaloneMaskSelection;
class FMeshMorpherMeshOctree;

/** internal FPrimitiveSceneProxy defined in OctreeDynamicMeshSceneProxy.h */
class FMeshMorpherMeshSceneProxy;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnVertexIDROI, int32, VertexID);

USTRUCT()
struct FMeshMorpherPivotTarget
{
	GENERATED_BODY()
public:
	UPROPERTY()
		TObjectPtr<UMeshMorpherTransformProxy> TransformProxy = nullptr;

	UPROPERTY()
		TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;
};


UENUM(BlueprintType)
enum class EMaskingBehaviour : uint8
{
	NONE,
	HIDESELECTED,
	HIDEUNSELECTED,
};


UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), hidecategories = (LOD, Physics, Collision), editinlinenew, ClassGroup = Rendering)
class MESHMORPHERRUNTIME_API UMeshMorpherMeshComponent : public UMeshComponent, public IToolFrameworkComponent, public IMeshVertexCommandChangeTarget, public IMeshCommandChangeTarget
{
	friend class UMeshMorpherTransformProxy;
	friend class UMeshMorpherTool;
	GENERATED_UCLASS_BODY()
public:
	/**
	 * initialize the internal mesh from a MeshDescription
	 */
	void InitializeMesh(FMeshDescription* MeshDescription);
	void InitializeMesh(FDynamicMesh3* DynamicMesh);
	/**
	 * @return pointer to internal mesh
	 */
	FDynamicMesh3* GetMesh() { return Mesh.Get(); }

	/**
	 * @return the current internal mesh, which is replaced with an empty mesh
	 */
	TUniquePtr<FDynamicMesh3> ExtractMesh(bool bNotifyUpdate);

	/**
	 * Write the internal mesh to a MeshDescription
	 * @param bHaveModifiedTopology if false, we only update the vertex positions in the MeshDescription, otherwise it is Empty()'d and regenerated entirely
	 * @param ConversionOptions struct of additional options for the conversion
	 */
	void Bake(FMeshDescription* MeshDescription, const bool bHaveModifiedTopology, const FConversionToMeshDescriptionOptions& ConversionOptions) const;

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component")
		void UpdateSectionVisibility();
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component")
		void CreateOrUpdateSelection();

	/**
	* Write the internal mesh to a MeshDescription with default conversion options
	* @param bHaveModifiedTopology if false, we only update the vertex positions in the MeshDescription, otherwise it is Empty()'d and regenerated entirely
	*/
	void Bake(FMeshDescription* MeshDescription, const bool bHaveModifiedTopology)
	{
		const FConversionToMeshDescriptionOptions ConversionOptions;
		Bake(MeshDescription, bHaveModifiedTopology, ConversionOptions);
	}

	
	virtual void NotifyMeshUpdated(const TArray<int32>& VertArray, const bool bUpdateSpatialData = false);

	/**
	 * Apply a vertex deformation change to the internal mesh
	 */
	virtual void ApplyChange(const FMeshVertexChange* Change, const bool bRevert) override;

	/**
	* Apply a general mesh change to the internal mesh
	*/
	virtual void ApplyChange(const FMeshChange* Change, const bool bRevert) override;

	UFUNCTION(BlueprintCallable, Meta = (AutoCreateRefTerm = "VertArray2"), Category = "Mesh Morpher|Mesh Component")
		void ApplyChanges(const TArray<int32>& VertArray1, const TArray<int32>& VertArray2, const bool bUpdateSpatialData = false);

	/**
	 * This delegate fires when a FCommandChange is applied to this component, so that
	 * parent objects know the mesh has changed.
	 */
	FSimpleMulticastDelegate OnMeshChanged;


	/**
	 * @return true if wireframe rendering pass is enabled (default false)
	 */
	virtual bool EnableWireframeRenderPass() const { return false; }

	/**
	 * if true, we always show the wireframe on top of the shaded mesh, even when not in wireframe mode
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component")
		bool bExplicitShowWireframe = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component")
		FLinearColor WireframeColor = FLinearColor(0, 0.5f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component")
		bool bMeshChanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		UStandaloneMaskSelection* LastStandaloneMaskSelection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		TSet<int32> SelectedVertices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		TSet<int32> SelectedTriangles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		FTransform LastTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		bool bDrawSelectionBoundingBox = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		bool bDrawSelectionPoints = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		float SelectionPrimitiveThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection|Edges")
		bool bDrawMaskSelection = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection|Points")
		FLinearColor MaskSelectionColor = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Selection")
		bool bDrawMaskSelectionWireframe = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bDrawMaskEdges"), Category = "Mesh Morpher|Mesh Component|Selection")
		FLinearColor MaskSelectionWireframeColor = FLinearColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		bool bShowIndicators = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		FBrushPosition IndicatorPosition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		FBrushPosition SymmetricIndicatorPosition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		bool bShowSymmetricIndicator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		int32 IndicatorSteps = 32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		FLinearColor IndicatorColor = FLinearColor::Black;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		float IndicatorLineThickness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		double IndicatorRadius = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		double IndicatorFalloff = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		bool bIndicatorDepthTested = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Morpher|Mesh Component|Indicator")
		EMaskingBehaviour MaskingBehaviour = EMaskingBehaviour::NONE;
	
	UPROPERTY(Transient)
		FMeshMorpherPivotTarget Transformable;

	TFunction<FColor(int32)> TriangleColorFunc = nullptr;

	TSharedPtr<FMeshMorpherMeshOctree> Octree;

	FDynamicMesh3 SpatialMesh;
	UE::Geometry::FDynamicMeshAABBTree3 SpatialData;
	UE::Geometry::FMeshNormals SpatialNormals;

private:
	TSharedPtr<FMeshVertexChangeBuilder> ActiveVertexChange = nullptr;
	FMeshMorpherMeshSceneProxy* CurrentProxy = nullptr;

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, const bool bGetDebugMaterials = false) const override;

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Materials")
		void ClearMaterials();
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Materials")
		void SetMaterials(TArray<UMaterialInterface*> Materials);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Materials")
		void SetSkeletalMaterials(const TArray<FSkeletalMaterial> Materials);
	
	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	//~ Begin USceneComponent Interface.

	TUniquePtr<FDynamicMesh3> Mesh;

public:

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		FColor GetTriangleColor(const int32 TriangleID) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		int32 GetMaxGroupID() const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|ROI")
		void GetTriangleROI(const TArray<int32>& VertexROI, TSet<int32>& TriangleROI) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|ROI")
		void GetTriangleROI2(const TSet<int32>& VertexROI, TSet<int32>& TriangleROI) const;
	
	UFUNCTION(BlueprintPure, Meta = (AutoCreateRefTerm = "Sections, Bones"), Category = "Mesh Morpher|Mesh Component|Selection")
		void GetSelectedFromSections(const TArray<int32>& Sections, TSet<int32>& OutSelectedVertices, TSet<int32>& OutSelectedTriangles) const;

	//friend class FCustomMeshSceneProxy;

	void InitializeNewMesh();

	/** @return true if this mesh has per-vertex normals */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		bool HasVertexNormals() const;

	/** @return true if this mesh has attribute layers */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		bool HasAttributes() const;


	/** @return true if VertexID is a valid vertex in this mesh */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool IsVertex(const int32 VertexID) const;

	/** @return true if VertexID is a valid vertex in this mesh AND is used by at least one triangle */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool IsReferencedVertex(const int32 VertexID) const;

	/** @return true if TriangleID is a valid triangle in this mesh */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool IsTriangle(const int32 TriangleID) const;

	/** @return true if EdgeID is a valid edge in this mesh */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool IsEdge(const int32 EdgeID) const;

	/** @return the vertex position */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVertex(const int32 VertexID, FVector& Vertex) const;

	/** Set vertex position */
	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool SetVertex(const int32 VertexID, const FVector& vNewPos);

	/** Get triangle vertices */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriangle(const int32 TriangleID, int32& A, int32& B, int32& C) const;

	/** Get triangle edges */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriEdges(const int32 TriangleID, int32& EdgeA, int32& EdgeB, int32& EdgeC) const;

	/** Find the neighbour triangles of a triangle (any of them might be InvalidID) */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriNeighbourTris(const int32 TriangleID, int32& TriangleA, int32& TriangleB, int32& TriangleC) const;

	/** Get the three vertex positions of a triangle */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriVertices(const int32 TriangleID, FVector& v0, FVector& v1, FVector& v2) const;

	/** Get the vertex pair for an edge */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool GetEdgeV(const int32 EdgeID, int32& A, int32& B) const;

	/** Get the triangle pair for an edge. The second triangle may be InvalidID */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool GetEdgeT(const int32 EdgeID, int32& TriangleA, int32& TriangleB) const;

	/** Return edge vertex indices, but oriented based on attached triangle (rather than min-sorted) */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool GetOrientedBoundaryEdgeV(const int32 EdgeID, int32& A, int32& B) const;

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		void EnableVertexNormals(const FVector& InitialNormal);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		void DiscardVertexNormals();

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVertexNormal(const int32 VertexID, FVector& Normal) const;

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool SetVertexNormal(const int32 VertexID, const FVector& vNewNormal);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool IsBoundaryEdge(const int32 EdgeID) const;

	/** Returns true if the vertex is part of any boundary edges */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool IsBoundaryVertex(const int32 VertexID) const;

	/** Returns true if any edge of triangle is a boundary edge */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool IsBoundaryTriangle(const int32 TriangleID) const;

	/** Find id of edge connecting A and B */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool FindEdge(const int32 VertexA, const int32 VertexB, int32& EdgeID) const;

	/** Find edgeid for edge [a,b] from triangle that contains the edge. Faster than FindEdge() because it is constant-time. */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool FindEdgeFromTri(const int32 VertexA, const int32 VertexB, const int32 TriangleID, int32& EdgeID) const;

	/** Find edgeid for edge connecting two triangles */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool FindEdgeFromTriPair(const int32 TriangleA, const int32 TriangleB, int32& EdgeID) const;

	/** Find triangle made up of any permutation of vertices [a,b,c] */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool FindTriangle(const int32 A, const int32 B, const int32 C, int32& TriangleID) const;

	/**
	 * If edge has vertices [a,b], and is connected two triangles [a,b,c] and [a,b,d],
	 * this returns [c,d], or [c,InvalidID] for a boundary edge
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool GetEdgeOpposingV(const int32 EdgeID, int32& A, int32& B) const;

	/**
	 * Returns count of boundary edges at vertex, and the first two boundary
	 * edges if found. If Count is > 2, call GetAllVtxBoundaryEdges
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVtxBoundaryEdges(const int32 VertexID, int32& Edge0Out, int32& Edge1Out, int32& Count) const;

	/**
	 * Find edge ids of boundary edges connected to vertex.
	 * @param VertexID Vertex ID
	 * @param EdgeListOut boundary edge IDs are appended to this list
	 * @param Count		  count of number of elements of e that were filled
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetAllVtxBoundaryEdges(const int32 VertexID, TArray<int32>& EdgeListOut, int32& Count) const;

	/**
	 * return # of triangles attached to VertexID, or -1 if invalid vertex
	 * if bBruteForce = true, explicitly checks, which creates a list and is expensive
	 * default is false, uses orientation, no memory allocation
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVtxTriangleCount(const int32 VertexID, int32& Count) const;

	/**
	 * Get triangle one-ring at vertex.
	 * bUseOrientation is more efficient but returns incorrect result if vertex is a bowtie
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVtxTriangles(const int32 VertexID, TArray<int32>& TrianglesOut) const;

	/**
	* Get triangles connected to vertex in contiguous order, with multiple groups if vertex is a bowtie.
	* @param VertexID Vertex ID to search around
	* @param TrianglesOut All triangles connected to the vertex, in contiguous order; if there are multiple contiguous groups they are packed one after another
	* @param ContiguousGroupLengths Lengths of contiguous groups packed into TrianglesOut (if not a bowtie, this will just be a length-one array w/ {TrianglesOut.Num()})
	* @param GroupIsLoop Indicates whether each contiguous group is a loop (first triangle connected to last) or not
	*/
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVtxContiguousTriangles(const int32 VertexID, TArray<int32>& TrianglesOut, TArray<int32>& ContiguousGroupLengths, TArray<bool>& GroupIsLoop) const;

	/** Returns true if the two triangles connected to edge have different group IDs */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool IsGroupBoundaryEdge(const int32 EdgeID) const;

	/** Returns true if vertex has more than one tri group in its tri nbrhood */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool IsGroupBoundaryVertex(const int32 VertexID) const;

	/** Returns true if more than two group boundary edges meet at vertex (ie 3+ groups meet at this vertex) */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool IsGroupJunctionVertex(const int32 VertexID) const;

	/** Returns all group IDs at vertex */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetAllVertexGroups(const int32 VertexID, TArray<int32>& GroupsOut) const;

	/** returns true if VertexID is a "bowtie" vertex, ie multiple disjoint triangle sets in one-ring */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool IsBowtieVertex(const int32 VertexID) const;

	/** returns true if vertices, edges, and triangles are all dense (Count == MaxID) **/
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		bool IsCompact() const;

	/** @return true if vertex count == max vertex id */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		bool IsCompactV() const;

	/** @return true if triangle count == max triangle id */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		bool IsCompactT() const;

	/** returns measure of compactness in range [0,1], where 1 is fully compacted */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		double CompactMetric() const;

	/** @return true if mesh has no boundary edges */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		bool IsClosed() const;

	/** Returns bounding box of all mesh vertices (including unreferenced vertices) */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		FBox GetBounds(FVector& Origin) const;

	/** Calculate face normal of triangle */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriNormal(const int32 TriangleID, FVector& Normal) const;

	/** Calculate area triangle */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriArea(const int32 TriangleID, double& Area) const;

	/**
	 * Compute triangle normal, area, and centroid all at once. Re-uses vertex
	 * lookups and computes normal & area simultaneously. *However* does not produce
	 * the same normal/area as separate calls, because of this.
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriInfo(const int32 TriangleID, FVector& Normal, double& Area, FVector& Centroid) const;

	/** Compute centroid of triangle */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriCentroid(const int32 TriangleID, FVector& Centroid) const;

	/** Construct bounding box of triangle as efficiently as possible */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriBounds(const int32 TriangleID, FBox& TriBounds) const;

	/** Compute solid angle of oriented triangle tID relative to Point - see WindingNumber() */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriSolidAngle(const int32 TriangleID, const FVector& Point, double& Angle) const;

	/** 
	 * Compute internal angles at all vertices of triangle 
	 * @param A AB vs AC angle
	 * @param B BA vs BC angle
	 * @param C AC vs BC angle
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		bool GetTriInternalAnglesR(const int32 TriangleID, double& A, double& B, double& C) const;

	/** Returns average normal of connected face normals */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool GetEdgeNormal(const int32 EdgeID, FVector& Normal) const;

	/** Get point along edge, t clamped to range [0,1] */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		bool GetEdgePoint(const int32 EdgeID, const double ParameterT, FVector& Point) const;

	/**
	 * Fastest possible one-ring centroid. This is used inside many other algorithms
	 * so it helps to have it be maximally efficient
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetVtxOneRingCentroid(const int32 VertexID, FVector& CentroidOut) const;

	/**
	 * Compute uniform centroid of a vertex one-ring.
	 * These weights are strictly positive and all equal to 1 / valence
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetUniformCentroid(const int32 VertexID, FVector& CentroidOut) const;

	/**
	 * Compute mean-value centroid of a vertex one-ring.
	 * These weights are strictly positive.
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetMeanValueCentroid(const int32 VertexID, FVector& CentroidOut) const;

	/**
	 * Compute cotan-weighted centroid of a vertex one-ring.
	 * These weights are numerically unstable if any of the triangles are degenerate.
	 * We catch these problems and return input vertex as centroid
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		bool GetCotanCentroid(const int32 VertexID, FVector& CentroidOut) const;

	/**
	 * Compute mesh winding number, from Jacobson et. al., Robust Inside-Outside Segmentation using Generalized Winding Numbers
	 * http://igl.ethz.ch/projects/winding-number/
	 * returns ~0 for points outside a closed, consistently oriented mesh, and a positive or negative integer
	 * for points inside, with value > 1 depending on how many "times" the point inside the mesh (like in 2D polygon winding)
	 */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		double CalculateWindingNumber(const FVector& QueryPoint) const;

	/** @return number of vertices in the mesh */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		int32 GetVertexCount() const;

	/** @return number of triangles in the mesh */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Triangle")
		int32 GetTriangleCount() const;

	/** @return number of edges in the mesh */
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Edge")
		int32 GetEdgeCount() const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|Vertex")
		void ComputeVertexNormal(const int32 VertexID, const bool bWeightByArea, const bool bWeightByAngle, FVector& Normal) const;

	UFUNCTION(BlueprintCallable, Meta = (AutoCreateRefTerm = "VertArray"), Category = ToolLibrary)
		void UpdateNormals();
	
	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Hit Test")
		int32 FindHitSpatialMeshTriangle(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const bool bHitBackFaces) const;

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		void RecalculateNormals_PerVertex();

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|DynamicMesh")
		void RecalculateNormals_Overlay();

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Hit Test")
		bool HitTest(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const bool bHitBackFaces, FHitResult& OutHit) const;

	UFUNCTION(BlueprintPure, Meta = (AutoCreateRefTerm = "EyePosition"), Category = "Mesh Morpher|ROI")
		void GetVertexROIAtLocationInRadius(const FVector& EyePosition, const FVector& BrushPos, const double& BrushSize, const bool bOnlyFacingCamera, TArray<int32>& ROI, TArray<int32>& TriangleROI, FVector& AverageNormal) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|ROI")
		void ComputeROIBrushPlane(const TArray<int32>& TriangleROI, const FVector& BrushCenter, const double BrushSize, const double FalloffAmount, const double Depth, const bool bIgnoreDepth, FVector& PlaneOrigin, FVector& PlaneNormal) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Brush")
		bool GetBrushPositionOnMesh(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const bool bHitBackFaces, const bool bNeedsSymmetry, const FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Brush")
		bool GetBrushPositionOnMeshPlane(const FVector& CurrentBrushPosition, const FVector& RayOrigin, const FVector& RayDirection, const FVector& PlaneOrigin, const FVector& PlaneNormal, const FVector& EyePosition, const FQuat& EyeRotation, const bool bHitBackFaces, const bool bNeedsSymmetry, const FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Brush")
		bool GetBrushPositionOnMeshAndPlane(const FVector& RayOrigin, const FVector& RayDirection, const FVector& PlaneOrigin, const FVector& PlaneNormal, const FVector& EyePosition, const bool bHitBackFaces, const bool bNeedsSymmetry, const FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Brush")
		bool GetDragPlane(const FVector& RayOrigin, const FVector& RayDirection, const FVector& EyePosition, const double BrushSize, const double Depth, const bool bHitBackFaces, FVector& PlaneOrigin, FVector& PlaneNormal) const;

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Gizmo")
		double BrushSizeToBrushRadius(const double BrushSize);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Undo")
		void BeginChange();

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Undo")
		void EndChange();

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Undo")
		void UpdateSavedVertex(const int32 VertexID, const FVector& OldPosition, const FVector& NewPosition);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Transform")
		FTransform ApplyTransform(const FTransform& NewTransform, bool bInvertMask);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Transform|Gizmo")
		bool MakeTransformGizmo(const FTransform InitialTransform, const FOnGizmoUpdateTransform& OnTransformUpdate);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Transform|Gizmo")
		void DestroyTransformGizmo();

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Transform|Gizmo")
		void SetTransformGizmoVisibility(const bool bNewVisibility);

	UFUNCTION(BlueprintCallable, Category = "Mesh Morpher|Mesh Component|Transform|Gizmo")
		void SetGizmoTransform(const FTransform Transform, const bool bNoCallback);

	UFUNCTION(BlueprintPure, Category = "Mesh Morpher|Mesh Component|SpatialMesh")
		bool FindNearestTriangleOnSpatialMesh(const FVector& Position, float SearchRadius, int32& TriangleID, FVector& TargetPosOut, FVector& TargetNormalOut);
};

