// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "FbxImporter.h"
#include <fbxsdk.h>
#include "DynamicMesh/DynamicMesh3.h"

using namespace UE::Geometry;

UENUM(BlueprintType)
enum class EMeshMorpherFBXCoordinate : uint8
{
	RightHanded   UMETA(DisplayName = "RightHanded"),
	LeftHanded 	    UMETA(DisplayName = "LeftHanded"),
};


UENUM(BlueprintType)
enum class EMeshMorpherFBXAxis : uint8
{
	X   UMETA(DisplayName = "X"),
	Y 	UMETA(DisplayName = "Y"),
	Z 	UMETA(DisplayName = "Z"),

};


class FMeshMorpherFBXImport
{

public:
	FMeshMorpherFBXImport(const FString FileName, EMeshMorpherFBXCoordinate Coordinate, EMeshMorpherFBXAxis FrontVector, EMeshMorpherFBXAxis UpVector, const bool bConvertScene, const bool UseT0, const int32 ImportID);
	~FMeshMorpherFBXImport();
public:
	FDynamicMesh3 DynamicMesh;
private:
	static FbxAMatrix CalculateGlobalTransform(FbxNode* pNode, const FbxAMatrix ParentGX);
	void CalculateGlobalTransformRecursive(FbxNode* pNode, const FbxAMatrix ParentGX);
	void ConvertScene();
	void FillFbxMeshArray(FbxNode* pNode);
	bool FillCollisionModelList(FbxNode* Node);
	static void FillLODGroupArray(FbxNode* Node, TArray<FbxNode*>& outLODGroupArray);
	void FindAllLODGroupNode(FbxNode* Node, const int32 LODIndex, TMap<FbxUInt64, FbxNode*>& outLODGroupArray);
	void InitializeFBXMesh(const FString& FileName, int32 ImportID);
	FbxAMatrix ComputeTotalMatrix(FbxNode* Node) const;
	void GetVertexArray(FbxMesh* FbxMesh, TArray<FbxVector4>& VertexArray) const;
	void CreateSingleFBXMesh(const int32 i);
private:
	FbxMap<FbxString, TSharedPtr< FbxArray<FbxNode* > > > CollisionModels;
	bool bCurrentConvertScene = false;
	bool bUseT0 = false;
	FVector CurrentFBXAxis;
	
	TArray<FbxNode*> outMeshArray;
	
	FbxManager* SdkManager;
	FbxScene* Scene;


	FbxAMatrix AxisConversionMatrix;

	TMap<FbxNode*, FbxAMatrix> GSMatrixMap;
	FbxAMatrix TotalMatrix;


};
