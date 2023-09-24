// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherWrapper.h"
#include "DynamicMesh/MeshNormals.h"
#include "MeshOperationsLibraryRT.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"

bool FMeshMorpherWrapper::IsDynamicMeshIdentical(const FDynamicMesh3& DynamicMeshA, const FDynamicMesh3& DynamicMeshB)
{
	const int32 BaseverticesNumA = DynamicMeshA.VertexCount();
	const int32 BaseverticesNumB = DynamicMeshB.VertexCount();

	if (BaseverticesNumA == BaseverticesNumB)
	{
		for (int32 Index = 0; Index < BaseverticesNumB; Index++)
		{
			if (!DynamicMeshA.GetVertex(Index).Equals(DynamicMeshB.GetVertex(Index)))
			{
				//Don't have same position
				return false;
			} else
			{
				const FVector NormalA = FMeshNormals::ComputeVertexNormal(DynamicMeshA, Index);
				const FVector NormalB = FMeshNormals::ComputeVertexNormal(DynamicMeshB, Index);
				if (!NormalA.Equals(NormalB))
				{
					//Don't have same normal
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool FMeshMorpherWrapper::ProjectDeltas(const TArray<FMorphTargetDelta>& InDeltas, const bool bCheckMeshesIdentical, const double Multiplier, const int32 SmoothIterations, const double SmoothStrength, TArray<FMorphTargetDelta>& OutDeltas) const
{
	OutDeltas.Empty();
	if(InDeltas.Num())
	{
		const bool bIdentical = bCheckMeshesIdentical ? IsDynamicMeshIdentical(TargetMesh, SourceMesh) : false;
		if (!bIdentical)
		{
			ComputeDeltaProjection(SourceMesh, TargetMesh, InDeltas, Multiplier, SmoothIterations, SmoothStrength, OutDeltas);
			
		} else
		{
			OutDeltas = InDeltas;
		}		
	}
	return OutDeltas.Num() > 0;
}

bool FMeshMorpherWrapper::ProjectMesh(const bool bCheckMeshesIdentical, const double Multiplier, const int32 SmoothIterations, const double SmoothStrength, TArray<FMorphTargetDelta>& OutDeltas) const
{
	OutDeltas.Empty();

	const bool bIdentical = bCheckMeshesIdentical ? IsDynamicMeshIdentical(TargetMesh, SourceMesh) : false;
	if (!bIdentical)
	{

		const double NormalIncompatibilityMultiplier = 1.0 / FMath::Max(static_cast<double>(1e-6), (1.0 - NormalIncompatibilityThreshold));
		
		FDynamicMesh3 TargetDynamicMesh = TargetMesh;
		
		TArray<TSet<int32>> VerticesSets;
		TArray<FMeshMorpherWrapPair> VertexPairs;
		TArray<int32> NoCorrespondent;
		CalculateDynamicMeshesForTransfer(SourceMesh, TargetDynamicMesh, VerticesSets, VertexPairs, NoCorrespondent);
		{
			const int32 Count = VertexPairs.Num();
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
						const FMeshMorpherWrapPair& VertexPair = VertexPairs[Index];
						const FVector3d Position = SourceMesh.GetVertex(VertexPair.SourceIndex);
						TargetDynamicMesh.SetVertex(VertexPair.TargetIndex, Position, false);
						const FVector SourceNormal = FMeshNormals::ComputeVertexNormal(TargetDynamicMesh, VertexPair.TargetIndex);
						TargetDynamicMesh.SetVertexNormal(VertexPair.TargetIndex, FVector3f(SourceNormal));
					}
				});
			}
		}

		{
			FDynamicMeshAABBTree3 TargetSpatialData(&SourceMesh);
			const int32 Count = NoCorrespondent.Num();
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
						const int32 TargetIndex = NoCorrespondent[Index];

						const FVector& TargetPosition = TargetMesh.GetVertex(TargetIndex);
						const FVector TargetNormal = FMeshNormals::ComputeVertexNormal(TargetMesh, TargetIndex);
						
						IMeshSpatial::FQueryOptions QueryOptions;
						QueryOptions.MaxDistance = TNumericLimits<double>::Max();
						QueryOptions.TriangleFilterF = [&](const int32 TriangleID)
						{
							FVector TriangleNormal, Centroid;
							double Area;
							SourceMesh.GetTriInfo(TriangleID, TriangleNormal, Area, Centroid);
							return FMath::Max(0, (TriangleNormal.Dot(TargetNormal) - NormalIncompatibilityThreshold) * NormalIncompatibilityMultiplier) > 0.0;
						};
						double dist;
						const int32 ClosestTriangle = TargetSpatialData.FindNearestTriangle(TargetPosition, dist, QueryOptions);
						if(ClosestTriangle != IndexConstants::InvalidID)
						{
							FTriangle3d Triangle;
							SourceMesh.GetTriVertices(ClosestTriangle, Triangle.V[0], Triangle.V[1], Triangle.V[2]);

							FDistPoint3Triangle3d DistanceQuery(TargetPosition, Triangle);
							DistanceQuery.GetSquared();
							if (VectorUtil::IsFinite(DistanceQuery.ClosestTrianglePoint))
							{
								TargetDynamicMesh.SetVertex(TargetIndex, DistanceQuery.ClosestTrianglePoint, false);
								const FVector SourceNormal = FMeshNormals::ComputeVertexNormal(TargetDynamicMesh, TargetIndex);
								TargetDynamicMesh.SetVertexNormal(TargetIndex, FVector3f(SourceNormal));								
								return ;
							}							
						}
					}
				});
			}
		}		

		TArray<FMorphTargetDelta> Deltas;
		UMeshOperationsLibraryRT::GetMorphDeltas(TargetMesh, TargetDynamicMesh, Deltas);

		TargetDynamicMesh = TargetMesh;
		ComputeDeltaProjection(TargetMesh, TargetDynamicMesh, Deltas, Multiplier, SmoothIterations, SmoothStrength, OutDeltas);
		
		
	} else
	{
		UMeshOperationsLibraryRT::GetMorphDeltas(TargetMesh, SourceMesh, OutDeltas);
	}	
	
	return OutDeltas.Num() > 0;
}

