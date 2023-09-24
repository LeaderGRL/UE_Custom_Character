// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherFBXImport.h"
#include "DynamicMesh/MeshNormals.h"
#include "Misc/FileHelper.h"

// Get the geometry deformation local to a node. It is never inherited by the
// children.
FbxAMatrix GetGeometry(FbxNode* pNode) 
{
	FbxVector4 lT, lR, lS;
	FbxAMatrix lGeometry;

	lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	lGeometry.SetT(lT);
	lGeometry.SetR(lR);
	lGeometry.SetS(lS);

	return lGeometry;
}

// Scale all the elements of a matrix.
void MatrixScale(FbxAMatrix& pMatrix, double pValue)
{
	int32 i,j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pMatrix[i][j] *= pValue;
		}
	}
}

// Add a value to all the elements in the diagonal of the matrix.
void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue)
{
	pMatrix[0][0] += pValue;
	pMatrix[1][1] += pValue;
	pMatrix[2][2] += pValue;
	pMatrix[3][3] += pValue;
}

// Sum two matrices element by element.
void MatrixAdd(FbxAMatrix& pDstMatrix, FbxAMatrix& pSrcMatrix)
{
	int32 i,j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pDstMatrix[i][j] += pSrcMatrix[i][j];
		}
	}
}

FVector ConvertPos(FbxVector4 Vector)
{
	FVector Out;
	Out[0] = Vector[0];
	Out[1] = -Vector[1];
	Out[2] = Vector[2];
	return Out;
}

FVector ConvertDir(FbxVector4 Vector)
{
	FVector Out;
	Out[0] = Vector[0];
	Out[1] = -Vector[1];
	Out[2] = Vector[2];
	return Out;
}

bool IsOddNegativeScale(const FbxAMatrix& TotalMatrix)
{
	FbxVector4 Scale = TotalMatrix.GetS();
	int32 NegativeNum = 0;

	if (Scale[0] < 0) NegativeNum++;
	if (Scale[1] < 0) NegativeNum++;
	if (Scale[2] < 0) NegativeNum++;

	return NegativeNum == 1 || NegativeNum == 3;
}

FMeshMorpherFBXImport::FMeshMorpherFBXImport(const FString FileName, EMeshMorpherFBXCoordinate Coordinate, EMeshMorpherFBXAxis FrontVector, EMeshMorpherFBXAxis UpVector, const bool bConvertScene, const bool UseT0, const int32 ImportID)
{
	DynamicMesh.Clear();
	DynamicMesh.EnableAttributes();
	DynamicMesh.EnableTriangleGroups(0);
	outMeshArray.Empty();
	
	CurrentFBXAxis = FVector(static_cast<uint8>(Coordinate), static_cast<uint8>(FrontVector), static_cast<uint8>(UpVector));
	bCurrentConvertScene = bConvertScene;
	bUseT0 = UseT0;
	SdkManager = FbxManager::GetDefaultManager();

	if (!SdkManager)
	{
		SdkManager = FbxManager::Create();

	}

	
	const std::string Name(TCHAR_TO_UTF8(*("MeshMorpherScene" + FString::FromInt(ImportID))));
	const char* fn = Name.c_str();

	Scene = FbxScene::Create(SdkManager, fn);

	InitializeFBXMesh(FileName, ImportID);

	for (int i = 0; i < outMeshArray.Num(); i++)
	{
		if(outMeshArray[i])
		{
			FString NodeName = outMeshArray[i]->GetName();
			const bool bIsCollisionMesh = NodeName.StartsWith("UCX_");
			if(!bIsCollisionMesh)
			{
				CreateSingleFBXMesh(i);
			}
		}
	}

	if (FDynamicMeshNormalOverlay* NormalOverlay = DynamicMesh.Attributes()->PrimaryNormals())
	{
		FMeshNormals::InitializeOverlayToPerVertexNormals(NormalOverlay);
	}
}

FMeshMorpherFBXImport::~FMeshMorpherFBXImport()
{
	if (Scene)
	{
		FBX_SAFE_DESTROY(Scene);
	}

	if (SdkManager)
	{
		FBX_SAFE_DESTROY(SdkManager);
	}
}

