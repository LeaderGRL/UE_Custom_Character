// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Animation/MorphTarget.h"

using namespace UE::Geometry;

struct FMeshMorpherWrapPair
{
	int32 SourceIndex = INDEX_NONE;
	int32 TargetIndex = INDEX_NONE;
	FMeshMorpherWrapPair()
	{
		
	}

	FMeshMorpherWrapPair(const int32& InTargetIndex, const int32& InSourceIndex)
	{
		TargetIndex = InTargetIndex;
		SourceIndex = InSourceIndex;
	}
	
};

class FMeshMorpherWrapper
{

public:
	FDynamicMesh3 SourceMesh;
	FDynamicMesh3 TargetMesh;
	double VertexThreshold = 20.0;
	double NormalIncompatibilityThreshold = 0.5;
public:
	static bool IsDynamicMeshIdentical(const FDynamicMesh3& DynamicMeshA, const FDynamicMesh3& DynamicMeshB);
	bool ProjectDeltas(const TArray<FMorphTargetDelta>& InDeltas, const bool bCheckMeshesIdentical, const double Multiplier, const int32 SmoothIterations, const double SmoothStrength, TArray<FMorphTargetDelta>& OutDeltas) const;
	bool ProjectMesh(const bool bCheckMeshesIdentical, const double Multiplier, const int32 SmoothIterations, const double SmoothStrength, TArray<FMorphTargetDelta>& OutDeltas) const;
private:
	void ComputeDeltaProjection(const FDynamicMesh3& SourceDynamicMesh, FDynamicMesh3 TargetDynamicMesh, const TArray<FMorphTargetDelta>& InDeltas, const double Multiplier, const int32 SmoothIterations, const double SmoothStrength, TArray<FMorphTargetDelta>& OutDeltas) const;
	void GetSmoothDeltas(const FDynamicMesh3& TargetDynamicMesh, TArray<FMorphTargetDelta>& Deltas, const double& SmoothStrength, TArray<FMorphTargetDelta>& OutSmoothDeltas) const;
	void ApplyDeltasToIdenticalVertices(const TArray<TSet<int32>>& VerticesSets, TArray<FMorphTargetDelta>& Deltas) const;
	void CreateDeltasForVertexPairs(const TArray<FMeshMorpherWrapPair>& VertexPairs, const TArray<FMorphTargetDelta>& BaseDeltas, TArray<FMorphTargetDelta>& OutDeltas) const;
	void CalculateDynamicMeshesForTransfer(const FDynamicMesh3& SourceDynamicMesh, const FDynamicMesh3& TargetDynamicMesh, TArray<TSet<int32>>& VerticesSets, TArray<FMeshMorpherWrapPair>& VertexPairs, TArray<int32>& NoCorrespondent) const;
};
