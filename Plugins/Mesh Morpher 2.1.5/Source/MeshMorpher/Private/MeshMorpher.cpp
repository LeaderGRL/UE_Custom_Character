// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpher.h"
#include "Preview/PreviewViewportCommands.h"
#include "Framework/Notifications/NotificationManager.h"
#include "MeshMorpherStyle.h"
#include "MeshMorpherCommands.h"
#include "LevelEditor.h"
#include "Interfaces/IHttpResponse.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Engine/StaticMeshActor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Interfaces/IPluginManager.h"
#include "Classes/EditorStyleSettings.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "MeshMorpherSettings.h"
#include "Widgets/SCreateMorphTargetFromMetaWidget.h"
#include "MetaMorph.h"
#include "ToolMenus.h"
#define LOCTEXT_NAMESPACE "FMeshMorpherModule"

const static FName ToolkitTabID("MeshMorpherToolbar");

const static FString VersionURL("https://www.puglifestudio.com/_functions/meshmorpher_latestversion");

void FMeshMorpherModule::StartupModule()
{

	FCoreDelegates::OnEnginePreExit.AddRaw(this, &FMeshMorpherModule::OnEnginePreExit);


	FMeshMorpherStyle::Initialize();

	FMeshMorpherCommands::Register();
	FMeshMorpherPreviewSceneCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMeshMorpherCommands::Get().OpenToolkitAction,
		FExecuteAction::CreateRaw(this, &FMeshMorpherModule::OpenToolkitAction),
		FCanExecuteAction());


	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMeshMorpherModule::RegisterMenus));

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	// Create new delegate that will be called to provide our menu extener
	MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FMeshMorpherModule::ContextMenuExtender));


	auto Plugin = IPluginManager::Get().FindPlugin("MeshMorpher");
	check(Plugin.IsValid());
	Version = Plugin->GetDescriptor().VersionName;

	TickDelegate = FTickerDelegate::CreateRaw(this, &FMeshMorpherModule::Tick);
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

	Settings = GetMutableDefault<UMeshMorpherSettings>();
	check(Settings);
}

void FMeshMorpherModule::OnEnginePreExit()
{
	for (int32 X = Toolkits.Num() - 1; X >= 0; --X)
	{
		if (Toolkits[X])
		{
			Toolkits[X]->RequestClose();;
		}
	}
	Toolkits.Empty();
	//UE_LOG(LogTemp, Warning, TEXT("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"));
}

void FMeshMorpherModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FMeshMorpherCommands::Get().OpenToolkitAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMeshMorpherCommands::Get().OpenToolkitAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}


