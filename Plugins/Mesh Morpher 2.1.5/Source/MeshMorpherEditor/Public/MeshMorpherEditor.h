// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

/**
* The public interface to this module.  In most cases, this interface is only public to sibling modules
* within this plugin.
*/
class IMeshMorpherEditorModule : public IModuleInterface
{
public:
	static inline IMeshMorpherEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IMeshMorpherEditorModule >("MeshMorpherEditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MeshMorpherEditor");
	}
};

class FMeshMorpherEditorModule : public IMeshMorpherEditorModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsAutomaticShutdown() override;

	virtual bool SupportsDynamicReloading() override;
};
