#pragma once

#include "custom-types/shared/macros.hpp"

#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Sprite.hpp"

#include "HMUI/TableView_IDataSource.hpp"
#include "HMUI/TableCell.hpp"

DECLARE_CLASS_CODEGEN(PlaylistManager, CustomTableCell, HMUI::TableCell,

    DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::SelectionDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
    DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::HighlightDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
    
    protected:
    std::function<void()> refreshVisualsFunc;
    std::function<void(UnityEngine::Sprite*, std::string)> initFunc;
    std::function<void(UnityEngine::Sprite*)> setSpriteFunc;
    std::function<void(std::string)> setTextFunc;

    public:
    void refreshVisuals() { refreshVisualsFunc(); }
    void init(UnityEngine::Sprite* sprite, std::string text) { initFunc(sprite, text); }
    void setSprite(UnityEngine::Sprite* sprite) { setSpriteFunc(sprite); }
    void setText(std::string text) { setTextFunc(text); }
)

___DECLARE_TYPE_WRAPPER_INHERITANCE(PlaylistManager, CustomListSource, Il2CppTypeEnum::IL2CPP_TYPE_CLASS, UnityEngine::MonoBehaviour, "PlaylistManager", { classof(HMUI::TableView::IDataSource*) }, 0, nullptr,
    
    DECLARE_INSTANCE_FIELD(Il2CppString*, reuseIdentifier);
    DECLARE_INSTANCE_FIELD(HMUI::TableView*, tableView);

    DECLARE_CTOR(ctor);
    DECLARE_DTOR(dtor);

    DECLARE_OVERRIDE_METHOD(HMUI::TableCell*, CellForIdx, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellForIdx>::get(), HMUI::TableView* tableView, int idx);

    DECLARE_OVERRIDE_METHOD(float, CellSize, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::CellSize>::get());
    DECLARE_OVERRIDE_METHOD(int, NumberOfCells, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::TableView::IDataSource::NumberOfCells>::get());

    private:
    std::vector<UnityEngine::Sprite*> sprites;
    std::vector<std::string> texts;
    bool expandCell;
    System::Type* type;

    public:
    void setType(System::Type* cellType);
    void addSprites(std::vector<UnityEngine::Sprite*> newSprites);
    void replaceSprites(std::vector<UnityEngine::Sprite*> newSprites);
    void clearSprites();
    void addTexts(std::vector<std::string> newTexts);
    void replaceTexts(std::vector<std::string> newTexts);
    void clearTexts();
    void clear();
    UnityEngine::Sprite* getSprite(int index);
    std::string getText(int index);
)