void FMeshMorpherWrapper::ComputeDeltaProjection(const FDynamicMesh3& SourceDynamicMesh, FDynamicMesh3 TargetDynamicMesh, const TArray<FMorphTargetDelta>& InDeltas, const double Multiplier, const int32 SmoothIterations, const double SmoothStrength, TArray<FMorphTargetDelta>& OutDeltas) const
{
	TArray<TSet<int32>> VerticesSets;
	TArray<FMeshMorpherWrapPair> VertexPairs;
	TArray<int32> NoCorrespondent;
	CalculateDynamicMeshesForTransfer(SourceDynamicMesh, TargetDynamicMesh, VerticesSets, VertexPairs, NoCorrespondent);

	TArray<FMorphTargetDelta> NewDeltas;
	CreateDeltasForVertexPairs(VertexPairs, InDeltas, NewDeltas);

	if (SmoothIterations > 0)
	{
		for (int32 SmoothIndex = 0; SmoothIndex < SmoothIterations; SmoothIndex++)
		{
			GetSmoothDeltas(TargetDynamicMesh, NewDeltas, SmoothStrength, NewDeltas);
		}
	}

	ApplyDeltasToIdenticalVertices(VerticesSets, NewDeltas);

	{
		const int32 Count = NewDeltas.Num();
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
					const FMorphTargetDelta& Delta = NewDeltas[Index];
					const FVector3d Position = TargetDynamicMesh.GetVertex(Delta.SourceIdx) + (FVector3d(Delta.PositionDelta) * Multiplier);
					TargetDynamicMesh.SetVertex(Delta.SourceIdx, Position, false);
					
					const FVector SourceNormal = FMeshNormals::ComputeVertexNormal(TargetDynamicMesh, Delta.SourceIdx);
					TargetDynamicMesh.SetVertexNormal(Delta.SourceIdx, FVector3f(SourceNormal));
				}
			});
		}
	}
			
	UMeshOperationsLibraryRT::GetMorphDeltas(TargetMesh, TargetDynamicMesh, OutDeltas);

}

