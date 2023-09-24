// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SMeshMorpherTool.h"
#include "SlateOptMacros.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Tools/MeshMorpherTool.h"
#include "Modules/ModuleManager.h"
#include "MeshMorpherToolkit.h"
#include "Blueprint/UserWidget.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SMeshMorpherToolWidget"

void SMeshMorpherToolWidget::Construct(const FArguments& InArgs)
{
	EdMode = InArgs._EdMode;
	checkf(EdMode, TEXT("Invalid Editor Mode!"))

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NotifyHook = nullptr;
	DetailsViewArgs.bSearchInitialKeyFocus = false;
	DetailsViewArgs.ViewIdentifier = NAME_None;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	// add customization that hides header labels
	TSharedPtr<FMeshMorpherDetailRootObjectCustomization> RootObjectCustomization
		= MakeShared<FMeshMorpherDetailRootObjectCustomization>();
	DetailsPanel->SetRootObjectCustomizationInstance(RootObjectCustomization);

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.Padding(5)
				[
					SAssignNew(ToolHeaderLabel, STextBlock)
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					.Text(LOCTEXT("SelectToolLabel", "Select a Morph Target"))
					.Justification(ETextJustify::Center)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.AutoHeight()
				.Padding(10, 10, 10, 15)
				[
					SAssignNew(ToolMessageArea, STextBlock)
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
					.ColorAndOpacity(FSlateColor(FLinearColor::White * 0.7f))
					.Text(LOCTEXT("ToolMessageLabel", ""))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.AutoHeight()
				.Padding(10, 5, 10, 15)
				[
					SAssignNew(ToolWarningArea, STextBlock)
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f)))
					.Text(LOCTEXT("ToolMessageLabel", ""))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.AutoHeight()
				[
					DetailsPanel->AsShared()
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillHeight(1.f)
				[
					SAssignNew(Content, SBorder)
				]
			]
		]
	];

	ClearNotification();
	ClearWarning();
	EdMode->GetToolManager()->OnToolStarted.AddLambda([this](UInteractiveToolManager* Manager, UInteractiveTool* Tool)

	{
		UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(Tool);
		if (CurTool && !CurTool->IsUnreachable())
		{
			DetailsPanel->SetObjects(CurTool->GetToolProperties());
			CurTool->OnPropertySetsModified.AddLambda([this, CurTool] { if (this && CurTool && !CurTool->IsUnreachable() && DetailsPanel.IsValid()) DetailsPanel->SetObjects(CurTool->GetToolProperties());  });

			if (Content.IsValid())
			{
				CurTool->OnToolChanged.AddLambda([this](UUserWidget* Tool)
				{
						if (Tool)
						{
							Content->SetContent(Tool->TakeWidget());
						}
						else {
							Content->SetContent(SNullWidget::NullWidget);
						}
						
				});
			}

		}

		ToolHeaderLabel->SetText(CurTool->GetClass()->GetDisplayNameText());
	});

	EdMode->GetToolManager()->OnToolEnded.AddLambda([this](UInteractiveToolManager* Manager, UInteractiveTool* Tool)
	{
		UMeshMorpherTool* CurTool = Cast<UMeshMorpherTool>(Tool);
		if (CurTool && !CurTool->IsUnreachable())
		{
			CurTool->OnPropertySetsModified.RemoveAll(this);
			CurTool->OnToolChanged.RemoveAll(this);
		}
		if (ToolHeaderLabel.IsValid())
		{
			ToolHeaderLabel->SetText(LOCTEXT("SelectToolLabel", "Select a Morph Target"));
		}

		ClearNotification();
		ClearWarning();
	});

	EdMode->OnToolNotificationMessage.AddLambda([this](const FText& Message)
	{
		PostNotification(Message);
	});
	EdMode->OnToolWarningMessage.AddLambda([this](const FText& Message)
	{
		PostWarning(Message);
	});
}

SMeshMorpherToolWidget::~SMeshMorpherToolWidget()
{
	if (EdMode)
	{
		if (EdMode->GetToolManager())
		{
			EdMode->GetToolManager()->OnToolStarted.RemoveAll(this);
			EdMode->GetToolManager()->OnToolEnded.RemoveAll(this);
		}
		EdMode->OnToolNotificationMessage.RemoveAll(this);
		EdMode->OnToolWarningMessage.RemoveAll(this);
	}
}

void SMeshMorpherToolWidget::PostNotification(const FText& Message)
{
	if (ToolMessageArea.IsValid())
	{
		ToolMessageArea->SetText(Message);
		if (Message.IsEmpty())
		{
			ToolMessageArea->SetVisibility(EVisibility::Collapsed);
		}
		else {
			ToolMessageArea->SetVisibility(EVisibility::Visible);
		}

	}
}

void SMeshMorpherToolWidget::ClearNotification()
{
	if (ToolMessageArea.IsValid())
	{
		ToolMessageArea->SetText(FText());
		ToolMessageArea->SetVisibility(EVisibility::Collapsed);
	}
}


void SMeshMorpherToolWidget::PostWarning(const FText& Message)
{
	if (ToolWarningArea.IsValid())
	{
		ToolWarningArea->SetText(Message);
		ToolWarningArea->SetVisibility(EVisibility::Visible);
	}
}

void SMeshMorpherToolWidget::ClearWarning()
{
	if (ToolWarningArea.IsValid())
	{
		ToolWarningArea->SetText(FText());
		ToolWarningArea->SetVisibility(EVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
