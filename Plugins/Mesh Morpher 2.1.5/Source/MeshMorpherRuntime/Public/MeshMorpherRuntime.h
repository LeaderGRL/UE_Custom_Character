// Copyright 2017-2020 SC Pug Life Studio S.R.L, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
* The public interface to this module.  In most cases, this interface is only public to sibling modules
* within this plugin.
*/
class IMeshMorpherRuntimeModule : public IModuleInterface
{
public:
	static inline IMeshMorpherRuntimeModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IMeshMorpherRuntimeModule >("MeshMorpherRuntime");
	}
};

class FMeshMorpherRuntimeModule : public IMeshMorpherRuntimeModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	bool SupportsAutomaticShutdown() override;
};