void FMeshMorpherWrapper::GetSmoothDeltas(const FDynamicMesh3& TargetDynamicMesh, TArray<FMorphTargetDelta>& Deltas, const double& SmoothStrength, TArray<FMorphTargetDelta>& OutSmoothDeltas) const
{
	const int32 TargetverticesNum = TargetDynamicMesh.VertexCount();

	TArray<FVector3f> SmoothDeltas;
	TArray<TArray<FVector3f>> TempSmoothDeltas;
	SmoothDeltas.SetNumZeroed(TargetverticesNum);
	TempSmoothDeltas.SetNumZeroed(TargetverticesNum);

	FCriticalSection Lock;

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
					const FMorphTargetDelta& NewDelta = Deltas[Index];
					SmoothDeltas[static_cast<int32>(NewDelta.SourceIdx)] = NewDelta.PositionDelta;
				}
			});

			ParallelFor(Chunks, [&](const int32 ChunkIndex)
			{
				const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
				for (int X = 0; X < IterationSize; ++X)
				{
					const int32 Index = (ChunkIndex * ChunkSize) + X;
					const FMorphTargetDelta& NewDelta = Deltas[Index];
					for (const FIndex3i& Triangle : TargetDynamicMesh.TrianglesItr())
					{
						if (NewDelta.SourceIdx == static_cast<uint32>(Triangle[0]) || NewDelta.SourceIdx == static_cast<uint32>(Triangle[1]) || NewDelta.SourceIdx == static_cast<uint32>(Triangle[2]))
						{
							Lock.Lock();
							TempSmoothDeltas[Triangle[0]].Add(((SmoothDeltas[Triangle[1]] + SmoothDeltas[Triangle[2]]) / 2 - SmoothDeltas[Triangle[0]]) * SmoothStrength + SmoothDeltas[Triangle[0]]);
							TempSmoothDeltas[Triangle[1]].Add(((SmoothDeltas[Triangle[2]] + SmoothDeltas[Triangle[0]]) / 2 - SmoothDeltas[Triangle[1]]) * SmoothStrength + SmoothDeltas[Triangle[1]]);
							TempSmoothDeltas[Triangle[2]].Add(((SmoothDeltas[Triangle[0]] + SmoothDeltas[Triangle[1]]) / 2 - SmoothDeltas[Triangle[2]]) * SmoothStrength + SmoothDeltas[Triangle[2]]);
							Lock.Unlock();
						}
					}
				}
			});
			
		}
	}

	OutSmoothDeltas.Empty();

	{
		const int32 Count = TempSmoothDeltas.Num();
		if(Count > 0)
		{
			const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
			const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
			const int32 LastChunkSize = Count - (ChunkSize * Cores);
			const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

			ParallelFor(Chunks, [&](const int32 ChunkIndex)
			{
				TArray<FMorphTargetDelta> LocalSmoothDeltas;
				const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
				for (int X = 0; X < IterationSize; ++X)
				{
					const int32 Index = (ChunkIndex * ChunkSize) + X;
					if (TempSmoothDeltas[Index].Num() > 0)
					{
						FMorphTargetDelta NewDelta;
						NewDelta.SourceIdx = static_cast<uint32>(Index);
						NewDelta.PositionDelta = FVector3f::ZeroVector;

						for (const FVector3f& Temp : TempSmoothDeltas[Index])
						{
							NewDelta.PositionDelta += Temp;
						}
						NewDelta.PositionDelta /= TempSmoothDeltas[Index].Num();
						LocalSmoothDeltas.Add(NewDelta);
					}
				}

				if(LocalSmoothDeltas.Num() > 0)
				{
					Lock.Lock();
					OutSmoothDeltas.Append(LocalSmoothDeltas);
					Lock.Unlock();
				}
			});
		}
	}
}

void FMeshMorpherWrapper::ApplyDeltasToIdenticalVertices(const TArray<TSet<int32>>& VerticesSets, TArray<FMorphTargetDelta>& Deltas) const
{
	const int32 Count = VerticesSets.Num();
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
				const auto& VerticeSet = VerticesSets[Index];

				FVector3f Sum = FVector3f::ZeroVector;

				for (const FMorphTargetDelta& NewDelta : Deltas)
				{
					if (VerticeSet.Contains(static_cast<int32>(NewDelta.SourceIdx)))
					{
						Sum += NewDelta.PositionDelta;
					}
				}

				const FVector3f Average = Sum / VerticeSet.Num();

				for (FMorphTargetDelta& NewDelta : Deltas)
				{
					if (VerticeSet.Contains(static_cast<int32>(NewDelta.SourceIdx)))
					{
						NewDelta.PositionDelta = Average;
					}
				}
			}
		});
	}
}

