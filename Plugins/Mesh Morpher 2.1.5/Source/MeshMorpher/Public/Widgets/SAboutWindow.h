// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class ITableRow;
class SImage;
class STableViewBase;
struct FSlateBrush;

/**
 * About screen contents widget
 */
class SAboutWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAboutWindow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	struct FLineDefinition
	{
	public:
		FText Text;
		int32 FontSize;
		FLinearColor TextColor;
		FMargin Margin;

		FLineDefinition(const FText& InText)
			: Text(InText)
			, FontSize(9)
			, TextColor(FLinearColor(0.5f, 0.5f, 0.5f))
			, Margin(FMargin(6.f, 0.f, 0.f, 0.f))
		{

		}

		FLineDefinition(const FText& InText, int32 InFontSize, const FLinearColor& InTextColor, const FMargin& InMargin)
			: Text(InText)
			, FontSize(InFontSize)
			, TextColor(InTextColor)
			, Margin(InMargin)
		{

		}
	};

	TArray< TSharedRef< FLineDefinition > > AboutLines;
	TSharedPtr<SImage> TwitterButton;
	TSharedPtr<SImage> DiscordButton;

	/**
	 * Makes the widget for the checkbox items in the list view
	 */
	TSharedRef<ITableRow> MakeAboutTextItemWidget(TSharedRef<FLineDefinition> Item, const TSharedRef<STableViewBase>& OwnerTable);

	void OnTwitterButtonHovered();
	void OnTwitterButtonUnhovered();
	void OnDiscordButtonHovered();
	void OnDiscordButtonUnhovered();
	FReply OnTwitterButtonClicked();
	FReply OnDiscordButtonClicked();
	FReply OnClose();
};

