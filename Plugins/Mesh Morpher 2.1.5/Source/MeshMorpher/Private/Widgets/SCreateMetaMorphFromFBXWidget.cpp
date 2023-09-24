// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "Widgets/SCreateMetaMorphFromFBXWidget.h"
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
#include "Misc/MessageDialog.h"
#include "MeshMorpherToolkit.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Misc/FeedbackContext.h"
#include "Preview/SPreviewViewport.h"
#include "AdvancedPreviewScene.h"
#include "Async/ParallelFor.h"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#define LOCTEXT_NAMESPACE "SCreateMetaMorphFromFBXWidget"


void UCreateMetaMorphFromFBXProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{

	const double HalfRadiusSkel = 0.0;
	const double Offset = 20.0;
	
	if (PropertyChangedEvent.MemberProperty)
	{
		const FString Name = PropertyChangedEvent.MemberProperty->GetFName().ToString();

		//UE_LOG(LogTemp, Warning, TEXT("Property: %s"), *Name);
		
		if (Name.Equals("LockTransform"))
		{
			if (MorphedTransform.Rotation != BaseTransform.Rotation || MorphedTransform.Scale != BaseTransform.Scale ||  MorphedTransform.Position != BaseTransform.Position)
			{
				bApplyMorphedTransform = true;
			}

			MorphedTransform = BaseTransform;

			FQuaterniond QuatX;
			FQuaterniond QuatY;
			FQuaterniond QuatZ;
			const double X = FMath::Clamp(MorphedTransform.Rotation.X, -180.0, 180.0);
			const double Y = FMath::Clamp(MorphedTransform.Rotation.Y, -180.0, 180.0);
			const double Z = FMath::Clamp(MorphedTransform.Rotation.Z, -180.0, 180.0);

			QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
			QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
			QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

			MorphedTransform.Rotation = FVector(X, Y, Z);

			
			FVector Location = FVector((HalfRadiusSkel + MorphedComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);
			FTransformSRT3d Transform;
			Transform.SetRotation((QuatX * QuatY * QuatZ).Normalized());
			Transform.SetScale(MorphedTransform.Scale);
			Transform.SetTranslation(Location + BaseTransform.Position);
			MorphedComponent->SetWorldTransform(Transform);
		}

		if (Name.Equals("RenderMeshes"))
		{
			if (RenderMeshes)
			{
				BaseComponent->SetMesh(CopyTemp(BaseMesh));
				BaseComponent->UpdateBounds();
				MorphedComponent->SetMesh(CopyTemp(MorphedMesh));
				MorphedComponent->UpdateBounds();
			}
			else {
				BaseComponent->GetMesh()->Clear();
				MorphedComponent->GetMesh()->Clear();
			}
			BaseComponent->MarkRenderStateDirty();
			MorphedComponent->MarkRenderStateDirty();

			UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/MeshMorpher/ObjMaterial"));
			if (Material != nullptr)
			{
				BaseComponent->OverrideMaterials.Empty();
				BaseComponent->SetMaterial(0, Material);

				MorphedComponent->OverrideMaterials.Empty();
				MorphedComponent->SetMaterial(0, Material);
			}

		}

		if ((Name.Equals("BaseFile") || Name.Equals("BaseFrontCoordinateSystem") || Name.Equals("BaseFrontAxis") || Name.Equals("BaseUpAxis") || Name.Equals("bBaseUseT0")) && BaseComponent)
		{
			BaseMesh.Clear();
			if (FPaths::FileExists(BaseFile.FilePath))
			{
				if(bool bHasMenu = FSlateApplication::Get().AnyMenusVisible())
				{
					FSlateApplication::Get().DismissAllMenus();
				}
				
				GWarn->BeginSlowTask(FText::FromString("Loading Mesh file ..."), true);
				if (!GWarn->GetScopeStack().IsEmpty())
				{
					GWarn->GetScopeStack().Last()->MakeDialog(false, true);
				}
				GWarn->UpdateProgress(0, 1);
				GWarn->StatusForceUpdate(1, 1, FText::FromString("Importing Mesh File ..."));
				FMeshMorpherFBXImport FBXImport(BaseFile.FilePath, BaseFrontCoordinateSystem, BaseFrontAxis, BaseUpAxis, true, bBaseUseT0, FMath::Rand());
				BaseMesh = FBXImport.DynamicMesh;
				
				GWarn->EndSlowTask();
				if (RenderMeshes)
				{
					BaseComponent->SetMesh(CopyTemp(BaseMesh));
					BaseComponent->UpdateBounds();
				}
			}
			else {
				BaseComponent->SetMesh(CopyTemp(BaseMesh));
				BaseComponent->UpdateBounds();
			}

			BaseComponent->MarkRenderStateDirty();

			bApplyBaseTransform = true;

			UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/MeshMorpher/ObjMaterial"));
			if (Material != nullptr)
			{
				BaseComponent->OverrideMaterials.Empty();
				BaseComponent->SetMaterial(0, Material);
			}
			FVector Location = FVector(-(HalfRadiusSkel + BaseComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);

			FQuaterniond QuatX;
			FQuaterniond QuatY;
			FQuaterniond QuatZ;
			const double X = FMath::Clamp(BaseTransform.Rotation.X, -180.0, 180.0);
			const double Y = FMath::Clamp(BaseTransform.Rotation.Y, -180.0, 180.0);
			const double Z = FMath::Clamp(BaseTransform.Rotation.Z, -180.0, 180.0);

			QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
			QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
			QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

			FTransformSRT3d Transform;
			Transform.SetRotation((QuatX * QuatY * QuatZ).Normalized());
			Transform.SetScale(BaseTransform.Scale);
			Transform.SetTranslation(Location + BaseTransform.Position);
			BaseComponent->SetWorldTransform(Transform);
			
		}
		if ((Name.Equals("MorphedFile") || Name.Equals("MorphedFrontCoordinateSystem") || Name.Equals("MorphedFrontAxis") || Name.Equals("MorphedUpAxis") || Name.Equals("bMorphedUseT0")) && BaseComponent)
		{
			MorphedMesh.Clear();
			if (FPaths::FileExists(MorphedFile.FilePath))
			{
				if(bool bHasMenu = FSlateApplication::Get().AnyMenusVisible())
				{
					FSlateApplication::Get().DismissAllMenus();
				}
				GWarn->BeginSlowTask(FText::FromString("Loading Mesh File ..."), true);
				if (!GWarn->GetScopeStack().IsEmpty())
				{
					GWarn->GetScopeStack().Last()->MakeDialog(false, true);
				}
				GWarn->UpdateProgress(0, 1);
				GWarn->StatusForceUpdate(1, 1, FText::FromString("Importing Mesh File ..."));
				FMeshMorpherFBXImport FBXImport(MorphedFile.FilePath, MorphedFrontCoordinateSystem, MorphedFrontAxis, MorphedUpAxis, true, bMorphedUseT0, FMath::Rand());
				MorphedMesh = FBXImport.DynamicMesh;
				
				GWarn->EndSlowTask();
				if (RenderMeshes)
				{
					MorphedComponent->SetMesh(CopyTemp(MorphedMesh));
					MorphedComponent->UpdateBounds();
				}
			}
			else {
				MorphedComponent->SetMesh(CopyTemp(MorphedMesh));
				MorphedComponent->UpdateBounds();
			}

			MorphedComponent->MarkRenderStateDirty();

			bApplyMorphedTransform = true;

			UMaterialInterface* Material = LoadObject<UMaterial>(nullptr, TEXT("/MeshMorpher/ObjMaterial"));
			if (Material != nullptr)
			{
				MorphedComponent->OverrideMaterials.Empty();
				MorphedComponent->SetMaterial(0, Material);
			}
			
			FVector Location = FVector((HalfRadiusSkel + MorphedComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);

			FQuaterniond QuatX;
			FQuaterniond QuatY;
			FQuaterniond QuatZ;
			const double X = FMath::Clamp(MorphedTransform.Rotation.X, -180.0, 180.0);
			const double Y = FMath::Clamp(MorphedTransform.Rotation.Y, -180.0, 180.0);
			const double Z = FMath::Clamp(MorphedTransform.Rotation.Z, -180.0, 180.0);

			QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
			QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
			QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

			FTransformSRT3d Transform;
			Transform.SetRotation((QuatX * QuatY * QuatZ).Normalized());
			Transform.SetScale(MorphedTransform.Scale);
			Transform.SetTranslation(Location + MorphedTransform.Position);
			MorphedComponent->SetWorldTransform(Transform);

		}

		if (Name.Equals("BaseTransform") && BaseComponent)
		{
			{
				FQuaterniond QuatX;
				FQuaterniond QuatY;
				FQuaterniond QuatZ;
				const double X = FMath::Clamp(BaseTransform.Rotation.X, -180.0, 180.0);
				const double Y = FMath::Clamp(BaseTransform.Rotation.Y, -180.0, 180.0);
				const double Z = FMath::Clamp(BaseTransform.Rotation.Z, -180.0, 180.0);

				QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
				QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
				QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

				BaseTransform.Rotation = FVector(X, Y, Z);


				FVector Location = FVector(-(HalfRadiusSkel + BaseComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);
				FTransformSRT3d Transform;
				Transform.SetRotation((QuatX* QuatY* QuatZ).Normalized());
				Transform.SetScale(BaseTransform.Scale);
				Transform.SetTranslation(Location + BaseTransform.Position);
				BaseComponent->SetWorldTransform(Transform);

				bApplyBaseTransform = true;
			}

			if (LockTransform)
			{
				if (MorphedTransform.Rotation != BaseTransform.Rotation || MorphedTransform.Scale != BaseTransform.Scale || MorphedTransform.Position != BaseTransform.Position)
				{
					bApplyMorphedTransform = true;
				}

				MorphedTransform = BaseTransform;

				FQuaterniond QuatX;
				FQuaterniond QuatY;
				FQuaterniond QuatZ;
				const double X = FMath::Clamp(MorphedTransform.Rotation.X, -180.0, 180.0);
				const double Y = FMath::Clamp(MorphedTransform.Rotation.Y, -180.0, 180.0);
				const double Z = FMath::Clamp(MorphedTransform.Rotation.Z, -180.0, 180.0);

				QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
				QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
				QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

				MorphedTransform.Rotation = FVector(X, Y, Z);

				FVector Location = FVector((HalfRadiusSkel + MorphedComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);
				FTransformSRT3d Transform;
				Transform.SetRotation((QuatX* QuatY* QuatZ).Normalized());
				Transform.SetScale(MorphedTransform.Scale);
				Transform.SetTranslation(Location + BaseTransform.Position);
				MorphedComponent->SetWorldTransform(Transform);
			}
		}

		if (Name.Equals("MorphedTransform") && MorphedComponent)
		{
			if (!LockTransform)
			{
				bApplyMorphedTransform = true;

				FQuaterniond QuatX;
				FQuaterniond QuatY;
				FQuaterniond QuatZ;
				const double X = FMath::Clamp(MorphedTransform.Rotation.X, -180.0, 180.0);
				const double Y = FMath::Clamp(MorphedTransform.Rotation.Y, -180.0, 180.0);
				const double Z = FMath::Clamp(MorphedTransform.Rotation.Z, -180.0, 180.0);

				QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
				QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
				QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

				MorphedTransform.Rotation = FVector(X, Y, Z);

				FVector Location = FVector((HalfRadiusSkel + MorphedComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);
				FTransformSRT3d Transform;
				Transform.SetRotation((QuatX* QuatY* QuatZ).Normalized());
				Transform.SetScale(MorphedTransform.Scale);
				Transform.SetTranslation(Location + MorphedTransform.Position);
				MorphedComponent->SetWorldTransform(Transform);
			}
			else {

				if (MorphedTransform.Rotation != BaseTransform.Rotation || MorphedTransform.Scale != BaseTransform.Scale)
				{
					bApplyMorphedTransform = true;
				}

				MorphedTransform = BaseTransform;

				FQuaterniond QuatX;
				FQuaterniond QuatY;
				FQuaterniond QuatZ;
				const double X = FMath::Clamp(MorphedTransform.Rotation.X, -180.0, 180.0);
				const double Y = FMath::Clamp(MorphedTransform.Rotation.Y, -180.0, 180.0);
				const double Z = FMath::Clamp(MorphedTransform.Rotation.Z, -180.0, 180.0);

				QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
				QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
				QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

				MorphedTransform.Rotation = FVector(X, Y, Z);

				FVector Location = FVector((HalfRadiusSkel + MorphedComponent->Bounds.SphereRadius / 2 + Offset), 0.0, 0.0);
				FTransformSRT3d Transform;
				Transform.SetRotation((QuatX* QuatY* QuatZ).Normalized());
				Transform.SetScale(MorphedTransform.Scale);
				Transform.SetTranslation(Location + BaseTransform.Position);
				MorphedComponent->SetWorldTransform(Transform);
			}
		}

	}
}

void SCreateMetaMorphFromFBXWidget::Construct(const FArguments& InArgs)
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
	// add customization that hides header labels
	TSharedPtr<FMeshMorpherDetailRootObjectCustomization> RootObjectCustomization
		= MakeShared<FMeshMorpherDetailRootObjectCustomization>();
	DetailsPanel->SetRootObjectCustomizationInstance(RootObjectCustomization);

	TSharedPtr<class SMeshMorpherPreviewViewport> PreviewViewport;
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
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Fill)
						.AutoHeight()
						.Padding(10, 10, 10, 15)
						[
							SAssignNew(ToolMessageArea, STextBlock)
							.AutoWrapText(true)
							.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
							.ColorAndOpacity(FSlateColor(FLinearColor::Red * 0.7f))
							.Text_Raw(this, &SCreateMetaMorphFromFBXWidget::GetMessage)
						]
						+ SVerticalBox::Slot()
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
						SAssignNew(PreviewViewport, SMeshMorpherPreviewViewport)
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
						.OnClicked_Raw(this, &SCreateMetaMorphFromFBXWidget::Close)
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
						.OnClicked_Raw(this, &SCreateMetaMorphFromFBXWidget::OnCreateMorphTarget)
						.IsEnabled_Raw(this, &SCreateMetaMorphFromFBXWidget::IsCreateMorphValid)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Ok")))
							.Font(FSlateStyleRegistry::FindSlateStyle("MeshMorpherStyle")->GetFontStyle("MeshMorpher.Header"))
						]
					]
				]
			]
		]
	];

	Config = NewObject<UCreateMetaMorphFromFBXProperties>(PreviewViewport->PreviewScene->GetWorld(), NAME_None, RF_Transient);
	Config->Initialize();
	DetailsPanel->SetObject(Config);


	BaseComponent = NewObject<UDynamicMeshComponent>(PreviewViewport->PreviewScene->GetWorld(), NAME_None, RF_Transient);
	PreviewViewport->AddComponent(BaseComponent);
	Config->BaseComponent = BaseComponent;


	MorphedComponent = NewObject<UDynamicMeshComponent>(PreviewViewport->PreviewScene->GetWorld(), NAME_None, RF_Transient);
	PreviewViewport->AddComponent(MorphedComponent);
	Config->MorphedComponent = MorphedComponent;

}

