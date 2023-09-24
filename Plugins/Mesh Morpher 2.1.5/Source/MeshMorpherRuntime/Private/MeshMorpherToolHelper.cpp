// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherToolHelper.h"
#include "FrameTypes.h"
#include "Components/MeshMorpherMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/FeedbackContext.h"
#include "Generators/SphereGenerator.h"
#include "MeshOperationsLibraryRT.h"

#define LOCTEXT_NAMESPACE "MeshMorpherToolHelper"

double UMeshMorpherToolHelper::CalculateBrushFalloff(double Distance, double BrushSize, double FalloffAmount)
{
	const double f = FMath::Clamp(1.0 - FalloffAmount, 0.0, 1.0);
	double d = Distance / BrushSize;
	double w = 1.0;
	if (d > f)
	{
		d = FMath::Clamp((d - f) / (1.0 - f), 0.0, 1.0);
		w = (1.0 - d * d);
		w = w * w * w;
	}
	return w;
}

void UMeshMorpherToolHelper::GetBrushPositionOnPlane(const FVector& RayOrigin, const FVector& RayDirection, const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector SymmetryAxis, FBrushPosition& BrushPosition, FBrushPosition& SymmetricBrushPosition)
{
	FVector3d NewHitPosWorld;
	UE::Geometry::FFrame3d Plane(PlaneOrigin, PlaneNormal);
	Plane.RayPlaneIntersection(RayOrigin, RayDirection, 2, NewHitPosWorld);

	BrushPosition = FBrushPosition(NewHitPosWorld,Plane.Z());

	FVector SymLocation = ((FVector)NewHitPosWorld).RotateAngleAxis(180, SymmetryAxis);
	SymLocation = FVector(SymLocation.X, SymLocation.Y, NewHitPosWorld.Z);

	SymmetricBrushPosition = FBrushPosition(SymLocation, Plane.Z());
}

bool UMeshMorpherToolHelper::FindTriangleIntersectionPoint(const FVector& RayOrigin, const FVector& RayDirection, const FVector& A, const FVector& B, const FVector& C, double& Distance)
{
	double ZeroTolerance = 1e-06;

	// Compute the offset origin, edges, and normal.
	FVector diff = RayOrigin - A;
	FVector edge1 = B - A;
	FVector edge2 = C - A;
	FVector normal = FVector::CrossProduct(edge1, edge2);

	// Solve Q + t*D = b1*E1 + b2*E2 (Q = kDiff, D = ray direction,
	// E1 = kEdge1, E2 = kEdge2, N = Cross(E1,E2)) by
	//   |Dot(D,N)|*b1 = sign(Dot(D,N))*Dot(D,Cross(Q,E2))
	//   |Dot(D,N)|*b2 = sign(Dot(D,N))*Dot(D,Cross(E1,Q))
	//   |Dot(D,N)|*t = -sign(Dot(D,N))*Dot(Q,N)
	double DdN = FVector::DotProduct(RayDirection, normal);
	double sign;

	
	if (DdN > ZeroTolerance)
	{
		sign = 1.0;
	}
	else if (DdN < -ZeroTolerance)
	{
		sign = -1.0;
		DdN = -DdN;
	}
	else
	{
		return false;
	}

	double DdQxE2 = sign * FVector::DotProduct(RayDirection, FVector::CrossProduct(diff, edge2));
	if (DdQxE2 >= 0.0)
	{
		double DdE1xQ = sign * FVector::DotProduct(RayDirection, FVector::CrossProduct(edge1, diff));
		if (DdE1xQ >= 0.0)
		{
			if (DdQxE2 + DdE1xQ <= DdN)
			{
				// Line intersects triangle, check if ray does.
				double QdN = -sign * FVector::DotProduct(diff, normal);
				if (QdN >= 0.0)
				{
					// Ray intersects triangle.
					double inv = 1.0 / DdN;
					Distance = QdN * inv;
					return true;
				}
				// else: t < 0, no intersection
			}
			// else: b1+b2 > 1, no intersection
		}
		// else: b2 < 0, no intersection
	}
	// else: b1 < 0, no intersection

	return false;
}

