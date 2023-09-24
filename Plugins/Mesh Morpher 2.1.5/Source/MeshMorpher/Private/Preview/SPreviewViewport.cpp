// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Preview/SPreviewViewport.h"
#include "Preview/SPreviewViewportToolbar.h"
#include "AdvancedPreviewScene.h"
#include "Preview/PreviewViewportCommands.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "AssetViewerSettings.h"
#include "EditorViewportClient.h"
#include "AssetEditorModeManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "EditorModeManager.h"
#include "MeshMorpherEdMode.h"
#include "MeshMorpherSettings.h"
#include "Snapping/ModelingSceneSnappingManager.h"

#define LOCTEXT_NAMESPACE "SMeshMorpherPreviewViewport"

TSharedPtr<class FEditorViewportClient> SMeshMorpherPreviewViewport::GetViewportClient()
{
	return ViewportClient;
}

UMeshMorpherEdMode* SMeshMorpherPreviewViewport::GetEditorMode()
{
	if (ViewportClient.IsValid())
	{
		auto ModeTools = ViewportClient->GetModeTools();
		if (ModeTools)
		{
			return (UMeshMorpherEdMode*)(ModeTools->GetActiveScriptableMode(UMeshMorpherEdMode::EM_MeshMorpherEdModeId));
		}
	}
	return nullptr;

}

void SMeshMorpherPreviewViewport::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (ViewportClient.IsValid())
	{
		ViewportClient->Tick(InDeltaTime);
		GEditor->UpdateSingleViewportClient(ViewportClient.Get(), true, false);

		if(Settings)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Widget: %s"), *ViewportClient->GetViewLocation().ToString());
			Settings->CameraPosition = ViewportClient->GetViewLocation();
			Settings->CameraRotation = ViewportClient->GetViewRotation();
		}
	}

	if (!GIntraFrameDebuggingGameThread)
	{
		if (PreviewScene.IsValid())
		{
			PreviewScene->GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
		}
	}
}

SMeshMorpherPreviewViewport::SMeshMorpherPreviewViewport()
{
	FPreviewScene::ConstructionValues CVS;
	CVS.bForceMipsResident = false;
	CVS.bCreatePhysicsScene = false;
	CVS.bShouldSimulatePhysics = false;
	CVS.bAllowAudioPlayback = false;
	PreviewScene = MakeShareable(new FAdvancedPreviewScene(CVS));
}

SMeshMorpherPreviewViewport::~SMeshMorpherPreviewViewport()
{

	if (ViewportClient->GetModeTools())
	{
		ViewportClient->GetModeTools()->DeactivateMode(UMeshMorpherEdMode::EM_MeshMorpherEdModeId);

	}

	if (ViewportClient.IsValid())
	{
		ViewportClient->Viewport = NULL;
	}
	ViewportClient.Reset();
}


void SMeshMorpherPreviewViewport::Construct(const FArguments& InArgs)
{
	SEditorViewport::Construct(SEditorViewport::FArguments());
	
}

void SMeshMorpherPreviewViewport::AddComponent(UPrimitiveComponent* Component, FTransform Transform, bool bAttachToRoot)
{
	if (Component)
	{
		PreviewScene->AddComponent(Component, Transform, bAttachToRoot);
	}
}

void SMeshMorpherPreviewViewport::RemoveComponent(UPrimitiveComponent* Component)
{
	if (PreviewScene.IsValid())
	{
		PreviewScene->RemoveComponent(Component);
	}
}


EVisibility SMeshMorpherPreviewViewport::GetToolbarVisibility() const
{
	return EVisibility::Visible;
}

TSharedRef<FEditorViewportClient> SMeshMorpherPreviewViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShareable(new FEditorViewportClient(nullptr, PreviewScene.Get(), SharedThis(this)));

	((FAssetEditorModeManager*)ViewportClient->GetModeTools())->SetPreviewScene(PreviewScene.Get());

	ViewportClient->SetRealtime(true);
	ViewportClient->EngineShowFlags.SetSnap(0);
	ViewportClient->EngineShowFlags.CompositeEditorPrimitives = true;

	Settings = GetMutableDefault<UMeshMorpherSettings>();
	
	if(Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("Widget: %s"), *ViewportClient->GetViewLocation().ToString());
		ViewportClient->SetViewLocation(Settings->CameraPosition);
		ViewportClient->SetViewRotation(Settings->CameraRotation);
		ViewportClient->SetCameraSpeedSetting(Settings->CameraSpeed);
		ViewportClient->SetCameraSpeedScalar(Settings->CameraSpeedScalar);
		ViewportClient->ViewFOV = Settings->CameraFOV;
	} else
	{
		const FVector CameraLocation(500, 300, 500);
		ViewportClient->SetViewLocation(CameraLocation);
		ViewportClient->SetViewRotation(UKismetMathLibrary::FindLookAtRotation(CameraLocation, FVector::ZeroVector));
	}

	ViewportClient->bSetListenerPosition = false;

	ViewportClient->VisibilityDelegate.BindSP(this, &SMeshMorpherPreviewViewport::IsVisible);

	if (ViewportClient->GetModeTools())
	{
		ViewportClient->GetModeTools()->ActivateMode(UMeshMorpherEdMode::EM_MeshMorpherEdModeId);
	}

	return ViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SMeshMorpherPreviewViewport::MakeViewportToolbar()
{
	// Build our toolbar level toolbar
	TSharedRef< SMeshMorpherViewportToolbar > ToolBar =
		SNew(SMeshMorpherViewportToolbar)
		.Viewport(SharedThis(this))
		.Visibility(this, &SMeshMorpherPreviewViewport::GetToolbarVisibility)
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute());

	return
		SNew(SVerticalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		[
			ToolBar
		];
}

void SMeshMorpherPreviewViewport::BindCommands()
{
	SEditorViewport::BindCommands();
	const FMeshMorpherPreviewSceneCommands& Commands = FMeshMorpherPreviewSceneCommands::Get();

	GetCommandList()->MapAction(Commands.ToggleFloor,
		FExecuteAction::CreateRaw(this, &SMeshMorpherPreviewViewport::HandleToggleFloor));

	GetCommandList()->MapAction(Commands.ToggleEnvironment,
		FExecuteAction::CreateRaw(this, &SMeshMorpherPreviewViewport::HandleToggleEnvironment));
}

void SMeshMorpherPreviewViewport::HandleToggleFloor()
{
	if (PreviewScene.IsValid())
	{
		auto DefaultSettings = UAssetViewerSettings::Get();
		check(DefaultSettings);
		int32 CurrentProfileIndex = 0;
		CurrentProfileIndex = DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex) ? GetDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex : 0;
		PreviewScene->SetFloorVisibility(!DefaultSettings->Profiles[CurrentProfileIndex].bShowFloor);
	}
}

void SMeshMorpherPreviewViewport::HandleToggleEnvironment()
{
	if (PreviewScene.IsValid())
	{
		auto DefaultSettings = UAssetViewerSettings::Get();
		check(DefaultSettings);
		int32 CurrentProfileIndex = 0;
		CurrentProfileIndex = DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex) ? GetDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex : 0;
		PreviewScene->SetEnvironmentVisibility(!DefaultSettings->Profiles[CurrentProfileIndex].bShowEnvironment);
	}
}


#undef LOCTEXT_NAMESPACE
