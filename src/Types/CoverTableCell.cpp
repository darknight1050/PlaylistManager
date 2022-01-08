#include "Main.hpp"
#include "Types/CoverTableCell.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionCell.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"

using namespace PlaylistManager;
using namespace QuestUI;

DEFINE_TYPE(PlaylistManager, CoverTableCell);

// "polymorphism"
void CoverTableCell::ctor() {
    // run tableCell ctor
    auto thisref = this;
    static auto info = il2cpp_utils::FindMethodUnsafe(classof(HMUI::TableCell*), ".ctor", 0);
    il2cpp_utils::RunMethodRethrow(thisref, info);
    
    refreshVisualsFunc = [this](){ refreshVisuals(); };
    initFunc = [this](UnityEngine::Sprite* sprite, std::string text){ init(sprite, text); };
    setSpriteFunc = [this](UnityEngine::Sprite* sprite){ setSprite(sprite); };
    setTextFunc = [this](std::string text){ setText(text); };
}

void CoverTableCell::refreshVisuals() {
    if(selected || highlighted) {
        selectedImage->set_color1({1, 1, 1, (float)(highlighted ? 1 : 0)});
        selectedImage->get_gameObject()->set_active(true);
    } else {
        selectedImage->get_gameObject()->set_active(false);
    }
}

void CoverTableCell::init(UnityEngine::Sprite* sprite, std::string text) {
    // get rounded sprite
    using TableCellType = GlobalNamespace::AnnotatedBeatmapLevelCollectionCell;
    static auto cell = UnityEngine::Resources::FindObjectsOfTypeAll<TableCellType*>()[0];
    static auto roundedSprite = cell->selectionImage->get_sprite();
    // rounded corner material for the image
    static auto roundedCornerMaterial = cell->coverImage->get_material();
    // create images
    coverImage = BeatSaberUI::CreateImage(get_transform(), sprite, {0, 0}, {13, 13});
    coverImage->set_material(roundedCornerMaterial);
    selectedImage = BeatSaberUI::CreateImage(get_transform(), roundedSprite, {0, 0}, {20, 20});
    selectedImage->set_color({0, 0.753, 1, 1});
    selectedImage->set_color0({1, 1, 1, 1});
    selectedImage->gradient = true;
    selectedImage->get_gameObject()->set_active(false);
    // create hover text
    hoverHint = BeatSaberUI::AddHoverHint(get_gameObject(), text);
}

void CoverTableCell::setSprite(UnityEngine::Sprite* sprite) {
    coverImage->set_sprite(sprite);
}

void CoverTableCell::setText(std::string text) {
    hoverHint->set_text(CSTR(text));
}
