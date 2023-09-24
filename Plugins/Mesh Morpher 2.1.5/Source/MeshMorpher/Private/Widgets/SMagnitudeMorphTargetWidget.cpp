// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SMagnitudeMorphTargetWidget.h"

#include "SlateOptMacros.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "MeshOperationsLibrary.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SMagnitudeMorphTargetWidget"

void SMagnitudeMorphTargetWidget::Construct(const FArguments& InArgs)
{
	Source = InArgs._Source;
	Selection = InArgs._Selection;
	Toolkit = InArgs._Toolkit;
	check(Source);
	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.MainBackgroundColor"))
			.Padding(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetMargin("MeshMorpher.GenericMargin"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Morph Target Name:")))
					.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.OptionsLabel"))
				]
				+ SVerticalBox::Slot()
				.Padding(0.f, 5.f)
				.AutoHeight()
				[
					SAssignNew(NumericBox, SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinValue(-100).MaxValue(100)
					.MinSliderValue(-10).MaxSliderValue(10)
					.Value_Raw(this, &SMagnitudeMorphTargetWidget::GetMagnitudeValue)
					.OnValueChanged_Raw(this, &SMagnitudeMorphTargetWidget::SetMagnitudeValue)
				]
				+ SVerticalBox::Slot()
				.HAlign(EHorizontalAlignment::HAlign_Right)
				.Padding(0.f, 20.f, 0.f, 0.f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.WidthOverride(90.f)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Raw(this, &SMagnitudeMorphTargetWidget::Close)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
								.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
							]
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.WidthOverride(70.f)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Raw(this, &SMagnitudeMorphTargetWidget::OnUpdateMagnitudeMorphTarget)
							.IsEnabled_Raw(this, &SMagnitudeMorphTargetWidget::IsUpdateMorphValid)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Ok")))
								.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
							]
						]
					]
				]
			]
		]
	];
}

TOptional<float> SMagnitudeMorphTargetWidget::GetMagnitudeValue() const
{
	return Value;
}

void SMagnitudeMorphTargetWidget::SetMagnitudeValue(float NewValue)
{
	Value = NewValue;
}

bool SMagnitudeMorphTargetWidget::IsUpdateMorphValid() const
{
	if (Source && Selection.Num() > 0)
	{
		return true;
	}
	return false;
}

FReply SMagnitudeMorphTargetWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SMagnitudeMorphTargetWidget::OnUpdateMagnitudeMorphTarget()
{
	if (IsUpdateMorphValid())
	{
		TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
		FFormatNamedArguments Args;
		Args.Add(TEXT("SkeletalMesh"), FText::FromString(Source->GetName()));
		const FText StatusUpdate = FText::Format(LOCTEXT("UpdateMagnitudeMorphTargets", "({SkeletalMesh}) Updating Magnitude for Morph Target(s)..."), Args);
		GWarn->BeginSlowTask(StatusUpdate, true);
		if (!GWarn->GetScopeStack().IsEmpty())
		{
			GWarn->GetScopeStack().Last()->MakeDialog(false, true);
		}
		int32 Index = 0;

		bool bNeedsRebuild = false;

		for (auto& MorphTarget : Selection)
		{
			if (MorphTarget.IsValid())
			{
				FFormatNamedArguments MorphArgs;
				MorphArgs.Add(TEXT("MorphTarget"), FText::FromString(*MorphTarget));
				const FText MorphStatusUpdate = FText::Format(LOCTEXT("UpdateMagnitudeMorphTarget", "Updating Magnitude for Morph Target ({MorphTarget})..."), MorphArgs);
				GWarn->StatusForceUpdate(Index, Selection.Num(), MorphStatusUpdate);

				TArray<FMorphTargetDelta> Deltas;
				const bool bIsSuccess = UMeshOperationsLibrary::SetMorphTargetMagnitude(Source, *MorphTarget, Value, Deltas);
				if (bIsSuccess)
				{
					UMeshOperationsLibrary::ApplyMorphTargetToImportData(Source, *MorphTarget, Deltas, false);
					bNeedsRebuild = true;
				}
			}
			Index++;
		}

		if (bNeedsRebuild && Source)
		{

			if (Toolkit.IsValid())
			{
				Toolkit->RefreshMorphList();
			}

			Source->InitMorphTargetsAndRebuildRenderData();
		}

		GWarn->EndSlowTask();

		UMeshOperationsLibrary::NotifyMessage(FString("Finished updating magnitude for selected Morph Target(s)."));
		ParentWindow->BringToFront();

		return Close();

	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