TSharedRef<FExtender> FMeshMorpherModule::ContextMenuExtender(const TArray<FAssetData>& Assets)
{
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

	bool bAnySkeletalMesh = false;
	bool bAnyMetaMorph = false;

	for (auto AssetIt = Assets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& Asset = *AssetIt;
		bAnySkeletalMesh = bAnySkeletalMesh || (Asset.GetClass()->IsChildOf(USkeletalMesh::StaticClass()) || (Asset.GetClass() == USkeletalMesh::StaticClass()));
		bAnyMetaMorph = bAnyMetaMorph || (Asset.GetClass()->IsChildOf(UMetaMorph::StaticClass()) || (Asset.GetClass() == UMetaMorph::StaticClass()));
	}

	if (bAnySkeletalMesh)
	{
		MenuExtender->AddMenuExtension("GetAssetActions", EExtensionHook::After, nullptr, FMenuExtensionDelegate::CreateLambda([Assets, this](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(

					NSLOCTEXT("AssetTypeActions_SkeletalMesh", "ObjectContext_OpenInMeshMorpher", "Open with Mesh Morpher"),

					NSLOCTEXT("AssetTypeActions_SkeletalMesh", "ObjectContext_OpenInMeshMorpherTooltip", "Opens the Skeletal Mesh in Mesh Morpher."),

					FSlateIcon(),

					FUIAction(FExecuteAction::CreateLambda([&]()
						{
							for (auto AssetIt = Assets.CreateConstIterator(); AssetIt; ++AssetIt)
							{
								const FAssetData& Asset = *AssetIt;
								if (Asset.GetClass()->IsChildOf(USkeletalMesh::StaticClass()) || (Asset.GetClass() == USkeletalMesh::StaticClass()))
								{
									USkeletalMesh* LocalSource = Cast<USkeletalMesh>(Asset.GetAsset());
									if (LocalSource)
									{
										OpenToolkit(LocalSource);
									}
								}
							}
						}), FCanExecuteAction()));

			}));
	}

	if (bAnySkeletalMesh || bAnyMetaMorph)
	{
		MenuExtender->AddMenuExtension("GetAssetActions", EExtensionHook::After, nullptr, FMenuExtensionDelegate::CreateLambda([Assets, this](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(

					NSLOCTEXT("AssetTypeActions_SkeletalMesh_Meta", "ObjectContext_CreateFromMetaMorph", "Create Morph Target(s) From Meta Morph"),

					NSLOCTEXT("AssetTypeActions_SkeletalMesh_Meta", "ObjectContext_CreateFromMetaMorphTooltip", "Creates Morph Target(s) from selected Meta Morph assets."),

					FSlateIcon("MeshMorpherStyle", "MeshMorpher.CreateMorphFromMetaButton"),

					FUIAction(FExecuteAction::CreateLambda([&]()
						{
							TSet<USkeletalMesh*> Targets;
							TSet<UMetaMorph*> Sources;

							for (auto AssetIt = Assets.CreateConstIterator(); AssetIt; ++AssetIt)
							{
								const FAssetData& Asset = *AssetIt;
								if (Asset.GetClass()->IsChildOf(USkeletalMesh::StaticClass()) || (Asset.GetClass() == USkeletalMesh::StaticClass()))
								{
									USkeletalMesh* LocalTarget = Cast<USkeletalMesh>(Asset.GetAsset());
									if (LocalTarget)
									{
										Targets.Add(LocalTarget);
									}
								}

								if (Asset.GetClass()->IsChildOf(UMetaMorph::StaticClass()) || (Asset.GetClass() == UMetaMorph::StaticClass()))
								{
									UMetaMorph* LocalSource = Cast<UMetaMorph>(Asset.GetAsset());
									if (LocalSource)
									{
										Sources.Add(LocalSource);
									}
								}

							}

							Targets.Remove(nullptr);
							Sources.Remove(nullptr);

							auto MorphWindow = SNew(SWindow)
								.CreateTitleBar(true)
								.Title(FText::FromString(TEXT("Create Morph Target(s) from Meta Morph (Experimental)")))
								.SupportsMaximize(false)
								.SupportsMinimize(false)
								.FocusWhenFirstShown(true)
								//.IsTopmostWindow(true)
								.MinWidth(700.f)
								.SizingRule(ESizingRule::Autosized)
								[
									SNew(SCreateMorphTargetFromMetaWidget)
									.Toolkit(nullptr)
									.Targets(Targets)
									.Sources(Sources)
								];

							TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr);

							if (ParentWindow)
							{
								FSlateApplication::Get().AddModalWindow(MorphWindow, ParentWindow);
							}

						}), FCanExecuteAction()));

			}));

	}
	return MenuExtender.ToSharedRef();
}

void FMeshMorpherModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

	if (HttpRequest.IsValid())
	{
		HttpRequest->CancelRequest();
		HttpRequest.Reset();
	}

	FMeshMorpherStyle::Shutdown();
	FMeshMorpherCommands::Unregister();
	FMeshMorpherPreviewSceneCommands::Unregister();
}

bool FMeshMorpherModule::SupportsAutomaticShutdown()
{
	return true;
}

 bool FMeshMorpherModule::SupportsDynamicReloading()
{
	return true;
}

void FMeshMorpherModule::OpenToolkitAction()
{
	TArray<FAssetData> AssetDatas;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	ContentBrowserSingleton.GetSelectedAssets(AssetDatas);

	if (AssetDatas.Num() > 0)
	{
		bool bAnyValidAsset = false;
		for (FAssetData& AssetData : AssetDatas)
		{
			USkeletalMesh* LocalSource = Cast<USkeletalMesh>(AssetData.GetAsset());
			if (LocalSource)
			{
				OpenToolkit(LocalSource);
				bAnyValidAsset = true;
			}
		}

		if (!bAnyValidAsset)
		{
			OpenToolkit(nullptr);
		}

	}
	else {
		OpenToolkit(nullptr);
	}
}

