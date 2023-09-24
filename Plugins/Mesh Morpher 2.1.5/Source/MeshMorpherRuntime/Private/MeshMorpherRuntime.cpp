// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#include "MeshMorpherRuntime.h"
#define LOCTEXT_NAMESPACE "FMeshMorpherRuntimeModule"

void FMeshMorpherRuntimeModule::StartupModule()
{


}

void FMeshMorpherRuntimeModule::ShutdownModule()
{

}

bool FMeshMorpherRuntimeModule::SupportsAutomaticShutdown()
{
	return true;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshMorpherRuntimeModule, MeshMorpherRuntime)
