// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SSelectReferenceSkeletalMeshWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "MeshOperationsLibrary.h"
#include "ObjectTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "MeshMorpherToolkit.h"
#include "Widgets/Input/SSearchBox.h"
#include "Modules/ModuleManager.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SSelectReferenceSkeletalMeshWidget"

void SSelectReferenceSkeletalMeshWidget::Construct(const FArguments& InArgs)
{
	Toolkit = InArgs._Toolkit;
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
	TSharedPtr<FMeshMorpherDetailRootObjectCustomization> RootObjectCustomization
		= MakeShared<FMeshMorpherDetailRootObjectCustomization>();
	DetailsPanel->SetRootObjectCustomizationInstance(RootObjectCustomization);
	Config = NewObject<USelectReferenceSkeletalMeshProperties>(Toolkit->ReferenceSkeletalMeshComponent->GetWorld(), NAME_None, RF_Transient);
	check(Config);
	ReferencedObjects.Add(Config);
	DetailsPanel->SetObject(Config);
	Config->Widget = this;
	if (Toolkit.IsValid())
	{
		const bool bHasMasterPose = Toolkit->ReferenceSkeletalMeshComponent->MasterPoseComponent.IsValid() || Toolkit->PoseSkeletalMeshComponent->MasterPoseComponent.IsValid();
		Config->bDisableMasterPose = !bHasMasterPose;
		if(bHasMasterPose)
		{
			if(Toolkit->PoseSkeletalMeshComponent->MasterPoseComponent.IsValid())
			{
				if(Toolkit->PoseSkeletalMeshComponent->MasterPoseComponent.Get() == Toolkit->ReferenceSkeletalMeshComponent)
				{
					Config->bInvertMasterPose = true;
				}
			}
		}
		if (Toolkit->ReferenceSkeletalMeshComponent)
		{
			SkeletalMeshComponent = Toolkit->ReferenceSkeletalMeshComponent;
			Config->ReferenceSkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
			Config->Transform = SkeletalMeshComponent->GetComponentTransform();
			Config->Materials = SkeletalMeshComponent->OverrideMaterials;
		}
	}

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
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SSplitter)
				.Orientation(EOrientation::Orient_Horizontal)
				+ SSplitter::Slot()
				[
					SNew(SBorder)
					.BorderImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.MainBackgroundColor"))
					.Padding(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetMargin("MeshMorpher.GenericMargin"))
					[
						SNew(SScrollBox)
						.Orientation(EOrientation::Orient_Vertical)
						+ SScrollBox::Slot()
						[
							DetailsPanel->AsShared()
						]
					]
				]
				+ SSplitter::Slot()
				[
					SNew(SBorder)
					.BorderImage(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetBrush("MeshMorpher.MainBackgroundColor"))
					.Padding(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetMargin("MeshMorpher.GenericMargin"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 2)
						[
							SNew(SSearchBox)
							.SelectAllTextWhenFocused(true)
							.OnTextChanged(this, &SSelectReferenceSkeletalMeshWidget::OnFilterTextChanged)
							.OnTextCommitted(this, &SSelectReferenceSkeletalMeshWidget::OnFilterTextCommitted)
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SAssignNew(MorphListView, SMeshMorpherMorphTargetListType)
							.ListItemsSource(&MorphTargetList)
							.ItemHeight(24)
							.SelectionMode(ESelectionMode::None)
							.OnGenerateRow_Raw(this, &SSelectReferenceSkeletalMeshWidget::OnGenerateRowForList)
							.HeaderRow
							(
								SNew( SHeaderRow )
								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetNameLabel )
								.DefaultLabel( LOCTEXT( "MorphTargetNameLabel", "Name" ) )

								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetWeightLabel )
								.DefaultLabel( LOCTEXT( "MorphTargetWeightLabel", "Weight" ) )

								+ SHeaderRow::Column(MeshMorpherBakeColumns::MorphTargetVertCountLabel )
								.DefaultLabel( LOCTEXT("MorphTargetVertCountLabel", "Vert Count") )
							)
						]
					]
				]
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
						.OnClicked_Raw(this, &SSelectReferenceSkeletalMeshWidget::Close)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Close")))
							.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
						]
					]
				]
			]
		]
	];
	
	InitializeMorphTargetList();
	CreateMorphTargetList(FilterText.ToString());
}

FReply SSelectReferenceSkeletalMeshWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void USelectReferenceSkeletalMeshProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FString Name = PropertyChangedEvent.MemberProperty->GetFName().ToString();
	if (Name.Equals("ReferenceSkeletalMesh"))
	{
		Materials.Empty();
		Transform = FTransform::Identity;
		if(Widget)
		{
			if (Widget->Toolkit.IsValid())
			{
				Widget->Toolkit->SetReferenceMesh(ReferenceSkeletalMesh);

				if(ReferenceSkeletalMesh)
				{
					if(!bDisableMasterPose)
					{
						if(bInvertMasterPose)
						{
							Widget->Toolkit->SetMasterPoseInv();
						} else
						{
							Widget->Toolkit->SetMasterPose();
						}
					}
				} else
				{
					Widget->Toolkit->SetMasterPoseNull();
				}
				
				Widget->SkeletalMeshComponent->SetForcedLOD(1);
				Materials = Widget->SkeletalMeshComponent->OverrideMaterials;
				Widget->Toolkit->SetReferenceMaterials(Materials);
				Widget->Toolkit->SetReferenceTransform();
			}
			Widget->InitializeMorphTargetList();
			Widget->CreateMorphTargetList(Widget->FilterText.ToString());
		}
	}
	if (Name.Equals("Materials"))
	{
		if(Widget && Widget->Toolkit.IsValid())
		{
			Widget->Toolkit->SetReferenceMaterials(Materials);
		}
	}
	if (Name.Equals("Transform"))
	{
		if(Widget && Widget->Toolkit.IsValid())
		{
			Widget->Toolkit->SetReferenceTransform(Transform);
		}
	}

	if (Name.Equals("bInvertMasterPose"))
	{
		if(Widget && Widget->SkeletalMeshComponent && Widget->SkeletalMeshComponent->SkeletalMesh && Widget->Toolkit.IsValid() && !bDisableMasterPose)
		{
			if(bInvertMasterPose)
			{
				Widget->Toolkit->SetMasterPoseInv();
			} else
			{
				Widget->Toolkit->SetMasterPose();
			}
		}
	}

	if (Name.Equals("bDisableMasterPose"))
	{
		if(Widget && Widget->SkeletalMeshComponent && Widget->SkeletalMeshComponent->SkeletalMesh && Widget->Toolkit.IsValid())
		{
			if(bDisableMasterPose)
			{
				Widget->Toolkit->SetMasterPoseNull();
			} else
			{
				if(bInvertMasterPose)
				{
					Widget->Toolkit->SetMasterPoseInv();
				} else
				{
					Widget->Toolkit->SetMasterPose();
				}
			}
		}
	}
}