void FMeshMorpherModule::OpenToolkit(USkeletalMesh* Mesh)
{
	TSharedRef<FMeshMorpherToolkit> SpawnedTab = SNew(FMeshMorpherToolkit, Mesh)
		.Label(NSLOCTEXT("MeshMorpherToolkit", "MainTitle", "Mesh Morpher"))
		.ContentPadding(0.0f)
		.OnTabClosed_Raw(this, &FMeshMorpherModule::OnDestroyToolkit)
		.TabRole(ETabRole::MajorTab);

	static_assert(sizeof(EAssetEditorToolkitTabLocation) == sizeof(int32), "EAssetEditorToolkitTabLocation is the incorrect size");

	const UEditorStyleSettings* StyleSettings = GetDefault<UEditorStyleSettings>();

	FName PlaceholderId(TEXT("StandaloneToolkit"));
	TSharedPtr<FTabManager::FSearchPreference> SearchPreference = nullptr;
	if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::Default)
	{
		// Work out where we should create this asset editor
		EAssetEditorToolkitTabLocation SavedAssetEditorToolkitTabLocation = EAssetEditorToolkitTabLocation::Standalone;
		GConfig->GetInt(
			TEXT("AssetEditorToolkitTabLocation"),
			*Mesh->GetPathName(),
			reinterpret_cast<int32&>(SavedAssetEditorToolkitTabLocation),
			GEditorPerProjectIni
		);

		PlaceholderId = (SavedAssetEditorToolkitTabLocation == EAssetEditorToolkitTabLocation::Docked) ? TEXT("DockedToolkit") : TEXT("StandaloneToolkit");
		SearchPreference = MakeShareable(new FTabManager::FLiveTabSearch());
	}

	else if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::NewWindow)
	{
		PlaceholderId = TEXT("StandaloneToolkit");
		SearchPreference = MakeShareable(new FTabManager::FRequireClosedTab());
	}
	else if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::MainWindow)
	{
		PlaceholderId = TEXT("DockedToolkit");
		SearchPreference = MakeShareable(new FTabManager::FLiveTabSearch(TEXT("LevelEditor")));
	}
	else if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::ContentBrowser)
	{
		PlaceholderId = TEXT("DockedToolkit");
		SearchPreference = MakeShareable(new FTabManager::FLiveTabSearch(TEXT("ContentBrowserTab1")));
	}
	else if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::LastDockedWindowOrNewWindow)
	{
		PlaceholderId = TEXT("StandaloneToolkit");
		SearchPreference = MakeShareable(new FTabManager::FLastMajorOrNomadTab(NAME_None));
	}
	else if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::LastDockedWindowOrMainWindow)
	{
		PlaceholderId = TEXT("StandaloneToolkit");
		SearchPreference = MakeShareable(new FTabManager::FLastMajorOrNomadTab(TEXT("LevelEditor")));
	}
	else if (StyleSettings->AssetEditorOpenLocation == EAssetEditorOpenLocation::LastDockedWindowOrContentBrowser)
	{
		PlaceholderId = TEXT("StandaloneToolkit");
		SearchPreference = MakeShareable(new FTabManager::FLastMajorOrNomadTab(TEXT("ContentBrowserTab1")));
	}
	else
	{
		// Add more cases!
		check(false);
	}

	FGlobalTabmanager::Get()->InsertNewDocumentTab(PlaceholderId, *SearchPreference, SpawnedTab);
	TabCounts++;

	TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(SpawnedTab);
	Toolkits.Add(ToolkitPtr.Get());

	if (Settings && Settings->bCheckForUpdates && bFirstTime)
	{
		CheckForUpdate();
		bFirstTime = false;
	}
}

bool FMeshMorpherModule::Tick(float DeltaTime)
{
	ElapsedTime += DeltaTime;
	if (Settings)
	{
		if (ElapsedTime >= (float)Settings->CheckInterval)
		{
			ElapsedTime = 0.0f;
			if (TabCounts > 0 && Settings->CheckInterval > 0.0)
			{
				CheckForUpdate();
			}
		}
	}
	return true;
}


void FMeshMorpherModule::CheckForUpdate()
{
	if (Settings && Settings->bCheckForUpdates && !HttpRequest.IsValid())
	{
		HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetVerb("GET");
		HttpRequest->SetURL(VersionURL);

		if (!FMath::IsNearlyZero(Settings->ConnectionTimeout))
		{
			HttpRequest->SetTimeout(Settings->ConnectionTimeout);
		}

		auto BytesToString = [](const uint8* In, int32 Count)
		{
			FString Result;
			Result.Empty(Count);

			while (Count)
			{
				int16 Value = *In;

				Result += TCHAR(Value);

				++In;
				Count--;
			}
			return Result;
		};


		auto OnResponseReceived = [this, &BytesToString](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (bWasSuccessful && Response.IsValid())
			{
				TArray<uint8> RawFileData = Response->GetContent();

				FString ServerResponse = BytesToString(RawFileData.GetData(), RawFileData.Num());

				TSharedPtr<FJsonObject> JsonObject;
				FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(ServerResponse), JsonObject);

				FString ServerVersion = "";
				FString ServerURL = "";

				if (JsonObject.IsValid())
				{
					ServerVersion = JsonObject->GetStringField("Version");
					ServerURL = JsonObject->GetStringField("URL");
				}

				const int32 ServerVersionNumber = FCString::Atoi(*ServerVersion.Replace(TEXT("."), TEXT("")));
				const int32 LocalVersionNumber = FCString::Atoi(*Version.Replace(TEXT("."), TEXT("")));

				if (ServerVersionNumber > LocalVersionNumber)
				{

					FTextBuilder NotificationBuilder;
					NotificationBuilder.AppendLine(FText::Format(LOCTEXT("NotificationMessage1", "There is a newer version available. (v{0}) "), FText::FromString(ServerVersion)));
					NotificationBuilder.AppendLine(FText::Format(LOCTEXT("NotificationMessage2", "Your current installed version is (v{0})."), FText::FromString(Version)));
					ShowNotification(NotificationBuilder.ToText(), ServerURL, ServerVersionNumber, EMeshMorpherNotificationType::Warning);
				}
				else if (ServerVersionNumber == LocalVersionNumber)
				{
					//UE_LOG(LogTemp, Warning, TEXT("You have the latest version (%s)"), *Version);
				}
				else {
					//UE_LOG(LogTemp, Warning, TEXT("Local version is higher than the server version (%s)"), *Version);
				}
			}

			if (HttpRequest.IsValid())
			{
				HttpRequest.Reset();
			}
		};

		HttpRequest->OnProcessRequestComplete().BindLambda(OnResponseReceived);
		HttpRequest->ProcessRequest();
	}
}


