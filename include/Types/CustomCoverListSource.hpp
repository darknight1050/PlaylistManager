#pragma once

#include "custom-types/shared/macros.hpp"

#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Sprite.hpp"

#include "HMUI/TableView_IDataSource.hpp"
#include "HMUI/TableCell.hpp"
#include "HMUI/ImageView.hpp"

DECLARE_CLASS_CODEGEN(PlaylistManager, CoverImageTableCell, HMUI::TableCell,

    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, coverImage);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, hoverImage);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, selectedImage);

    DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::SelectionDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::HighlightDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_INSTANCE_METHOD(void, RefreshVisuals);

    public:
    void init(UnityEngine::Sprite* coverSprite);
    void setCover(UnityEngine::Sprite* coverSprite);
)

___DECLARE_TYPE_WRAPPER_INHERITANCE(PlaylistManager, CustomCoverListSource, Il2CppTypeEnum::IL2CPP_TYPE_CLASS, UnityEngine::MonoBehaviour, "PlaylistManager", { classof(HMUI::TableView::IDataSource*) }, 0, nullptr,
    
    DECLARE_INSTANCE_FIELD(Il2CppString*, reuseIdentifier);
    DECLARE_INSTANCE_FIELD(HMUI::TableView*, tableView);

    DECLARE_CTOR(ctor);
    DECLARE_DTOR(dtor);

    DECLARE_OVERRIDE_METHOD(CoverImageTableCell*, CellForIdx, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellForIdx>::get(), HMUI::TableView* tableView, int idx);

    DECLARE_OVERRIDE_METHOD(float, CellSize, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellSize>::get());
    DECLARE_OVERRIDE_METHOD(int, NumberOfCells, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::NumberOfCells>::get());

    private:
    std::vector<UnityEngine::Sprite*> sprites;
    bool expandCell;

    public:
    void addSprites(std::vector<UnityEngine::Sprite*> newSprites);
    void replaceSprites(std::vector<UnityEngine::Sprite*> newSprites);
    void clearSprites();
    UnityEngine::Sprite* getSprite(int index);
)