FbxAMatrix FMeshMorpherFBXImport::CalculateGlobalTransform(FbxNode* pNode, const FbxAMatrix ParentGX)
{
	FbxAMatrix TranlationM, ScalingM, ScalingPivotM, ScalingOffsetM, RotationOffsetM, RotationPivotM, PreRotationM, RotationM, PostRotationM;
	FbxAMatrix GlobalT, GlobalRS;
	FbxAMatrix Transform;
	if (!pNode)
	{

		Transform.SetIdentity();
		return Transform;
	}
	
	TranlationM.SetT(FbxVector4(pNode->LclTranslation.Get()));
	RotationM.SetR(FbxVector4(pNode->LclRotation.Get()));
	PreRotationM.SetR(FbxVector4(pNode->PreRotation.Get()));
	PostRotationM.SetR(FbxVector4(pNode->PostRotation.Get()));
	ScalingM.SetS(FbxVector4(pNode->LclScaling.Get()));
	ScalingOffsetM.SetT(FbxVector4(pNode->ScalingOffset.Get()));
	ScalingPivotM.SetT(FbxVector4(pNode->ScalingPivot.Get()));
	RotationOffsetM.SetT(FbxVector4(pNode->RotationOffset.Get()));
	RotationPivotM.SetT(FbxVector4(pNode->RotationPivot.Get()));

	const FbxNode* ParentNode = pNode->GetParent();

	//Construct Global Rotation
	FbxAMatrix ParentGRM;
	ParentGRM.SetR(FbxVector4(ParentGX.GetR()));
	const FbxAMatrix LRM = PreRotationM * RotationM * PostRotationM;

	FbxAMatrix ParentTM;
	ParentTM.SetT(FbxVector4(ParentGX.GetT()));
	const FbxAMatrix ParentGRSM = ParentTM.Inverse() * ParentGX;
	const FbxAMatrix ParentGSM = ParentGRM.Inverse() * ParentGRSM;
	const FbxAMatrix LSM = ScalingM;

	const FbxTransform::EInheritType InheritType = pNode->InheritType.Get();
	if (InheritType == FbxTransform::eInheritRrSs)
	{
		GlobalRS = ParentGRM * LRM * ParentGSM * LSM;
	}
	else if (InheritType == FbxTransform::eInheritRSrs)
	{
		GlobalRS = ParentGRM * ParentGSM * LRM * LSM;
	}
	else if (InheritType == FbxTransform::eInheritRrs)
	{
		FbxAMatrix ParentLSM;

		if (ParentNode && ParentNode->LclScaling.IsValid())
		{
			ParentLSM.SetS(FbxVector4(ParentNode->LclScaling.Get()));
		}

		const FbxAMatrix lParentGSM_noLocal = ParentGSM * ParentLSM.Inverse();
		GlobalRS = ParentGRM * LRM * lParentGSM_noLocal * LSM;
	}

	Transform = TranlationM * RotationOffsetM * RotationPivotM * PreRotationM * RotationM * PostRotationM * RotationPivotM.Inverse() * ScalingOffsetM * ScalingPivotM * ScalingM * ScalingPivotM.Inverse();
	GlobalT.SetT(Transform.GetT());

	Transform = GlobalT * GlobalRS;

	return Transform;
}

void FMeshMorpherFBXImport::CalculateGlobalTransformRecursive(FbxNode* pNode, const FbxAMatrix ParentGX)
{
	const FbxAMatrix CurrentGS = CalculateGlobalTransform(pNode, ParentGX);

	if (pNode->GetMesh())
	{
		if (pNode->GetMesh()->GetPolygonVertexCount() > 0)
		{
			GSMatrixMap.Add(pNode, CurrentGS);
		}
	}

	for (int32 ChildIndex = 0; ChildIndex < pNode->GetChildCount(); ChildIndex++)
	{
		
		CalculateGlobalTransformRecursive(pNode->GetChild(ChildIndex), CurrentGS);
	}

}