TSharedPtr<class FUICommandList> FMeshMorpherModule::GetCommands()
{
	return PluginCommands;
}

void FMeshMorpherModule::OnDestroyToolkit(TSharedRef<SDockTab> Tab)
{
	TSharedPtr<FMeshMorpherToolkit> ToolkitPtr = StaticCastSharedRef<FMeshMorpherToolkit>(Tab);
	//TSharedRef<FMeshMorpherToolkit> Toolkit = StaticCastSharedRef<FMeshMorpherToolkit>(Tab);
	if (ToolkitPtr.IsValid())
	{
		ToolkitPtr->RequestClose();
		Toolkits.Remove(ToolkitPtr.Get());
	}

	TabCounts--;
	TabCounts = FMath::Clamp(TabCounts, 0, MAX_int32);
	if (TabCounts <= 0)
	{
		if (HttpRequest.IsValid())
		{
			HttpRequest->CancelRequest();
			HttpRequest.Reset();
		}
	}
}

void FMeshMorpherModule::ShowNotification(const FText Message, const FString URL, int32 ServerVersion, EMeshMorpherNotificationType Type)
{
	if (!NotificationItem.IsValid() && ServerVersion != HideVersion)
	{
		FNotificationInfo Info(Message);
		Info.bUseSuccessFailIcons = true;
		Info.bFireAndForget = false;
		Info.bUseLargeFont = true;
		Info.bUseThrobber = false;
		Info.FadeOutDuration = 0.5f;
		
		if (!URL.IsEmpty())
		{
			auto HyperlinkFunc = [URL]()
			{
				FPlatformProcess::LaunchURL(*URL, NULL, NULL);
			};

			Info.Hyperlink.BindLambda(HyperlinkFunc);
			Info.HyperlinkText = LOCTEXT("PatchNotes", "Patch Notes");
		}


		auto ButtonFunc = [this]()
		{
			if (NotificationItem.IsValid())
			{
				//NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
				NotificationItem->ExpireAndFadeout();
				NotificationItem = nullptr;
				ElapsedTime = 0.0f;
			}
		};

		auto ButtonFunc2 = [this, ServerVersion]()
		{
			if (NotificationItem.IsValid())
			{
				//NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
				NotificationItem->ExpireAndFadeout();
				NotificationItem = nullptr;
				HideVersion = ServerVersion;
				ElapsedTime = 0.0f;
			}
		};

		Info.ButtonDetails.Add(FNotificationButtonInfo(LOCTEXT("CloseNotification", "Close"), LOCTEXT("CloseNotificationTT", "Close"), FSimpleDelegate::CreateLambda(ButtonFunc)));
		Info.ButtonDetails.Add(FNotificationButtonInfo(LOCTEXT("DontShowNotification2", "Don't show again"), LOCTEXT("DontShowNotificationTT2", "Hides this notification for the current session."), FSimpleDelegate::CreateLambda(ButtonFunc2)));

		switch (Type)
		{
		case EMeshMorpherNotificationType::Success:
			Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
			break;
		case EMeshMorpherNotificationType::Warning:
			Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Warning"));
			break;
		case EMeshMorpherNotificationType::Fail:
			Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.FailImage"));
			break;
		}

		NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
	}
}


void FMeshMorpherModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FMeshMorpherCommands::Get().OpenToolkitAction);
}

void FMeshMorpherModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FMeshMorpherCommands::Get().OpenToolkitAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshMorpherModule, MeshMorpher)