FText SCreateMetaMorphFromFBXWidget::GetMessage() const
{
	bool bPickFBXFiles = false;

	if (!FPaths::FileExists(Config->BaseFile.FilePath) || !FPaths::FileExists(Config->MorphedFile.FilePath))
	{
		bPickFBXFiles = true;
	}

	if (!bPickFBXFiles && Config->BaseFile.FilePath.Equals(Config->MorphedFile.FilePath))
	{
		return FText::FromString("You can't use same file for Base Mesh and Morphed Mesh.");
	}

	if (BaseComponent && MorphedComponent && !bPickFBXFiles)
	{
		if (BaseComponent->GetMesh() && MorphedComponent->GetMesh())
		{
			if (BaseComponent->GetMesh()->VertexCount() != MorphedComponent->GetMesh()->VertexCount())
			{
				return FText::FromString("Base Mesh has different Vertex Count than Morphed Mesh.");
			}
		}
		else {
			bPickFBXFiles = true;
		}
	}
	else {
		bPickFBXFiles = true;
	}

	if (bPickFBXFiles)
	{
		return FText::FromString("Please select a Base Mesh file and a Morphed Mesh file.");
	}

	return FText::FromString("");
}

bool SCreateMetaMorphFromFBXWidget::IsCreateMorphValid() const
{
	return true;
}