void FMeshMorpherFBXImport::ConvertScene()
{
	if (Scene)
	{
		const FbxAxisSystem::ECoordSystem CoordSystem = CurrentFBXAxis.X == 0 ? FbxAxisSystem::eRightHanded : FbxAxisSystem::eLeftHanded;

		FbxAxisSystem::EUpVector UpVector = FbxAxisSystem::EUpVector();

		switch (static_cast<int32>(CurrentFBXAxis.Z))
		{
		case 0:
			UpVector = FbxAxisSystem::eXAxis;
			break;
		case 1:
			UpVector = FbxAxisSystem::eYAxis;
			break;

		case 2:
			UpVector = FbxAxisSystem::eZAxis;
			break;
		default: ;
		};

		FbxAxisSystem::EFrontVector FrontVector = FbxAxisSystem::EFrontVector();

		switch (static_cast<int32>(CurrentFBXAxis.Y))
		{
		case 0:
			FrontVector = static_cast<FbxAxisSystem::EFrontVector>(-FbxAxisSystem::eXAxis);
			break;

		case 1:
			FrontVector = static_cast<FbxAxisSystem::EFrontVector>(-FbxAxisSystem::eYAxis);
			break;
		case 2:
			FrontVector = static_cast<FbxAxisSystem::EFrontVector>(-FbxAxisSystem::eZAxis);
			break;
		default: ;
		};

		FbxAxisSystem UnrealImportAxis(UpVector, FrontVector, CoordSystem);

		if (FbxAxisSystem SourceSetup = Scene->GetGlobalSettings().GetAxisSystem(); SourceSetup != UnrealImportAxis)
		{
			FbxRootNodeUtility::RemoveAllFbxRoots(Scene);
			UnrealImportAxis.ConvertScene(Scene);
			
			FbxAMatrix SourceMatrix;
			SourceSetup.GetMatrix(SourceMatrix);
			FbxAMatrix UE4Matrix;
			UnrealImportAxis.GetMatrix(UE4Matrix);
			AxisConversionMatrix = SourceMatrix.Inverse() * UE4Matrix;
			
		}

		Scene->GetAnimationEvaluator()->Reset();
	}
}

void FMeshMorpherFBXImport::FillFbxMeshArray(FbxNode* pNode)
{
	if (pNode->GetMesh() && !FillCollisionModelList(pNode) && pNode->GetMesh()->GetPolygonVertexCount() > 0)
	{
			const FbxVector4 Zero(0, 0, 0);

			pNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
			pNode->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);

			pNode->SetPostRotation(FbxNode::eDestinationPivot, Zero);
			pNode->SetPreRotation(FbxNode::eDestinationPivot, Zero);
			pNode->SetRotationOffset(FbxNode::eDestinationPivot, Zero);
			pNode->SetScalingOffset(FbxNode::eDestinationPivot, Zero);
			pNode->SetRotationPivot(FbxNode::eDestinationPivot, Zero);
			pNode->SetScalingPivot(FbxNode::eDestinationPivot, Zero);

			FbxEuler::EOrder lRotationOrder;
			pNode->GetRotationOrder(FbxNode::eSourcePivot, lRotationOrder);
			pNode->SetRotationOrder(FbxNode::eDestinationPivot, lRotationOrder);

			pNode->SetGeometricTranslation(FbxNode::eDestinationPivot, pNode->GetGeometricTranslation(FbxNode::eSourcePivot));
			pNode->SetGeometricRotation(FbxNode::eDestinationPivot, pNode->GetGeometricRotation(FbxNode::eSourcePivot));
			pNode->SetGeometricScaling(FbxNode::eDestinationPivot, pNode->GetGeometricScaling(FbxNode::eSourcePivot));

			pNode->SetQuaternionInterpolation(FbxNode::eDestinationPivot, pNode->GetQuaternionInterpolation(FbxNode::eSourcePivot));

			pNode->ResetPivotSet(FbxNode::EPivotSet::eDestinationPivot);
			outMeshArray.Add(pNode);
	}

	for (int32 ChildIndex = 0; ChildIndex < pNode->GetChildCount(); ChildIndex++)
	{
		FillFbxMeshArray(pNode->GetChild(ChildIndex));
	}
}

static FbxString GetNodeNameWithoutNamespace( const FbxString& NodeName )
{
	// Namespaces are marked with colons so find the last colon which will mark the start of the actual name
	const int32 LastNamespceIndex = NodeName.ReverseFind(':');

	if( LastNamespceIndex == -1 )
	{
		// No namespace
		return NodeName;
	}
	else
	{
		// chop off the namespace
		return NodeName.Right( NodeName.GetLen() - (LastNamespceIndex + 1) );
	}
}


