// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SAboutWindow.h"
#include "SlateOptMacros.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "UnrealEdMisc.h"
#include "IDocumentation.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SAboutWindow"

void SAboutWindow::Construct(const FArguments& InArgs)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4428)	// universal-character-name encountered in source
#endif
	AboutLines.Add(MakeShareable(new FLineDefinition(LOCTEXT("Copyright1", "Copyright 2017-2022 SC Pug Life Studio S.R.L. All rights reserved"), 11, FLinearColor(1.f, 1.f, 1.f), FMargin(0.f) )));
	AboutLines.Add(MakeShareable(new FLineDefinition(LOCTEXT("Copyright2", "Mesh Morpher is an Unreal Engine plug-in that allows you to easily create and modify Morph Targets.\nThis plugin is distributed through the Marketplace and is licensed only under the Unreal Engine EULA."), 8, FLinearColor(1.f, 1.f, 1.f), FMargin(0.0f,2.0f) )));

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	auto Plugin = IPluginManager::Get().FindPlugin("MeshMorpher");
	check(Plugin.IsValid());
	const FText Version = FText::FromString(Plugin->GetDescriptor().VersionName);

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				.Padding(FMargin(10.f, 10.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Mesh Morpher")))
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.AboutTitle"))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Left)
				.Padding(FMargin(10.f, 35.f, 7.f, 0.f))
				[
					SNew(SEditableText)
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
					.IsReadOnly(true)
					.Text(Version)
				]
			]
			+SVerticalBox::Slot()
			.Padding(FMargin(5.f, 5.f, 5.f, 5.f))
			.VAlign(VAlign_Top)
			[
				SNew(SListView<TSharedRef<FLineDefinition>>)
				.ListItemsSource(&AboutLines)
				.OnGenerateRow(this, &SAboutWindow::MakeAboutTextItemWidget)
				.SelectionMode( ESelectionMode::None )
			] 
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding(FMargin(5.f, 0.f, 5.f, 5.f))
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ToolTipText(LOCTEXT("TwitterToolTip", "SC Pug Life Studio S.R.L on Twitter"))
					.OnClicked(this, &SAboutWindow::OnTwitterButtonClicked)
					.OnHovered_Raw(this, &SAboutWindow::OnTwitterButtonHovered)
					.OnUnhovered_Raw(this, &SAboutWindow::OnTwitterButtonUnhovered)
					[
						SAssignNew(TwitterButton, SImage)
						.Image(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.TwitterGrey"))
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding(FMargin(5.f, 0.f, 5.f, 5.f))
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ToolTipText(LOCTEXT("DiscordToolTip", "SC Pug Life Studio S.R.L on Discord"))
					.OnClicked(this, &SAboutWindow::OnDiscordButtonClicked)
					.OnHovered_Raw(this, &SAboutWindow::OnDiscordButtonHovered)
					.OnUnhovered_Raw(this, &SAboutWindow::OnDiscordButtonUnhovered)
					[
						SAssignNew(DiscordButton, SImage)
						.Image(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.DiscordGrey"))
					]
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(FMargin(5.f, 0.f, 5.f,5.f))
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("Close", "Close"))
					.ButtonColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
					.OnClicked(this, &SAboutWindow::OnClose)
				]
			]
		]
	];
}

TSharedRef<ITableRow> SAboutWindow::MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	if( Item->Text.IsEmpty() )
	{
		return
			SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			.Padding(6.0f)
			[
				SNew(SSpacer)
			];
	}
	else 
	{
		return
			SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			.Padding( Item->Margin )
			[
				SNew(STextBlock)
				.ColorAndOpacity( Item->TextColor )
				.Font( FCoreStyle::GetDefaultFontStyle("Regular", Item->FontSize) )
				.Text( Item->Text )
			];
	}
}

void SAboutWindow::OnTwitterButtonHovered()
{
	TwitterButton->SetImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.Twitter"));
}

void SAboutWindow::OnTwitterButtonUnhovered()
{
	TwitterButton->SetImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.TwitterGrey"));
}

void SAboutWindow::OnDiscordButtonHovered()
{
	DiscordButton->SetImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.Discord"));
}

void SAboutWindow::OnDiscordButtonUnhovered()
{
	DiscordButton->SetImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.DiscordGrey"));
}

FReply SAboutWindow::OnTwitterButtonClicked()
{
	FString TwitterURL = "https://twitter.com/PugLifeStudio";
	FPlatformProcess::LaunchURL(*TwitterURL, NULL, NULL);
	return FReply::Handled();
}

FReply SAboutWindow::OnDiscordButtonClicked()
{
	FString DiscordURL = "https://discord.gg/v4D42Vp";
	FPlatformProcess::LaunchURL(*DiscordURL, NULL, NULL);
	return FReply::Handled();
}

FReply SAboutWindow::OnClose()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow( AsShared() ).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow( ParentWindow );
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