void UMeshMorpherToolHelper::FindBestAxisVectors(const FVector& Normal, FVector& Axis1, FVector& Axis2)
{
	Normal.FindBestAxisVectors(Axis1, Axis2);
}

FVector UMeshMorpherToolHelper::ProjectOnPlane(const FVector& PlaneOrigin, const FVector& PlaneNormal, const FVector& Position, int32 Axis)
{
	UE::Geometry::FFrame3d Plane(PlaneOrigin, PlaneNormal);
	return Plane.ToPlane(Position, Axis);
}

void UMeshMorpherToolHelper::GetPlaneX(const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& Out)
{
	UE::Geometry::FFrame3d Plane(PlaneOrigin, PlaneNormal);
	Out = Plane.X();
}

void UMeshMorpherToolHelper::GetPlaneY(const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& Out)
{
	UE::Geometry::FFrame3d Plane(PlaneOrigin, PlaneNormal);
	Out = Plane.Y();
}

void UMeshMorpherToolHelper::GetPlaneZ(const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& Out)
{
	UE::Geometry::FFrame3d Plane(PlaneOrigin, PlaneNormal);
	Out = Plane.Z();
}

double UMeshMorpherToolHelper::GetNormalizedLength(const FVector& Vector)
{
	double length = Vector.Length();
	if (length > 0.0)
	{
		double invLength = 1.0 / length;
		return length;
	}
	return 0.0;
}

void UMeshMorpherToolHelper::LerpVector(const FVector& A, const FVector& B, double Alpha, FVector& ReturnValue)
{
	double OneMinusAlpha = 1.0 - Alpha;
	ReturnValue = FVector(OneMinusAlpha * A.X + Alpha * B.X, OneMinusAlpha * A.Y + Alpha * B.Y, OneMinusAlpha * A.Z + Alpha * B.Z);
}

bool UMeshMorpherToolHelper::BrushPositionEqualsBrushPosition(const FBrushPosition& A, const FBrushPosition& B)
{
	return A.Location.Equals(B.Location) && A.Normal.Equals(B.Normal);
}

void UMeshMorpherToolHelper::ExportMeshComponentToOBJFile(UMeshMorpherMeshComponent* MeshComponent, const TSet<int32>& SelectedTriangles, FString FilePath, bool bInvert, const TArray<FSkeletalMaterial>& Materials)
{
	if (MeshComponent)
	{
		ExportMeshToOBJFile(MeshComponent->GetMesh(), SelectedTriangles, FilePath, bInvert, Materials);
	}
}

