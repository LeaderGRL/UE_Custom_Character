// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "SViewportToolBar.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Widgets/Input/SSpinBox.h"

class SMeshMorpherPreviewViewport;
class SSlider;
class UMeshMorpherSettings;
/**
* A level viewport toolbar widget that is placed in a viewport
*/
class MESHMORPHER_API SMeshMorpherViewportToolbar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(SMeshMorpherViewportToolbar) {}
	SLATE_ARGUMENT(TSharedPtr<class SMeshMorpherPreviewViewport>, Viewport)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> GeneratePropertiesMenu() const;
	TSharedRef<SWidget> GenerateCameraMenu() const;
	TSharedPtr<FExtender> GetViewMenuExtender();

	EVisibility GetTransformToolbarVisibility() const;

	TSharedRef<SWidget> FillCameraSpeedMenu();

	FText GetCameraSpeedLabel() const;

	float GetCamSpeedSliderPosition() const;

	void OnSetCamSpeed(float NewValue);

	FText GetCameraSpeedScalarLabel() const;

	float GetCamSpeedScalarBoxValue() const;

	void OnSetCamSpeedScalarBoxValue(float NewValue);

	float GetCamFOVScalarBoxValue() const;

	void OnSetCamFOVScalarBoxValue(float NewValue);

private:
	/** The viewport that we are in */
	TWeakPtr<class SMeshMorpherPreviewViewport> Viewport;
	/** Reference to the camera slider used to display current camera speed */
	mutable TSharedPtr< SSlider > CamSpeedSlider;

	/** Reference to the camera spinbox used to display current camera speed scalar */
	mutable TSharedPtr< SSpinBox<float> > CamSpeedScalarBox;
	mutable TSharedPtr< SSpinBox<float> > CamFOVScalarBox;
	UMeshMorpherSettings* Settings = nullptr;
};