FReply SCreateMetaMorphFromFBXWidget::Close()
{
	TSharedRef<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();
	FSlateApplication::Get().RequestDestroyWindow(ParentWindow);
	return FReply::Handled();
}

FReply SCreateMetaMorphFromFBXWidget::OnCreateMorphTarget()
{
	FText Error = GetMessage();
	if (!Error.IsEmpty())
	{
		FText Title = FText::FromString("Error!");
		FMessageDialog::Open(EAppMsgType::Ok, Error, &Title);
		return FReply::Handled();
	}

	FTransformSRT3d BaseTransform;
	{
		FQuaterniond QuatX;
		FQuaterniond QuatY;
		FQuaterniond QuatZ;
		const double X = FMath::Clamp(Config->BaseTransform.Rotation.X, -180.0, 180.0);
		const double Y = FMath::Clamp(Config->BaseTransform.Rotation.Y, -180.0, 180.0);
		const double Z = FMath::Clamp(Config->BaseTransform.Rotation.Z, -180.0, 180.0);

		QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
		QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
		QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

		BaseTransform.SetRotation((QuatX * QuatY * QuatZ).Normalized());
		BaseTransform.SetScale(Config->BaseTransform.Scale);
		BaseTransform.SetTranslation(Config->BaseTransform.Position);
	}

	FTransformSRT3d MorphedTransform;
	{
		FQuaterniond QuatX;
		FQuaterniond QuatY;
		FQuaterniond QuatZ;
		const double X = FMath::Clamp(Config->MorphedTransform.Rotation.X, -180.0, 180.0);
		const double Y = FMath::Clamp(Config->MorphedTransform.Rotation.Y, -180.0, 180.0);
		const double Z = FMath::Clamp(Config->MorphedTransform.Rotation.Z, -180.0, 180.0);

		QuatX.SetAxisAngleD(FVector3d(1, 0, 0), X);
		QuatY.SetAxisAngleD(FVector3d(0, 1, 0), Y);
		QuatZ.SetAxisAngleD(FVector3d(0, 0, 1), Z);

		MorphedTransform.SetRotation((QuatX * QuatY * QuatZ).Normalized());
		MorphedTransform.SetScale(Config->MorphedTransform.Scale);
		MorphedTransform.SetTranslation(Config->MorphedTransform.Position);
	}

	if (Config->bApplyMorphedTransform || Config->bApplyBaseTransform)
	{
		const int32 Count = Config->BaseMesh.VertexCount();
		if(Count > 0)
		{
			const int32 Cores = Count > FPlatformMisc::NumberOfCoresIncludingHyperthreads() ? FPlatformMisc::NumberOfCoresIncludingHyperthreads() : 1;
			const int32 ChunkSize = FMath::FloorToInt((static_cast<double>(Count) / static_cast<double>(Cores)));
			const int32 LastChunkSize = Count - (ChunkSize * Cores);
			const int32 Chunks = LastChunkSize > 0 ? Cores + 1 : Cores;

			ParallelFor(Chunks, [&](const int32 ChunkIndex)
			{
				const int32 IterationSize = ((LastChunkSize > 0) && (ChunkIndex == Chunks - 1)) ? LastChunkSize : ChunkSize;
				for (int X = 0; X < IterationSize; ++X)
				{
					const int32 Index = (ChunkIndex * ChunkSize) + X;
					if (Config->BaseMesh.IsVertex(Index) && Config->bApplyBaseTransform)
					{
						FVector Position = Config->BaseMesh.GetVertex(Index);
						Position = BaseTransform.TransformPosition(Position);
						Config->BaseMesh.SetVertex(Index, Position, false);
					}

					if (Config->MorphedMesh.IsVertex(Index) && Config->bApplyMorphedTransform)
					{
						FVector Position = Config->MorphedMesh.GetVertex(Index);
						Position = MorphedTransform.TransformPosition(Position);
						Config->MorphedMesh.SetVertex(Index, Position, false);
					}
				}
			});
		}

		if (Config->bApplyMorphedTransform)
		{
			Config->bApplyMorphedTransform = false;
		}

		if (Config->bApplyBaseTransform)
		{
			Config->bApplyBaseTransform = false;
		}
	}


	FString Name = FPaths::GetBaseFilename(Config->MorphedFile.FilePath);

	const bool bResult = UMeshOperationsLibrary::CreateMetaMorphAssetFromDynamicMeshes(Config->BaseMesh, Config->MorphedMesh, Name, Config->IgnoreMasks, Config->MoveMasks);

	return FReply::Handled();
}

void SCreateMetaMorphFromFBXWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(BaseComponent);
	Collector.AddReferencedObject(MorphedComponent);
	Collector.AddReferencedObject(Config);
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