bool FMeshMorpherFBXImport::FillCollisionModelList(FbxNode* Node)
{
	FbxString NodeName = GetNodeNameWithoutNamespace( Node->GetName() );

	if ( NodeName.Find("UCX") != -1 || NodeName.Find("MCDCX") != -1 ||
		 NodeName.Find("UBX") != -1 || NodeName.Find("USP") != -1 || NodeName.Find("UCP") != -1)
	{
		// Get name of static mesh that the collision model connect to
		uint32 StartIndex = NodeName.Find('_') + 1;
		int32 TmpEndIndex = NodeName.Find('_', StartIndex);
		int32 EndIndex = TmpEndIndex;
		// Find the last '_' (underscore)
		while (TmpEndIndex >= 0)
		{
			EndIndex = TmpEndIndex;
			TmpEndIndex = NodeName.Find('_', EndIndex+1);
		}
		
		const int32 NumMeshNames = 2;
		FbxString MeshName[NumMeshNames];
		if ( EndIndex >= 0 )
		{
			// all characters between the first '_' and the last '_' are the FBX mesh name
			// convert the name to upper because we are case insensitive
			MeshName[0] = NodeName.Mid(StartIndex, EndIndex - StartIndex).Upper();
			
			// also add a version of the mesh name that includes what follows the last '_'
			// in case that's not a suffix but, instead, is part of the mesh name
			if (StartIndex < (int32)NodeName.GetLen())
			{            
				MeshName[1] = NodeName.Mid(StartIndex).Upper();
			}
		}
		else if (StartIndex < (int32)NodeName.GetLen())
		{            
			MeshName[0] = NodeName.Mid(StartIndex).Upper();
		}

		for (int32 NameIdx = 0; NameIdx < NumMeshNames; ++NameIdx)
		{
			if ((int32)MeshName[NameIdx].GetLen() > 0)
			{
				FbxMap<FbxString, TSharedPtr<FbxArray<FbxNode* > > >::RecordType const *Models = CollisionModels.Find(MeshName[NameIdx]);
				TSharedPtr< FbxArray<FbxNode* > > Record;
				if ( !Models )
				{
					Record = MakeShareable( new FbxArray<FbxNode*>() );
					CollisionModels.Insert(MeshName[NameIdx], Record);
				}
				else
				{
					Record = Models->GetValue();
				}

				//Unique add
				Record->AddUnique(Node);
			}
		}

		return true;
	}

	return false;
}

void FMeshMorpherFBXImport::FillLODGroupArray(FbxNode* Node, TArray<FbxNode*>& outLODGroupArray)
{
	// Is this node an LOD group

	if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
	{
		if(Node->GetChildCount() > 0)
		{
			outLODGroupArray.Add(Node->GetChild(0));
			return;
		}
	}

	for (int32 ChildIndex = 0; ChildIndex < Node->GetChildCount(); ++ChildIndex)
	{
		FillLODGroupArray(Node->GetChild(ChildIndex), outLODGroupArray);
	}
}

void FMeshMorpherFBXImport::FindAllLODGroupNode(FbxNode* Node, const int32 LODIndex, TMap<FbxUInt64, FbxNode*>& outLODGroupArray)
{
	if (Node->GetMesh() && !FillCollisionModelList(Node) && Node->GetMesh()->GetPolygonVertexCount() > 0)
	{
		if(!outLODGroupArray.Contains((Node->GetUniqueID())))
		{
			outLODGroupArray.Add(Node->GetUniqueID(), Node);
		}
	}
	
	for (int32 ChildIndex = 0; ChildIndex < Node->GetChildCount(); ++ChildIndex)
	{
		FindAllLODGroupNode(Node->GetChild(ChildIndex), LODIndex, outLODGroupArray);
	}
}