void UMeshMorpherToolHelper::ExportMeshToOBJFile(FDynamicMesh3* Mesh, const TSet<int32>& SelectedTriangles, FString FilePath, bool bInvert, const TArray<FSkeletalMaterial>& Materials)
{
	if (Mesh)
	{
		if (FilePath.Len() > 0)
		{

			bool bHasMaterial = false;
			FDynamicMeshMaterialAttribute* MaterialID = NULL;
			const bool bHasAttributes = Mesh->HasAttributes();

			if (bHasAttributes)
			{
				bHasMaterial = Mesh->Attributes()->HasMaterialID();
			}

			if (bHasMaterial)
			{
				MaterialID = Mesh->Attributes()->GetMaterialID();
			}

			FDynamicMeshUVOverlay* UVOverlay = nullptr;
			FDynamicMeshNormalOverlay* NormalOverlay = nullptr;
			if (bHasAttributes)
			{
				UVOverlay = Mesh->Attributes()->PrimaryUV();
				NormalOverlay = Mesh->Attributes()->PrimaryNormals();
			}


			FFormatNamedArguments Args;
			Args.Add(TEXT("OBJFilename"), FText::FromString(FilePath));
			const FText StatusUpdate = FText::Format(LOCTEXT("SaveOBJ", "Exporting selection to OBJ File...({OBJFilename})"), Args);
			GWarn->BeginSlowTask(StatusUpdate, true);
			if (!GWarn->GetScopeStack().IsEmpty())
			{
				GWarn->GetScopeStack().Last()->MakeDialog(false, true);
			}

			TArray<FString> FileString;
			FText OBJUpdate;

			OBJUpdate = FText::FromString("Writing header data...");
			GWarn->StatusForceUpdate(0, 4, OBJUpdate);
			const FString FirstLine = "# Mesh Morpher OBJ exporter.";
			FileString.Add(FirstLine);
			FileString.Add(FString("# X Forward Y UP"));


			TArray<FString> StringVertices;
			TArray<FString> StringNormals;
			TArray<FString> StringUVs;
			TMap<int32, int32> VerticeMap;
			TMap<int32, TArray<FString>> TriangleMapping;

			OBJUpdate = FText::FromString("Preparing Triangle data...");
			GWarn->StatusForceUpdate(1, 4, OBJUpdate);


			auto AddVertex = [&](const uint32& VertIdx, const uint32& NormalIdx, const uint32& UVIdx)->int32
			{
				if (!VerticeMap.Contains(VertIdx))
				{
					FVector Normal = (NormalOverlay != nullptr && NormalIdx != FDynamicMesh3::InvalidID) ? FVector(NormalOverlay->GetElement(NormalIdx)) : FMeshNormals::ComputeVertexNormal(*Mesh, VertIdx);

					const FVector Vertex = Mesh->GetVertex(VertIdx);
					const int32 NewVertexID = StringVertices.Add(FString::Printf(TEXT("v %lf %lf %lf"), Vertex.X, Vertex.Z, Vertex.Y));
					StringNormals.Add(FString::Printf(TEXT("vn %lf %lf %lf"), Normal.X, Normal.Z, Normal.Y));
					if ((UVOverlay != nullptr && UVIdx != FDynamicMesh3::InvalidID))
					{
						FVector2f UV = UVOverlay->GetElement(UVIdx);
						StringUVs.Add(FString::Printf(TEXT("vt %lf %lf"), UV.X, 1.0 - UV.Y));
					}
					VerticeMap.Add(VertIdx, NewVertexID);
					return NewVertexID;
				}
				else {

					return VerticeMap[VertIdx];
				}
			};

			auto GetTriGroup = [&](const uint32& TriIdx)->int32
			{
				int32 TriangleGroup = 0;
				if (bHasMaterial && MaterialID)
				{
					MaterialID->GetValue(TriIdx, &TriangleGroup);
				}
				return TriangleGroup;
			};

			// "f v1//vn1 v2//vn2 v3//vn3"
			// "f v1/vt1 v2/vt2 v3/vt3"
			// "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3"

			if (SelectedTriangles.Num() <= 0)
			{
				for (const int32& Tri : Mesh->TriangleIndicesItr())
				{
					int32 TriangleGroup = GetTriGroup(Tri);

					const FIndex3i Indices = Mesh->GetTriangle(Tri);
					FIndex3i NewIndices;

					const FIndex3i TriUV = (UVOverlay != nullptr) ? UVOverlay->GetTriangle(Tri) : FIndex3i(FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID);
					const FIndex3i TriNormal = (NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(Tri) : FIndex3i(FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID);

					for (int32 Corner = 0; Corner < 3; ++Corner)
					{
						NewIndices[Corner] = AddVertex((uint32)Indices[Corner], TriNormal[Corner], TriUV[Corner]);
					}

					TArray<FString>& TriangleList = TriangleMapping.FindOrAdd(TriangleGroup);

					int32 IDx0 = NewIndices.A + 1;
					int32 IDx1 = NewIndices.B + 1;
					int32 IDx2 = NewIndices.C + 1;

					if (UVOverlay != nullptr)
					{
						TriangleList.Add(FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d"), IDx0, IDx0, IDx0, IDx1, IDx1, IDx1, IDx2, IDx2, IDx2));
					}
					else {
						TriangleList.Add(FString::Printf(TEXT("f %d//%d %d//%d %d//%d"), IDx0, IDx0, IDx1, IDx1, IDx2, IDx2));
					}
				}
			}
			else {

				TSet<int32> SelectedTrianglesSorted = SelectedTriangles;
				SelectedTrianglesSorted.Sort([](const int32& A, const int32& B)
				{
					return A < B;
				});
				
				if (!bInvert)
				{
					for (const int32& Tri : SelectedTrianglesSorted)
					{
						int32 TriangleGroup = GetTriGroup(Tri);
						const FIndex3i Indices = Mesh->GetTriangle(Tri);
						FIndex3i NewIndices;

						const FIndex3i TriUV = (UVOverlay != nullptr) ? UVOverlay->GetTriangle(Tri) : FIndex3i(FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID);
						const FIndex3i TriNormal = (NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(Tri) : FIndex3i(FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID);

						for (int32 Corner = 0; Corner < 3; ++Corner)
						{
							NewIndices[Corner] = AddVertex((uint32)Indices[Corner], TriNormal[Corner], TriUV[Corner]);
						}

						TArray<FString>& TriangleList = TriangleMapping.FindOrAdd(TriangleGroup);

						int32 IDx0 = NewIndices.A + 1;
						int32 IDx1 = NewIndices.B + 1;
						int32 IDx2 = NewIndices.C + 1;

						if (UVOverlay != nullptr)
						{
							TriangleList.Add(FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d"), IDx0, IDx0, IDx0, IDx1, IDx1, IDx1, IDx2, IDx2, IDx2));
						}
						else {
							TriangleList.Add(FString::Printf(TEXT("f %d//%d %d//%d %d//%d"), IDx0, IDx0, IDx1, IDx1, IDx2, IDx2));
						}
					}
				}
				else {
					for (const int32& Tri : Mesh->TriangleIndicesItr())
					{
						if (!SelectedTrianglesSorted.Contains(Tri))
						{
							int32 TriangleGroup = GetTriGroup(Tri);

							const FIndex3i Indices = Mesh->GetTriangle(Tri);
							FIndex3i NewIndices;

							const FIndex3i TriUV = (UVOverlay != nullptr) ? UVOverlay->GetTriangle(Tri) : FIndex3i(FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID);
							const FIndex3i TriNormal = (NormalOverlay != nullptr) ? NormalOverlay->GetTriangle(Tri) : FIndex3i(FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID, FDynamicMesh3::InvalidID);

							for (int32 Corner = 0; Corner < 3; ++Corner)
							{
								NewIndices[Corner] = AddVertex((uint32)Indices[Corner], TriNormal[Corner], TriUV[Corner]);
							}

							TArray<FString>& TriangleList = TriangleMapping.FindOrAdd(TriangleGroup);

							int32 IDx0 = NewIndices.A + 1;
							int32 IDx1 = NewIndices.B + 1;
							int32 IDx2 = NewIndices.C + 1;

							if (UVOverlay != nullptr)
							{
								TriangleList.Add(FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d"), IDx0, IDx0, IDx0, IDx1, IDx1, IDx1, IDx2, IDx2, IDx2));
							}
							else {
								TriangleList.Add(FString::Printf(TEXT("f %d//%d %d//%d %d//%d"), IDx0, IDx0, IDx1, IDx1, IDx2, IDx2));
							}
						}
					}
				}
			}

			OBJUpdate = FText::FromString("Sorting Triangle data...");
			GWarn->StatusForceUpdate(2, 4, OBJUpdate);
			TriangleMapping.KeySort([](const int32& A, const int32& B)
				{
					return A < B;
				});


			FileString.Append(StringVertices);
			FileString.Append(StringNormals);
			FileString.Append(StringUVs);

			OBJUpdate = FText::FromString("Writing Triangle data...");
			GWarn->StatusForceUpdate(3, 4, OBJUpdate);
			for (auto& TriObj : TriangleMapping)
			{
				if(Materials.IsValidIndex(TriObj.Key) && !Materials[TriObj.Key].MaterialSlotName.IsNone())
				{
					FileString.Add(FString::Printf(TEXT("o %s"), *Materials[TriObj.Key].MaterialSlotName.ToString()));
				} else
				{
					FileString.Add(FString::Printf(TEXT("o %d"), TriObj.Key));
				}
				
				FileString.Append(TriObj.Value);
			}

			OBJUpdate = FText::FromString("Writing File...");
			GWarn->StatusForceUpdate(4, 4, OBJUpdate);
			const bool bWriteResult = FFileHelper::SaveStringArrayToFile(FileString, *FilePath);

			GWarn->EndSlowTask();
			
		}

	}
}

void UMeshMorpherToolHelper::GetBoundaryVertices(UMeshMorpherMeshComponent* MeshComponent, TSet<int32>& Vertices)
{
	Vertices.Empty();
	if (MeshComponent)
	{
		FDynamicMesh3* Mesh = MeshComponent->GetMesh();
		if (Mesh)
		{
			for (int32 EdgeIdx : Mesh->BoundaryEdgeIndicesItr())
			{
				FDynamicMesh3::FEdge Edge = Mesh->GetEdge(EdgeIdx);

				for (int32 Idx = 0; Idx < 2; ++Idx)
				{
					if (Mesh->IsBoundaryVertex(Edge.Vert[Idx]))
					{
						Vertices.Add(Edge.Vert[Idx]);
					}
				}
			}
		}
	}
}

bool UMeshMorpherToolHelper::MakeSphereBrushGizmo(UObject* WorldContextObject, UMaterialInterface* Material, UIndicatorMesh*& Gizmo, int32 Resolution)
{
	if(WorldContextObject && WorldContextObject->GetWorld())
	{
		Gizmo = NewObject<UIndicatorMesh>(WorldContextObject);
		Gizmo->CreateInWorld(WorldContextObject->GetWorld(), FTransform::Identity);
		FSphereGenerator SphereGen;
		SphereGen.NumPhi = SphereGen.NumTheta = Resolution;
		SphereGen.Generate();
		FDynamicMesh3 Mesh(&SphereGen);
		Gizmo->UpdatePreview(&Mesh);

		if (Material)
		{
			Gizmo->SetMaterial(Material);
		}
		return true;
	}
	return false;
}

void UMeshMorpherToolHelper::DestroyBrushGizmo(UIndicatorMesh* Gizmo)
{
	if (Gizmo)
	{
		Gizmo->Disconnect();
	}
}

void UIndicatorMesh::Update(float Size, const FVector& Position, const FVector& Normal)
{
	FTransform Transform = this->GetTransform();

	Transform.SetTranslation(Position);

	const FQuat CurRotation = Transform.GetRotation();
	const FQuat ApplyRotation = FQuat::FindBetween(CurRotation.GetAxisZ(), Normal);
	Transform.SetRotation(ApplyRotation * CurRotation);

	Transform.SetScale3D(Size * FVector::OneVector);
	if (!Transform.ContainsNaN())
	{
		SetTransform(Transform);
	}
}

void UIndicatorMesh::UpdateMaterial(UMaterialInterface* Material)
{
	if (DynamicMeshComponent)
	{
		SetMaterial(Material);
	}
}

void UIndicatorMesh::UpdateVisibility(const bool Visibility)
{
	if (DynamicMeshComponent)
	{
		DynamicMeshComponent->SetVisibility(Visibility);
	}
}


void UIndicatorMesh::BeginDestroy()
{
	Disconnect();
	Super::BeginDestroy();
}

#undef LOCTEXT_NAMESPACE