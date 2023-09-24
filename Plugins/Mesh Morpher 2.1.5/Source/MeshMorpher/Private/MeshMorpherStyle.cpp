// Copyright 2020-2022 SC Pug Life Studio S.R.L. All Rights Reserved.
#include "MeshMorpherStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"

TSharedPtr< FSlateStyleSet > FMeshMorpherStyle::Style = nullptr;
TSharedPtr< class ISlateStyle > FMeshMorpherStyle::Get() { return Style; }

void FMeshMorpherStyle::Shutdown()
{
	if (Style.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Style.Get());
		ensure(Style.IsUnique());
		Style.Reset();
	}
}

FName FMeshMorpherStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("MeshMorpherStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FMeshMorpherStyle::Initialize()
{
	if (Style.IsValid())
	{
		return;
	}

	Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MeshMorpher")->GetBaseDir() / TEXT("Resources"));


	Style->Set("MeshMorpher.OpenToolkitAction", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("MeshMorpher.OpenToolkitAction.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon20x20));

	Style->Set("MeshMorpher.OpenMorphButton", new IMAGE_BRUSH(TEXT("icon_editgeometry_40x"), Icon40x40));
	Style->Set("MeshMorpher.OpenMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_editgeometry_40x"), Icon40x40));

	Style->Set("MeshMorpher.DeselectMorphButton", new IMAGE_BRUSH(TEXT("Deselect_40x"), Icon40x40));
	Style->Set("MeshMorpher.DeselectMorphButton.Small", new IMAGE_BRUSH(TEXT("Deselect_40x"), Icon40x40));

	Style->Set("MeshMorpher.AddMorphButton", new IMAGE_BRUSH(TEXT("icon_AddMorph_40x"), Icon40x40));
	Style->Set("MeshMorpher.AddMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_AddMorph_40x"), Icon40x40));

	Style->Set("MeshMorpher.DeleteMorphButton", new IMAGE_BRUSH(TEXT("icon_RemoveMorph_40x"), Icon40x40));
	Style->Set("MeshMorpher.DeleteMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_RemoveMorph_40x"), Icon40x40));

	Style->Set("MeshMorpher.DuplicateMorphButton", new IMAGE_BRUSH(TEXT("icon_Edit_Duplicate_40x"), Icon40x40));
	Style->Set("MeshMorpher.DuplicateMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_Edit_Duplicate_40x"), Icon40x40));

	Style->Set("MeshMorpher.RenameMorphButton", new IMAGE_BRUSH(TEXT("icon_Edit_Rename_40x"), Icon40x40));
	Style->Set("MeshMorpher.RenameMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_Edit_Rename_40x"), Icon40x40));

	Style->Set("MeshMorpher.CopyMorphButton", new IMAGE_BRUSH(TEXT("icon_Edit_Copy_40x"), Icon40x40));
	Style->Set("MeshMorpher.CopyMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_Edit_Copy_40x"), Icon40x40));

	Style->Set("MeshMorpher.ApplyMorphTargetToLODsButton", new IMAGE_BRUSH(TEXT("icon_ViewMode_LODColoration_40px"), Icon40x40));
	Style->Set("MeshMorpher.ApplyMorphTargetToLODsButton.Small", new IMAGE_BRUSH(TEXT("icon_ViewMode_LODColoration_40px"), Icon40x40));

	Style->Set("MeshMorpher.MorphMagnitudeButton", new IMAGE_BRUSH(TEXT("icon_MatineeCurveView_40px"), Icon40x40));
	Style->Set("MeshMorpher.MorphMagnitudeButton.Small", new IMAGE_BRUSH(TEXT("icon_MatineeCurveView_40px"), Icon40x40));

	Style->Set("MeshMorpher.SettingsButton", new IMAGE_BRUSH(TEXT("icon_game_settings_40x"), Icon40x40));
	Style->Set("MeshMorpher.SettingsButton.Small", new IMAGE_BRUSH(TEXT("icon_game_settings_40x"), Icon40x40));

	Style->Set("MeshMorpher.SelectReferenceMeshButton", new IMAGE_BRUSH(TEXT("icon_Persona_ReferencePose_40x"), Icon40x40));
	Style->Set("MeshMorpher.SelectReferenceMeshButton.Small", new IMAGE_BRUSH(TEXT("icon_Persona_ReferencePose_40x"), Icon40x40));

	Style->Set("MeshMorpher.MergeMorphsButton", new IMAGE_BRUSH(TEXT("icon_merge_40x"), Icon40x40));
	Style->Set("MeshMorpher.MergeMorphsButton.Small", new IMAGE_BRUSH(TEXT("icon_merge_40x"), Icon40x40));

	Style->Set("MeshMorpher.ExportMorphButton", new IMAGE_BRUSH(TEXT("icon_Export_40x"), Icon40x40));
	Style->Set("MeshMorpher.ExportMorphButton.Small", new IMAGE_BRUSH(TEXT("icon_Export_40x"), Icon40x40));

	Style->Set("MeshMorpher.ExportOBJMenu", new IMAGE_BRUSH(TEXT("OBJExport"), Icon40x40));
	Style->Set("MeshMorpher.ExportOBJMenu.Small", new IMAGE_BRUSH(TEXT("OBJExport"), Icon40x40));

	Style->Set("MeshMorpher.ExportMorphTargetOBJMenuButton", new IMAGE_BRUSH(TEXT("OBJExport"), Icon40x40));
	Style->Set("MeshMorpher.ExportMorphTargetOBJMenuButton.Small", new IMAGE_BRUSH(TEXT("OBJExport"), Icon40x40));

	Style->Set("MeshMorpher.MaskSelectionMenu", new IMAGE_BRUSH(TEXT("icon_Landscape_Tool_Mask_40x"), Icon40x40));
	Style->Set("MeshMorpher.MaskSelectionMenu.Small", new IMAGE_BRUSH(TEXT("icon_Landscape_Tool_Mask_40x"), Icon40x40));

	Style->Set("MeshMorpher.BakeButton", new IMAGE_BRUSH(TEXT("icon_Bake_40x"), Icon40x40));
	Style->Set("MeshMorpher.BakeButton.Small", new IMAGE_BRUSH(TEXT("icon_Bake_40x"), Icon40x40));

	Style->Set("MeshMorpher.CreateMorphFromPoseButton", new IMAGE_BRUSH(TEXT("PoseAsset_64x"), Icon40x40));
	Style->Set("MeshMorpher.CreateMorphFromPoseButton.Small", new IMAGE_BRUSH(TEXT("PoseAsset_64x"), Icon40x40));

	Style->Set("MeshMorpher.CreateMorphFromMeshButton", new IMAGE_BRUSH(TEXT("ModelingMirror_x40"), Icon40x40));
	Style->Set("MeshMorpher.CreateMorphFromMeshButton.Small", new IMAGE_BRUSH(TEXT("ModelingMirror_x40"), Icon40x40));

	Style->Set("MeshMorpher.CreateMorphFromFBXButton", new IMAGE_BRUSH(TEXT("icon_Persona_ImportMesh_40x"), Icon40x40));
	Style->Set("MeshMorpher.CreateMorphFromFBXButton.Small", new IMAGE_BRUSH(TEXT("icon_Persona_ImportMesh_40x"), Icon40x40));

	Style->Set("MeshMorpher.CreateMetaMorphFromFBXButton", new IMAGE_BRUSH(TEXT("MetaExport"), Icon40x40));
	Style->Set("MeshMorpher.CreateMetaMorphFromFBXButton.Small", new IMAGE_BRUSH(TEXT("MetaExport"), Icon40x40));

	Style->Set("MeshMorpher.CreateMetaMorphFromMorphTargetButton", new IMAGE_BRUSH(TEXT("MetaExport"), Icon40x40));
	Style->Set("MeshMorpher.CreateMetaMorphFromMorphTargetButton.Small", new IMAGE_BRUSH(TEXT("MetaExport"), Icon40x40));

	Style->Set("MeshMorpher.CreateMorphFromMeta", new IMAGE_BRUSH(TEXT("MetaImport"), Icon40x40));
	Style->Set("MeshMorpher.CreateMorphFromMeta.Small", new IMAGE_BRUSH(TEXT("MetaImport"), Icon40x40));

	Style->Set("MeshMorpher.FocusCameraButton", new IMAGE_BRUSH(TEXT("CameraFocus"), Icon40x40));
	Style->Set("MeshMorpher.FocusCameraButton.Small", new IMAGE_BRUSH(TEXT("CameraFocus"), Icon40x40));

	Style->Set("MeshMorpher.SaveButton", new IMAGE_BRUSH(TEXT("icon_file_save_40x"), Icon40x40));
	Style->Set("MeshMorpher.SaveButton.Small", new IMAGE_BRUSH(TEXT("icon_file_save_40x"), Icon40x40));

	Style->Set("MeshMorpher.New", new IMAGE_BRUSH(TEXT("icon_New_40x"), Icon40x40));
	Style->Set("MeshMorpher.Exit", new IMAGE_BRUSH(TEXT("icon_file_exit_16px"), Icon16x16));
	Style->Set("MeshMorpher.About", new IMAGE_BRUSH(TEXT("icon_info_16x"), Icon16x16));

	Style->Set("MeshMorpher.ToolbarTab", new IMAGE_BRUSH(TEXT("icon_tab_Toolbars_16x"), Icon16x16));
	Style->Set("MeshMorpher.PoseTab", new IMAGE_BRUSH(TEXT("PoseAsset_64x"), Icon16x16));
	Style->Set("MeshMorpher.PreviewTab", new IMAGE_BRUSH(TEXT("icon_Persona_PreviewAnim_16x"), Icon16x16));
	Style->Set("MeshMorpher.ToolTab", new IMAGE_BRUSH(TEXT("Sculpt_40x"), Icon16x16));
	Style->Set("MeshMorpher.MorphTargetsTab", new IMAGE_BRUSH(TEXT("icon_Persona_Morph_Target_Previewer_16x"), Icon16x16));

	Style->Set("MeshMorpher.MainBackgroundColor", new FSlateColorBrush(FLinearColor::FromSRGBColor(FColor(62, 62, 62, 240))));
	Style->Set("MeshMorpher.TitleBarBackgroundColor", new FSlateColorBrush(FLinearColor::FromSRGBColor(FColor(48, 48, 48, 255))));
	Style->Set("MeshMorpher.ListBackgroundColor", new FSlateColorBrush(FLinearColor::FromSRGBColor(FColor(62, 62, 62, 30))));

	Style->Set("MeshMorpher.AboutTitle", FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 30));

	Style->Set("MeshMorpher.Header", FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
	Style->Set("MeshMorpher.OptionsLabel", FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10));
	Style->Set("MeshMorpher.ListItem", FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 11));

	Style->Set("MeshMorpher.GenericMargin", FMargin(5.0f, 5.0f, 5.0f, 5.0f));

	Style->Set("MeshMorpher.Twitter", new IMAGE_BRUSH(TEXT("Twitter_Icon"), Icon40x40));
	Style->Set("MeshMorpher.TwitterGrey", new IMAGE_BRUSH(TEXT("Twitter_Icon_Greyscale"), Icon40x40));
	Style->Set("MeshMorpher.Discord", new IMAGE_BRUSH(TEXT("Discord_Icon"), Icon40x40));
	Style->Set("MeshMorpher.DiscordGrey", new IMAGE_BRUSH(TEXT("Discord_Icon_Greyscale"), Icon40x40));

	FSlateStyleRegistry::RegisterSlateStyle(*Style.Get());
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