void FMeshMorpherFBXImport::InitializeFBXMesh(const FString& FileName, const int32 ImportID)
{
	const std::string iossettingsname(TCHAR_TO_UTF8(*("IOSROOT" + FString::FromInt(ImportID))));
	const char* iosname = iossettingsname.c_str();

	FbxIOSettings *ios = FbxIOSettings::Create(SdkManager, iosname);
	SdkManager->SetIOSettings(ios);

	const std::string importername(TCHAR_TO_UTF8(*("MeshMorpherImporter" + FString::FromInt(ImportID))));
	const char* iname = importername.c_str();

	FbxImporter* Importer = FbxImporter::Create(SdkManager, iname);

	const std::string Filenamestring(TCHAR_TO_UTF8(*FileName));
	const char* Filename = Filenamestring.c_str();

	
	if (Importer->Initialize(Filename, - 1, ios))
	{
		Importer->Import(Scene);

		Importer->Destroy();
		ios->Destroy();
			
		ConvertScene();

		if (FbxNode* RootNode = Scene->GetRootNode())
		{
			FbxAMatrix lParentGX;
			lParentGX.SetIdentity();

			FbxNode* FoundLOD = nullptr;

			//Maya and Unreal
			{
				TArray<FbxNode*> LODGroupArray;
				for (int NodeIndex = 0; NodeIndex < Scene->GetNodeCount(); ++NodeIndex)
				{
					FbxNode *SceneNode = Scene->GetNode(NodeIndex);
					FillLODGroupArray(SceneNode, LODGroupArray);
					if(LODGroupArray.Num() > 0)
					{
						FoundLOD = LODGroupArray[0];
						const FString MeshName = FString(FoundLOD->GetName());
						//UE_LOG(LogTemp, Warning, TEXT("Property0: %s"), *MeshName);
					}
				}
			}

			//Blender FBX
			if(FoundLOD == nullptr)
			{
				TMap<FbxUInt64, FbxNode*> LODGroupArray;
				for (int NodeIndex = 0; NodeIndex < Scene->GetNodeCount(); ++NodeIndex)
				{
					FbxNode *SceneNode = Scene->GetNode(NodeIndex);
					FindAllLODGroupNode(SceneNode, 0, LODGroupArray);

					int32 MaxCount = 0;
					for (const auto& LocalNode : LODGroupArray)
					{
						const int32 CurrentCount = LocalNode.Value->GetMesh()->GetControlPointsCount();
						if(CurrentCount > MaxCount)
						{
							const FString MeshName = FString(LocalNode.Value->GetName());
							//UE_LOG(LogTemp, Warning, TEXT("Property1: %s"), *MeshName);
							MaxCount = CurrentCount;
							FoundLOD = LocalNode.Value;
						}

					}
				}
			}

			if(FoundLOD)
			{
				const FString MeshName = FString(FoundLOD->GetName());
				//UE_LOG(LogTemp, Warning, TEXT("Final: %s"), *MeshName);
				RootNode = FoundLOD;
			}

			FillFbxMeshArray(RootNode);

			RootNode->ConvertPivotAnimationRecursive(nullptr, FbxNode::EPivotSet::eDestinationPivot, 30);
			CalculateGlobalTransformRecursive(RootNode, lParentGX);
		}
	}
}

FbxAMatrix FMeshMorpherFBXImport::ComputeTotalMatrix(FbxNode* Node) const
{
	FbxAMatrix Geometry;
	const FbxVector4 Translation = Node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 Rotation = Node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 Scaling = Node->GetGeometricScaling(FbxNode::eSourcePivot);
	Geometry.SetT(Translation);
	Geometry.SetR(Rotation);
	Geometry.SetS(Scaling);

	const FbxAMatrix& GlobalTransform = Scene->GetAnimationEvaluator()->GetNodeGlobalTransform(Node);
	
	Geometry.SetIdentity();
	FbxAMatrix tm = GlobalTransform * Geometry;
	return tm;
}

void FMeshMorpherFBXImport::GetVertexArray(FbxMesh* FbxMesh, TArray<FbxVector4>& VertexArray) const
{
	FbxTime poseTime = FBXSDK_TIME_INFINITE;
	if(bUseT0)
	{
		poseTime = 0;
	}

	const int32 VertexCount = FbxMesh->GetControlPointsCount();

	// Create a copy of the vertex array to receive vertex deformations.
	VertexArray.Reset();
	VertexArray.SetNumZeroed(VertexCount);
	FMemory::Memcpy(VertexArray.GetData(), FbxMesh->GetControlPoints(), VertexCount * sizeof(FbxVector4));																	 


	int32 ClusterCount = 0;
	const int32 SkinCount = FbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	for( int32 i=0; i< SkinCount; i++)
	{
		ClusterCount += ((FbxSkin *)(FbxMesh->GetDeformer(i, FbxDeformer::eSkin)))->GetClusterCount();
	}
	
	// Deform the vertex array with the links contained in the mesh.
	if (ClusterCount)
	{
		const FbxAMatrix MeshMatrix = ComputeTotalMatrix(FbxMesh->GetNode());
		// All the links must have the same link mode.
		FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)FbxMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

		int32 i, j;
		int32 lClusterCount=0;

		const int32 lSkinCount = FbxMesh->GetDeformerCount(FbxDeformer::eSkin);

		TArray<FbxAMatrix> ClusterDeformations;
		ClusterDeformations.AddZeroed( VertexCount );

		TArray<double> ClusterWeights;
		ClusterWeights.AddZeroed( VertexCount );
	

		if (lClusterMode == FbxCluster::eAdditive)
		{
			for (i = 0; i < VertexCount; i++)
			{
				ClusterDeformations[i].SetIdentity();
			}
		}

		for ( i=0; i<lSkinCount; ++i)
		{
			lClusterCount =( (FbxSkin *)FbxMesh->GetDeformer(i, FbxDeformer::eSkin))->GetClusterCount();
			for (j=0; j<lClusterCount; ++j)
			{
				FbxCluster* Cluster =((FbxSkin *) FbxMesh->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
				if (!Cluster->GetLink())
					continue;
					
				FbxNode* Link = Cluster->GetLink();
				
				FbxAMatrix lReferenceGlobalInitPosition;
				FbxAMatrix lReferenceGlobalCurrentPosition;
				FbxAMatrix lClusterGlobalInitPosition;
				FbxAMatrix lClusterGlobalCurrentPosition;
				FbxAMatrix lReferenceGeometry;
				FbxAMatrix lClusterGeometry;

				FbxAMatrix lClusterRelativeInitPosition;
				FbxAMatrix lClusterRelativeCurrentPositionInverse;

				if (lClusterMode == FbxCluster::eAdditive && Cluster->GetAssociateModel())
				{
					Cluster->GetTransformAssociateModelMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = Scene->GetAnimationEvaluator()->GetNodeGlobalTransform(Cluster->GetAssociateModel(), poseTime);
					// Geometric transform of the model
					lReferenceGeometry = GetGeometry(Cluster->GetAssociateModel());
					lReferenceGlobalCurrentPosition *= lReferenceGeometry;
				}
				else
				{
					Cluster->GetTransformMatrix(lReferenceGlobalInitPosition);
					lReferenceGlobalCurrentPosition = MeshMatrix; //pGlobalPosition;
					// Multiply lReferenceGlobalInitPosition by Geometric Transformation
					lReferenceGeometry = GetGeometry(FbxMesh->GetNode());
					lReferenceGlobalInitPosition *= lReferenceGeometry;
				}
				// Get the link initial global position and the link current global position.
				Cluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
				lClusterGlobalCurrentPosition = Link->GetScene()->GetAnimationEvaluator()->GetNodeGlobalTransform(Link, poseTime);

				// Compute the initial position of the link relative to the reference.
				lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

				// Compute the current position of the link relative to the reference.
				lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;

				// Compute the shift of the link relative to the reference.
				const FbxAMatrix lVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;

				int32 k;
				const int32 lVertexIndexCount = Cluster->GetControlPointIndicesCount();

				for (k = 0; k < lVertexIndexCount; ++k) 
				{
					int32 lIndex = Cluster->GetControlPointIndices()[k];

					// Sometimes, the mesh can have less points than at the time of the skinning
					// because a smooth operator was active when skinning but has been deactivated during export.
					if (lIndex >= VertexCount)
					{
						continue;
					}

					const double lWeight = Cluster->GetControlPointWeights()[k];

					if (lWeight == 0.0)
					{
						continue;
					}

					// Compute the influence of the link on the vertex.
					FbxAMatrix lInfluence = lVertexTransformMatrix;
					MatrixScale(lInfluence, lWeight);

					if (lClusterMode == FbxCluster::eAdditive)
					{
						// Multiply with to the product of the deformations on the vertex.
						MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
						ClusterDeformations[lIndex] = lInfluence * ClusterDeformations[lIndex];

						// Set the link to 1.0 just to know this vertex is influenced by a link.
						ClusterWeights[lIndex] = 1.0;
					}
					else // lLinkMode == KFbxLink::eNORMALIZE || lLinkMode == KFbxLink::eTOTAL1
						{
						// Add to the sum of the deformations on the vertex.
						MatrixAdd(ClusterDeformations[lIndex], lInfluence);

						// Add to the sum of weights to either normalize or complete the vertex.
						ClusterWeights[lIndex] += lWeight;
						}

				}
			}
		}
		
		for (i = 0; i < VertexCount; i++) 
		{
			FbxVector4 lSrcVertex = VertexArray[i];
			FbxVector4& lDstVertex = VertexArray[i];
			const double lWeight = ClusterWeights[i];

			// Deform the vertex if there was at least a link with an influence on the vertex,
			if (lWeight != 0.0) 
			{
				lDstVertex = ClusterDeformations[i].MultT(lSrcVertex);

				if (lClusterMode == FbxCluster::eNormalize)
				{
					// In the normalized link mode, a vertex is always totally influenced by the links. 
					lDstVertex /= lWeight;
				}
				else if (lClusterMode == FbxCluster::eTotalOne)
				{
					// In the total 1 link mode, a vertex can be partially influenced by the links. 
					lSrcVertex *= (1.0 - lWeight);
					lDstVertex += lSrcVertex;
				}
			} 
		}
	}
}


