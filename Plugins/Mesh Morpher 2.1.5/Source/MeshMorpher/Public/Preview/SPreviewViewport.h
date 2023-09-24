// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"

class ISlateStyle;
class SDockTab;
class FEditorViewportClient;
class FAdvancedPreviewScene;
class UMeshMorpherEdMode;
class USkeletalMeshComponent;
class UMeshMorpherSettings;

class SMeshMorpherPreviewViewport : public SEditorViewport
{
	friend class FMeshMorpherToolkit;
	friend class SBakeSkeletalMeshWidget;
	friend class SCreateMorphTargetFromObjWidget;
	friend class SCreateMorphTargetFromFBXWidget;
	friend class SCreateMetaMorphFromFBXWidget;
public:

	SLATE_BEGIN_ARGS(SMeshMorpherPreviewViewport)
	{}
	SLATE_END_ARGS()

	SMeshMorpherPreviewViewport();
	~SMeshMorpherPreviewViewport();
	void Construct(const FArguments& InArgs);
	void AddComponent(UPrimitiveComponent* Component, FTransform Transform = FTransform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1.0f, 1.0f, 1.0f)), bool bAttachToRoot = true);
	void RemoveComponent(UPrimitiveComponent* Component);
	EVisibility GetToolbarVisibility() const;
	TSharedPtr<class FEditorViewportClient> GetViewportClient();
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);
	UMeshMorpherEdMode* GetEditorMode();
protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	virtual void BindCommands() override;
	void HandleToggleFloor();
	void HandleToggleEnvironment();

private:

	TSharedPtr<FEditorViewportClient> ViewportClient;
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	UMeshMorpherSettings* Settings = nullptr;
};