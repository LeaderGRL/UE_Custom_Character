// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Preview/SPreviewViewportToolbar.h"
#include "Preview/SPreviewViewport.h"
#include "EditorViewportCommands.h"
#include "LevelEditor.h"
#include "SEditorViewportToolBarButton.h"
#include "SEditorViewportToolBarMenu.h"
#include "SEditorViewportViewMenu.h"
#include "Types/ISlateMetaData.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "EditorStyleSet.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Preview/PreviewViewportCommands.h"
#include "Widgets/Input/SSlider.h"
#include "SViewportToolBarIconMenu.h"
#include "MeshMorpherSettings.h"

#define LOCTEXT_NAMESPACE "MeshMorpherViewportToolbar"

void SMeshMorpherViewportToolbar::Construct(const FArguments& InArgs)
{
	Viewport = InArgs._Viewport;

	TSharedRef<SMeshMorpherPreviewViewport> ViewportRef = Viewport.Pin().ToSharedRef();

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

	const FMargin ToolbarSlotPadding(2.0f, 2.0f);
	const FMargin ToolbarButtonPadding(2.0f, 0.0f);

	static const FName DefaultForegroundName("DefaultForeground");

	FName ToolBarStyle = "EditorViewportToolBar";


	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		// Color and opacity is changed based on whether or not the mouse cursor is hovering over the toolbar area
		.ForegroundColor(FEditorStyle::GetSlateColor(DefaultForegroundName))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ToolbarSlotPadding)
				[
					SNew(SEditorViewportToolbarMenu)
					.Label(LOCTEXT("ShowMenuTitle", "Options"))
					.Cursor(EMouseCursor::Default)
					.ParentToolBar(SharedThis(this))
					.ToolTipText(LOCTEXT("Options_ToolTip", "Options"))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.ShowMenu")))
					.OnGetMenuContent(this, &SMeshMorpherViewportToolbar::GeneratePropertiesMenu)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ToolbarSlotPadding)
				[
					SNew(SEditorViewportToolbarMenu)
					.Label(LOCTEXT("ShowCamMenuTitle", "Perspective"))
					.Cursor(EMouseCursor::Default)
					.ParentToolBar(SharedThis(this))
					.ToolTipText(LOCTEXT("CameraMenu_ToolTip", "Camera Menu"))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.CameraMenu")))
					.OnGetMenuContent(this, &SMeshMorpherViewportToolbar::GenerateCameraMenu)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ToolbarSlotPadding)
				[
					SNew(SEditorViewportViewMenu, ViewportRef, SharedThis(this))
					.Cursor(EMouseCursor::Default)
					.MenuExtenders(GetViewMenuExtender())
					.ToolTipText(LOCTEXT("View_ToolTip", "View Menu"))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ViewMenuButton")))
				]
				+ SHorizontalBox::Slot()
				.Padding(ToolbarSlotPadding)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SViewportToolBarIconMenu)
					.Cursor(EMouseCursor::Default)
					.Style(ToolBarStyle)
					.Label(this, &SMeshMorpherViewportToolbar::GetCameraSpeedLabel)
					.OnGetMenuContent(this, &SMeshMorpherViewportToolbar::FillCameraSpeedMenu)
					.ToolTipText(LOCTEXT("CameraMenu_ToolTip", "Camera Menu"))
					.Icon(FSlateIcon(FEditorStyle::GetStyleSetName(), "EditorViewport.CamSpeedSetting"))
					.ParentToolBar(SharedThis(this))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("CameraSpeedButton")))
				]
			]
		]
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef<SWidget> SMeshMorpherViewportToolbar::GeneratePropertiesMenu() const
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder PropertiesMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList());

	const FMeshMorpherPreviewSceneCommands& PreviewViewportActions = FMeshMorpherPreviewSceneCommands::Get();

	PropertiesMenuBuilder.BeginSection("LevelViewportViewportOptions");

	PropertiesMenuBuilder.AddMenuEntry(PreviewViewportActions.ToggleEnvironment);
	PropertiesMenuBuilder.AddMenuEntry(PreviewViewportActions.ToggleFloor);
	PropertiesMenuBuilder.EndSection();

	return PropertiesMenuBuilder.MakeWidget();
}


TSharedRef<SWidget> SMeshMorpherViewportToolbar::GenerateCameraMenu() const
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;

	FMenuBuilder CameraMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList());

	// Camera types
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Perspective);

	CameraMenuBuilder.BeginSection("LevelViewportCameraType_Ortho", LOCTEXT("CameraTypeHeader_Ortho", "Orthographic"));
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Top);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Bottom);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Left);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Right);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Front);
	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Back);
	CameraMenuBuilder.EndSection();

	return CameraMenuBuilder.MakeWidget();
}

TSharedPtr<FExtender> SMeshMorpherViewportToolbar::GetViewMenuExtender()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FExtender> LevelEditorExtenders = LevelEditorModule.GetMenuExtensibilityManager()->GetAllExtenders();
	return LevelEditorExtenders;
}

EVisibility SMeshMorpherViewportToolbar::GetTransformToolbarVisibility() const
{
	return EVisibility::Visible;
}

