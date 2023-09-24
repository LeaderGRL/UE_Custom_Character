#pragma once
#include "CoreMinimal.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"

namespace MeshMorpherBakeColumns
{
	static const FName MorphTargetNameLabel("MorphTargetName");
	static const FName MorphTargetWeightLabel("Weight");
	static const FName MorphTargetRemoveLabel("Remove");
	static const FName MorphTargetVertCountLabel("NumberOfVerts");
}

class FMeshMorpherMorphTargetInfo
{
public:
	FName Name;
	float Weight;
	bool bRemove;
	int32 NumberOfVerts;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FMeshMorpherMorphTargetInfo> Make(const FName& Source, const int32 InNumberOfVerts)
	{
		return MakeShareable(new FMeshMorpherMorphTargetInfo(Source, InNumberOfVerts));
	}
	
	static TSharedRef<FMeshMorpherMorphTargetInfo> Make(const FName& Source, const float InWeight, const int32 InNumberOfVerts)
	{
		return MakeShareable(new FMeshMorpherMorphTargetInfo(Source, InWeight, InNumberOfVerts));
	}

protected:
	/** Hidden constructor, always use Make above */
	FMeshMorpherMorphTargetInfo(const FName& InSource, const int32 InNumberOfVerts)
		: Name(InSource)
		, Weight(0)
		, bRemove(false)
		, NumberOfVerts(InNumberOfVerts)
	{}

	FMeshMorpherMorphTargetInfo(const FName& InSource, const float InWeight, const int32 InNumberOfVerts)
	: Name(InSource)
	, Weight(InWeight)
	, bRemove(false)
	, NumberOfVerts(InNumberOfVerts)
	{}

	/** Hidden constructor, always use Make above */
	FMeshMorpherMorphTargetInfo() {}
};


typedef SListView< TSharedPtr<FMeshMorpherMorphTargetInfo> > SMeshMorpherMorphTargetListType;
typedef TSharedPtr< FMeshMorpherMorphTargetInfo > FMeshMorpherMorphTargetInfoPtr;
class USkeletalMeshComponent;

class SMeshMorpherMorphTargetMainWidget : public SCompoundWidget, public FGCObject
{
public:

	virtual FString GetReferencerName() const override
	{
		return "MeshMorpherMorphTargetMainWidget" + FString::FromInt(FMath::Rand());
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	
	virtual void AddMorphTargetOverride(FName& Name, float Weight, bool bRemoveZeroWeight);
	virtual FText& GetFilterText() { return FilterText; }
protected:
	virtual TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FMeshMorpherMorphTargetInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);
	virtual void OnFilterTextChanged(const FText& SearchText);
	virtual void OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo);
	virtual void InitializeMorphTargetList();
	virtual void CreateMorphTargetList(const FString& SearchText = FString());

protected:
	TSharedPtr<SMeshMorpherMorphTargetListType> MorphListView;
	TArray< TSharedPtr<FMeshMorpherMorphTargetInfo> > MorphTargetList;
	TArray< TSharedPtr<FMeshMorpherMorphTargetInfo> > FullMorphTargetList;
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	FText FilterText;
	TArray<UObject*> ReferencedObjects;
};

class SMeshMorpherMorphTargetListRow : public SMultiColumnTableRow< FMeshMorpherMorphTargetInfoPtr >
{
public:

	SLATE_BEGIN_ARGS(SMeshMorpherMorphTargetListRow) {}

	/** The item for this row **/
	SLATE_ARGUMENT(FMeshMorpherMorphTargetInfoPtr, Item )

		/* The SMorphTargetViewer that we push the morph target weights into */
		SLATE_ARGUMENT( class SMeshMorpherMorphTargetMainWidget*, MorphTargetViewer )

		/* Widget used to display the list of morph targets */
		SLATE_ARGUMENT( TSharedPtr<SMeshMorpherMorphTargetListType>, MorphTargetListView )

		SLATE_END_ARGS()

		void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView );

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

private:

	/**
	* Called when the user changes the value of the SSpinBox
	*
	* @param NewWeight - The new number the SSpinBox is set to
	*
	*/
	void OnMorphTargetWeightChanged( float NewWeight );
	
	/**
	* Called when the user types the value and enters
	*
	* @param NewWeight - The new number the SSpinBox is set to
	*
	*/
	void OnMorphTargetWeightValueCommitted( float NewWeight, ETextCommit::Type CommitType);
	
	/**
	* Called to know if we enable or disable the weight sliders
	*/
	bool IsMorphTargetWeightSliderEnabled() const;
	
	/**
	* Show the tooltip for the weight widget
	*/
	FText GetMorphTargetWeightSliderToolTip() const;

	/** Remove check call back functions */
	void OnMorphTargetRemoveChecked(ECheckBoxState InState);
	ECheckBoxState IsMorphTargetRemoveChangedChecked() const;
	
	/**
	* Returns the weight of this morph target
	*
	* @return SearchText - The new number the SSpinBox is set to
	*
	*/
	float GetWeight() const;
	FText GetFilterText() const;
	/* The SMorphTargetViewer that we push the morph target weights into */
	SMeshMorpherMorphTargetMainWidget* MorphTargetViewer;

	/** Widget used to display the list of morph targets */
	TSharedPtr<SMeshMorpherMorphTargetListType> MorphTargetListView;

	/** The name and weight of the morph target */
	FMeshMorpherMorphTargetInfoPtr	Item;


};