void FMeshMorpherFBXImport::CreateSingleFBXMesh(const int32 i)
{
	if(outMeshArray.IsValidIndex((i)))
	{
		if (FbxNode* Node = outMeshArray[i])
		{
			FbxMesh* Mesh = Node->GetMesh();

			if (Mesh != nullptr)
			{
				if(bCurrentConvertScene)
				{
					TotalMatrix = ComputeTotalMatrix(Node);
				}
				else
				{
					if (GSMatrixMap.Contains(Node))
					{
						TotalMatrix = GSMatrixMap[Node];
					}
				}
		
				Mesh->RemoveBadPolygons();

				
				TArray<FbxVector4> VertexArray;
				GetVertexArray(Mesh, VertexArray);				

				TMap<int32, int32> IndexMap;
				const int32 TriangleCount = Mesh->GetPolygonCount();
				for (int32 TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
				{
					const int32 polyVertCount = Mesh->GetPolygonSize(TriangleIndex);
					if(polyVertCount == 3)
					{
						FIndex3i Triangle;
						for (int32 CornerIndex = 0; CornerIndex < polyVertCount; CornerIndex++)
						{
							const int32 ControlPointIndex = Mesh->GetPolygonVertex(TriangleIndex, CornerIndex);

							if (const int32* ExistingIndex = IndexMap.Find(ControlPointIndex))
							{
								Triangle[CornerIndex] = *ExistingIndex;
							}
							else
							{
								FbxVector4 FbxPosition = VertexArray[ControlPointIndex];
								const FbxVector4 FinalPosition = TotalMatrix.MultT(FbxPosition);

								FVector VFinalPosition = ConvertPos(FinalPosition);
								Triangle[CornerIndex] = DynamicMesh.AppendVertex(VFinalPosition);
								IndexMap.Add(ControlPointIndex, Triangle[CornerIndex]);
							}
						}
						DynamicMesh.AppendTriangle(Triangle, i);
					}
					else if(polyVertCount == 4)
					{
						FIntVector4 Polygon;
						for (int32 CornerIndex = 0; CornerIndex < polyVertCount; CornerIndex++)
						{
							const int32 ControlPointIndex = Mesh->GetPolygonVertex(TriangleIndex, CornerIndex);

							if (const int32* ExistingIndex = IndexMap.Find(ControlPointIndex))
							{
								Polygon[CornerIndex] = *ExistingIndex;
							}
							else
							{
								FbxVector4 FbxPosition = VertexArray[ControlPointIndex];
								const FbxVector4 FinalPosition = TotalMatrix.MultT(FbxPosition);

								FVector VFinalPosition = ConvertPos(FinalPosition);
								Polygon[CornerIndex] = DynamicMesh.AppendVertex(VFinalPosition);
								IndexMap.Add(ControlPointIndex, Polygon[CornerIndex]);
							}
						}
						FIndex3i Triangle1 = FIndex3i(Polygon[0], Polygon[1], Polygon[2]);
						FIndex3i Triangle2 = FIndex3i(Polygon[0], Polygon[2], Polygon[3]);
						DynamicMesh.AppendTriangle(Triangle1, i);
						DynamicMesh.AppendTriangle(Triangle2, i);
					}
				}
			}
		}
	}
}