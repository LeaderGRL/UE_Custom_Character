// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherEditor.h"
#include "MeshMorpherEdMode.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "MeshMorpherSettings.h"

#define LOCTEXT_NAMESPACE "FMeshMorpherEditorModule"

void FMeshMorpherEditorModule::StartupModule()
{

	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Editor", "Plugins", "Mesh Morpher",
			LOCTEXT("MeshMorpherSettingsName", "Mesh Morpher"),
			LOCTEXT("MeshMorpherSettingsDescription", "Configure Mesh Morpher plug-in."),
			GetMutableDefault<UMeshMorpherSettings>()
		);
	}
}

void FMeshMorpherEditorModule::ShutdownModule()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "Mesh Morpher");
	}

}

bool FMeshMorpherEditorModule::SupportsAutomaticShutdown()
{
	return true;
}

 bool FMeshMorpherEditorModule::SupportsDynamicReloading()
{
	return true;
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshMorpherEditorModule, MeshMorpherEditor)