TSharedRef<SWidget> SMeshMorpherViewportToolbar::FillCameraSpeedMenu()
{
	TSharedRef<SWidget> ReturnWidget = SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush(TEXT("Menu.Background")))
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 2.0f, 60.0f, 2.0f) )
		.HAlign( HAlign_Left )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("MouseSettingsCamSpeed", "Camera Speed")  )
			.Font( FEditorStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 4.0f) )
		[	
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding( FMargin(0.0f, 2.0f) )
			[
				SAssignNew(CamSpeedSlider, SSlider)
				.Value(this, &SMeshMorpherViewportToolbar::GetCamSpeedSliderPosition)
				.OnValueChanged(this, &SMeshMorpherViewportToolbar::OnSetCamSpeed)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 8.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew( STextBlock )
				.Text(this, &SMeshMorpherViewportToolbar::GetCameraSpeedLabel )
				.Font( FEditorStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
			]
		] // Camera Speed Scalar
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 2.0f, 60.0f, 2.0f))
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MouseSettingsCamSpeedScalar", "Camera Speed Scalar"))
				.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SAssignNew(CamSpeedScalarBox, SSpinBox<float>)
				.MinValue(1)
 			    .MaxValue(TNumericLimits<int32>::Max())
			    .MinSliderValue(1)
			    .MaxSliderValue(128)
				.Value(this, &SMeshMorpherViewportToolbar::GetCamSpeedScalarBoxValue)
				.OnValueChanged(this, &SMeshMorpherViewportToolbar::OnSetCamSpeedScalarBoxValue)
				.ToolTipText(LOCTEXT("CameraSpeedScalar_ToolTip", "Scalar to increase camera movement range"))
			]
		]
		// Camera Speed Scalar
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 2.0f, 60.0f, 2.0f))
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MouseSettingsCamFOV", "Camera FOV"))
				.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SAssignNew(CamFOVScalarBox, SSpinBox<float>)
				.MinValue(5)
 			    .MaxValue(170)
			    .MinSliderValue(5)
			    .MaxSliderValue(170)
				.Value(this, &SMeshMorpherViewportToolbar::GetCamFOVScalarBoxValue)
				.OnValueChanged(this, &SMeshMorpherViewportToolbar::OnSetCamFOVScalarBoxValue)
				.ToolTipText(LOCTEXT("CameraFOV_ToolTip", "Scalar to change camera FOV"))
			]
		]
	];

	Settings = GetMutableDefault<UMeshMorpherSettings>();
	
	return ReturnWidget;
}


FText SMeshMorpherViewportToolbar::GetCameraSpeedLabel() const
{
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			return FText::AsNumber(ViewportPin->GetViewportClient()->GetCameraSpeedSetting());
		}
	}

	return FText();
}

float SMeshMorpherViewportToolbar::GetCamSpeedSliderPosition() const
{
	float SliderPos = 0.f;
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			SliderPos = (ViewportPin->GetViewportClient()->GetCameraSpeedSetting() - 1) / ((float)FEditorViewportClient::MaxCameraSpeeds - 1);
		}
	}
	return SliderPos;
}


void SMeshMorpherViewportToolbar::OnSetCamSpeed(float NewValue)
{
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			const int32 SpeedSetting = NewValue * ((float)FEditorViewportClient::MaxCameraSpeeds - 1) + 1;
			ViewportPin->GetViewportClient()->SetCameraSpeedSetting(SpeedSetting);
			if(Settings)
			{
				Settings->CameraSpeed = SpeedSetting;
			}
		}
	}
}

FText SMeshMorpherViewportToolbar::GetCameraSpeedScalarLabel() const
{
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			return FText::AsNumber(ViewportPin->GetViewportClient()->GetCameraSpeedScalar());
		}
	}

	return FText();
}

float SMeshMorpherViewportToolbar::GetCamSpeedScalarBoxValue() const
{
	float CamSpeedScalar = 1.f;
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			CamSpeedScalar = (ViewportPin->GetViewportClient()->GetCameraSpeedScalar());
		}
	}

	return CamSpeedScalar;
}
void SMeshMorpherViewportToolbar::OnSetCamSpeedScalarBoxValue(float NewValue)
{
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			ViewportPin->GetViewportClient()->SetCameraSpeedScalar(NewValue);
			if(Settings)
			{
				Settings->CameraSpeedScalar = NewValue;
			}
		}
	}
}

float SMeshMorpherViewportToolbar::GetCamFOVScalarBoxValue() const
{
	float CamFOVScalar = 60.f;
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			CamFOVScalar = (ViewportPin->GetViewportClient()->ViewFOV);
		}
	}

	return CamFOVScalar;
}
void SMeshMorpherViewportToolbar::OnSetCamFOVScalarBoxValue(float NewValue)
{
	if (Viewport.IsValid())
	{
		auto ViewportPin = Viewport.Pin();
		if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
		{
			ViewportPin->GetViewportClient()->ViewFOV = NewValue;
			if(Settings)
			{
				Settings->CameraFOV = NewValue;
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE 