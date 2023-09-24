// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "MeshMorpherToolkit.h"
#include "AssetData.h"
#include "HttpModule.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Containers/Ticker.h"

class FUICommandList;
class FMenuBuilder;
class USkeletalMesh; 
class UMeshMorpherSettings;

enum class EMeshMorpherNotificationType : uint8
{
	Success,
	Warning,
	Fail
};


/**
* The public interface to this module.  In most cases, this interface is only public to sibling modules
* within this plugin.
*/
class IMeshMorpherModule : public IModuleInterface
{
public:
	static inline IMeshMorpherModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IMeshMorpherModule >("MeshMorpher");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MeshMorpher");
	}


	virtual void OpenToolkitAction() = 0;

	virtual TSharedPtr<class FUICommandList> GetCommands() = 0;
};

class FMeshMorpherModule : public IMeshMorpherModule
{
	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	void OnEnginePreExit();
	void RegisterMenus();
	TSharedRef<FExtender> ContextMenuExtender(const TArray<FAssetData>& Assets);
	virtual void ShutdownModule() override;
	virtual bool SupportsAutomaticShutdown() override;

	virtual bool SupportsDynamicReloading() override;

	void OpenToolkitAction() override;
	void OpenToolkit(USkeletalMesh* Mesh);
	bool Tick(float DeltaTime);
	void CheckForUpdate();
	virtual TSharedPtr<class FUICommandList> GetCommands() override;
private:
	void OnDestroyToolkit(TSharedRef<SDockTab> Tab);
	void ShowNotification(const FText Message, const FString URL, int32 Version, EMeshMorpherNotificationType Type);
	void AddMenuExtension(FMenuBuilder& Builder);

	void AddToolbarExtension(FToolBarBuilder& Builder);

private:

	TArray<FMeshMorpherToolkit*> Toolkits;
	UMeshMorpherSettings* Settings = nullptr;
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest;
	TSharedPtr<SNotificationItem> NotificationItem = nullptr;
	FString Version = "0.0.0";
	int32 TabCounts = 0;
	float ElapsedTime = 0.0f;
	int32 HideVersion = 0;
	bool bFirstTime = true;
};
