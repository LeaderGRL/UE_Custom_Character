#include "Widgets/SMeshMorpherMorphTargetListRow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Engine/RendererSettings.h"
#include "GPUSkinCache.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/MorphTarget.h"

#define LOCTEXT_NAMESPACE "SMeshMorpherMorphTargetListRow"
const float MaxMorphWeight = 1.f;

void SMeshMorpherMorphTargetListRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Item = InArgs._Item;
	MorphTargetViewer = InArgs._MorphTargetViewer;
	MorphTargetListView = InArgs._MorphTargetListView;

	check( Item.IsValid() );

	SMultiColumnTableRow< FMeshMorpherMorphTargetInfoPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > SMeshMorpherMorphTargetListRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	if ( ColumnName == MeshMorpherBakeColumns::MorphTargetNameLabel )
	{
		return
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 4.0f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromName(Item->Name) )
				.HighlightText_Raw(this, &SMeshMorpherMorphTargetListRow::GetFilterText )
			];
	}
	else if ( ColumnName == MeshMorpherBakeColumns::MorphTargetWeightLabel )
	{
		// Encase the SSpinbox in an SVertical box so we can apply padding. Setting ItemHeight on the containing SListView has no effect :-(
		return
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 1.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SSpinBox<float> )
				.MinSliderValue(-1.f)
				.MaxSliderValue(1.f)
				.MinValue(-MaxMorphWeight)
				.MaxValue(MaxMorphWeight)
				.Value( this, &SMeshMorpherMorphTargetListRow::GetWeight )
				.OnValueChanged( this, &SMeshMorpherMorphTargetListRow::OnMorphTargetWeightChanged )
				.OnValueCommitted( this, &SMeshMorpherMorphTargetListRow::OnMorphTargetWeightValueCommitted )
				.IsEnabled(this, &SMeshMorpherMorphTargetListRow::IsMorphTargetWeightSliderEnabled)
				.ToolTipText(this, &SMeshMorpherMorphTargetListRow::GetMorphTargetWeightSliderToolTip)
			];
	}
	else if (ColumnName == MeshMorpherBakeColumns::MorphTargetRemoveLabel)
	{
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SMeshMorpherMorphTargetListRow::OnMorphTargetRemoveChecked)
				.IsChecked(this, &SMeshMorpherMorphTargetListRow::IsMorphTargetRemoveChangedChecked)
			];
	}
	else
	{
		return
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.0f, 4.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(FText::AsNumber(Item->NumberOfVerts))
						.HighlightText(MorphTargetViewer->GetFilterText())
					]
				];
	}
}
FText SMeshMorpherMorphTargetListRow::GetFilterText() const
{ 
	if (MorphTargetViewer)
	{
		return MorphTargetViewer->GetFilterText();
	}
	else {
		return FText();
	}

}

void SMeshMorpherMorphTargetListRow::OnMorphTargetRemoveChecked(ECheckBoxState InState)
{
	Item->bRemove = InState == ECheckBoxState::Checked;
}

ECheckBoxState SMeshMorpherMorphTargetListRow::IsMorphTargetRemoveChangedChecked() const
{
	return (Item->bRemove) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SMeshMorpherMorphTargetListRow::OnMorphTargetWeightChanged( float NewWeight )
{
	Item->Weight = NewWeight;
	MorphTargetViewer->AddMorphTargetOverride( Item->Name, Item->Weight, false );
}

void SMeshMorpherMorphTargetListRow::OnMorphTargetWeightValueCommitted( float NewWeight, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
	{
		float NewValidWeight = FMath::Clamp(NewWeight, -MaxMorphWeight, MaxMorphWeight);
		Item->Weight = NewValidWeight;
		MorphTargetViewer->AddMorphTargetOverride(Item->Name, Item->Weight, false);
	}
}

bool SMeshMorpherMorphTargetListRow::IsMorphTargetWeightSliderEnabled() const
{
	const uint32 CVarMorphTargetModeValue = GetDefault<URendererSettings>()->bUseGPUMorphTargets;
	return GEnableGPUSkinCache > 0 ? CVarMorphTargetModeValue > 0 : true;
}

FText SMeshMorpherMorphTargetListRow::GetMorphTargetWeightSliderToolTip() const
{
	if (!IsMorphTargetWeightSliderEnabled())
	{
		return LOCTEXT("MorphTargetWeightSliderTooltip", "When using skin cache, the morph target must use the GPU to affect the mesh");
	}
	return FText();
}

float SMeshMorpherMorphTargetListRow::GetWeight() const 
{ 
	return Item->Weight;
}

TSharedRef<ITableRow> SMeshMorpherMorphTargetMainWidget::OnGenerateRowForList(TSharedPtr<FMeshMorpherMorphTargetInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(InInfo.IsValid());

	return
		SNew(SMeshMorpherMorphTargetListRow, OwnerTable)
		.Item(InInfo)
		.MorphTargetViewer(this)
		.MorphTargetListView(MorphListView);
}

void SMeshMorpherMorphTargetMainWidget::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;

	CreateMorphTargetList(SearchText.ToString());
}

void SMeshMorpherMorphTargetMainWidget::OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo)
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged(SearchText);
}


void SMeshMorpherMorphTargetMainWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	for(UObject* Object : ReferencedObjects)
	{
		Collector.AddReferencedObject(Object);
	}
}

void SMeshMorpherMorphTargetMainWidget::AddMorphTargetOverride(FName& Name, float Weight, bool bRemoveZeroWeight)
{
	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetMorphTarget(Name, Weight, bRemoveZeroWeight);
	}
}

void SMeshMorpherMorphTargetMainWidget::InitializeMorphTargetList()
{
	FullMorphTargetList.Empty();

	if (SkeletalMeshComponent && SkeletalMeshComponent->SkeletalMesh)
	{
		TArray<UMorphTarget*>& LocalMorphTargets = SkeletalMeshComponent->SkeletalMesh->GetMorphTargets();
		for (int32 I = 0; I < LocalMorphTargets.Num(); ++I)
		{
			const FName MorphTargetName = LocalMorphTargets[I]->GetFName();
			const float CurrentValue = SkeletalMeshComponent->GetMorphTarget(MorphTargetName);
			const int32 NumberOfVerts = (LocalMorphTargets[I]->GetMorphLODModels().Num() > 0) ? LocalMorphTargets[I]->GetMorphLODModels()[0].Vertices.Num() : 0;
			TSharedRef<FMeshMorpherMorphTargetInfo> Info = FMeshMorpherMorphTargetInfo::Make(MorphTargetName, CurrentValue, NumberOfVerts);
			FullMorphTargetList.Add(Info);
		}
	}
}

void SMeshMorpherMorphTargetMainWidget::CreateMorphTargetList(const FString& SearchText)
{
	MorphTargetList.Empty();

	const bool bDoFiltering = !SearchText.IsEmpty();

	for (int32 I = 0; I < FullMorphTargetList.Num(); ++I)
	{
		if (bDoFiltering && !FullMorphTargetList[I]->Name.ToString().Contains(SearchText))
		{
			continue; // Skip items that don't match our filter
		}
		MorphTargetList.Add(FullMorphTargetList[I]);
	}

	MorphListView->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE
