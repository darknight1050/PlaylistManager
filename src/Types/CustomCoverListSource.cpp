#include "Types/CustomCoverListSource.hpp"
#include "Main.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "UnityEngine/Resources.hpp"

DEFINE_TYPE(PlaylistManager, CoverImageTableCell);
DEFINE_TYPE(PlaylistManager, CustomCoverListSource);

using namespace PlaylistManager;
using namespace QuestUI;

// copied from questui
void CustomCoverListSource::ctor() {
    INVOKE_CTOR();
    expandCell = false;
    tableView = nullptr;
    reuseIdentifier = CSTR("PlaylistManagerCoverCell");
}

void CustomCoverListSource::dtor() {
    Finalize();
}

CoverImageTableCell* CustomCoverListSource::CellForIdx(HMUI::TableView* tableView, int idx) {
    LOG_INFO("cell requested");
    // check for available reusable cells
    CoverImageTableCell* reusableCell = reinterpret_cast<CoverImageTableCell*>(tableView->DequeueReusableCellForIdentifier(reuseIdentifier));
    if(!reusableCell) {
        // create a new cell
        auto cellObject = UnityEngine::GameObject::New_ctor(CSTR("CoverCellGameObject"));
        auto rectTransform = cellObject->AddComponent<UnityEngine::RectTransform*>();
        rectTransform->set_sizeDelta({15, 15});
        reusableCell = cellObject->AddComponent<CoverImageTableCell*>();
        reusableCell->init(sprites[idx]);
    } else {
        reusableCell->setCover(sprites[idx]);
    }
    return reusableCell;
}

float CustomCoverListSource::CellSize() {
    return 15;
}

int CustomCoverListSource::NumberOfCells() {
    return sprites.size();
}

void CustomCoverListSource::addSprites(std::vector<UnityEngine::Sprite*> newSprites) {
    sprites.reserve(sprites.size() + newSprites.size());
    sprites.insert(sprites.end(), newSprites.begin(), newSprites.end());
}

void CustomCoverListSource::replaceSprites(std::vector<UnityEngine::Sprite*> newSprites) {
    clearSprites();
    addSprites(newSprites);
}

void CustomCoverListSource::clearSprites() {
    sprites.clear();
}

UnityEngine::Sprite* CustomCoverListSource::getSprite(int index) {
    if(index < 0 || index >= sprites.size())
        return nullptr;
    return sprites[index];
}

void CoverImageTableCell::RefreshVisuals() {
    if(selected || highlighted) {
        selectedImage->set_color({0.5, 0.5, 0.5, (float)(highlighted ? 0.6 : 0.5)});
        selectedImage->get_gameObject()->set_active(true);
        hoverImage->get_gameObject()->set_active(highlighted);
    } else {
        selectedImage->get_gameObject()->set_active(false);
        hoverImage->get_gameObject()->set_active(false);
    }
}

void CoverImageTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType) {
    RefreshVisuals();
}

void CoverImageTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
    RefreshVisuals();
}

void CoverImageTableCell::init(UnityEngine::Sprite* coverSprite) {
    // get rounded sprite
    auto img = ArrayUtil::Last(UnityEngine::Resources::FindObjectsOfTypeAll<HMUI::ImageView*>(), [](HMUI::ImageView* x){
        auto sprite = x->get_sprite();
        return sprite && STR(sprite->get_name()) == "RoundRect10";
    });
    // create images
    selectedImage = BeatSaberUI::CreateImage(get_transform(), img->get_sprite(), {0, 0}, {15, 15});
    coverImage = BeatSaberUI::CreateImage(get_transform(), coverSprite, {0, 0}, {13, 13});
    hoverImage = BeatSaberUI::CreateImage(get_transform(), WhiteSprite(), {0, 0}, {13, 13});

    hoverImage->set_color({0.5, 0.5, 0.5, 0.6});
    selectedImage->get_gameObject()->set_active(false);
    hoverImage->get_gameObject()->set_active(false);
    selectedImage->set_type(UnityEngine::UI::Image::Type::Tiled);
}

void CoverImageTableCell::setCover(UnityEngine::Sprite* coverSprite) {
    coverImage->set_sprite(coverSprite);
}