void FMeshMorpherWrapper::CreateDeltasForVertexPairs(const TArray<FMeshMorpherWrapPair>& VertexPairs, const TArray<FMorphTargetDelta>& BaseDeltas, TArray<FMorphTargetDelta>& OutDeltas) const
{
	OutDeltas.Empty();
	const int32 Count = VertexPairs.Num();
	if(Count > 0)
	{
		FCriticalSection Lock;
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
				const FMeshMorpherWrapPair& VertexPair = VertexPairs[Index];
				
				FMorphTargetDelta NewDelta;
				NewDelta.PositionDelta = FVector3f::ZeroVector;
				NewDelta.SourceIdx = static_cast<uint32>(VertexPair.TargetIndex);

				for(const FMorphTargetDelta& Delta : BaseDeltas)
				{
					if (Delta.SourceIdx == static_cast<uint32>(VertexPair.SourceIndex))
					{
						NewDelta.PositionDelta += Delta.PositionDelta;
					}
				}

				if (NewDelta.PositionDelta.Length() > 0.0f)
				{
					LocalDeltas.Add(NewDelta);
				}
			}

			if(LocalDeltas.Num() > 0)
			{
				Lock.Lock();
				OutDeltas.Append(LocalDeltas);
				Lock.Unlock();
			}
		});
	}
}

void FMeshMorpherWrapper::CalculateDynamicMeshesForTransfer(const FDynamicMesh3& SourceDynamicMesh, const FDynamicMesh3& TargetDynamicMesh, TArray<TSet<int32>>& VerticesSets, TArray<FMeshMorpherWrapPair>& VertexPairs, TArray<int32>& NoCorrespondent) const
{
	const double NormalIncompatibilityMultiplier = 1.0 / FMath::Max(static_cast<double>(1e-6), (1.0 - NormalIncompatibilityThreshold));

	VerticesSets.Empty();
	VertexPairs.Empty();
	NoCorrespondent.Empty();
	
	FCriticalSection Lock;

	const int32 Count = TargetDynamicMesh.VertexCount();
	if(Count > 0)
	{
		const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
		const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
		const int32 LastChunkSize = Count - (ChunkSize * Cores);
		const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

		ParallelFor(Chunks, [&](const int32 ChunkIndex)
		{
			TArray<TSet<int32>> LocalVerticeSets;
			TArray<FMeshMorpherWrapPair> LocalVertexPairs;
			TArray<int32> LocalNoCorrespondent;
			const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
			for (int X = 0; X < IterationSize; ++X)
			{
				TSet<int32>& VerticeSet = LocalVerticeSets.Add_GetRef(TSet<int32>());
				const int32 TargetIndex = (ChunkIndex * ChunkSize) + X;

				const FVector TargetPosition = TargetDynamicMesh.GetVertex(TargetIndex);
				const FVector TargetNormal = FVector(TargetDynamicMesh.GetVertexNormal(TargetIndex));
				for (int32 NextTargetIndex = TargetIndex; NextTargetIndex < Count; NextTargetIndex++)
				{
					if (TargetDynamicMesh.IsVertex(TargetIndex) && TargetDynamicMesh.IsVertex(NextTargetIndex))
					{
						if (TargetDynamicMesh.GetVertex(TargetIndex).Equals(TargetDynamicMesh.GetVertex(NextTargetIndex)))
						{
							VerticeSet.Add(TargetIndex);
							VerticeSet.Add(NextTargetIndex);
						}
					}
				}

				double LastDistance = TNumericLimits<double>::Max();
				int32 ClosestCompatibleIndex = INDEX_NONE;

				for (int32 SourceIndex = 0; SourceIndex < SourceDynamicMesh.VertexCount(); SourceIndex++)
				{
					if (SourceDynamicMesh.IsVertex(SourceIndex) && TargetDynamicMesh.IsVertex(TargetIndex))
					{
						const FVector SourceNormal = FVector(SourceDynamicMesh.GetVertexNormal(SourceIndex));
						const bool TriangleNormalCompatibility = FMath::Max(0, (SourceNormal.Dot(TargetNormal) - NormalIncompatibilityThreshold) * NormalIncompatibilityMultiplier) > 0.0;
						
						if(TriangleNormalCompatibility)
						{
							const FVector SourcePosition = SourceDynamicMesh.GetVertex(SourceIndex);
							const double Distance = (SourcePosition - TargetPosition).Size();
							if (Distance < VertexThreshold && Distance < LastDistance)
							{
								LastDistance = Distance;
								ClosestCompatibleIndex = SourceIndex;
							}
						}
					}
				}	

				if(ClosestCompatibleIndex > INDEX_NONE)
				{
					LocalVertexPairs.Add(FMeshMorpherWrapPair(TargetIndex, ClosestCompatibleIndex));
				} else
				{
					LocalNoCorrespondent.Add(TargetIndex);
				}
			}

			Lock.Lock();
			VerticesSets.Append(LocalVerticeSets);
			VertexPairs.Append(LocalVertexPairs);
			NoCorrespondent.Append(LocalNoCorrespondent);
			Lock.Unlock();
		});
	}
}