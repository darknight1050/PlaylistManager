#include "Types/CustomListSource.hpp"
#include "Types/CoverTableCell.hpp"
#include "Main.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"

DEFINE_TYPE(PlaylistManager, CustomTableCell);
DEFINE_TYPE(PlaylistManager, CustomListSource);

using namespace PlaylistManager;
using namespace QuestUI;

// copied from questui
void CustomListSource::ctor() {
    INVOKE_CTOR();
    expandCell = false;
    tableView = nullptr;
    reuseIdentifier = CSTR("PlaylistManagerListCell");
}

void CustomListSource::dtor() {
    this->~CustomListSource();
    Finalize();
}

HMUI::TableCell* CustomListSource::CellForIdx(HMUI::TableView* tableView, int idx) {
    // kinda needed
    if(!type) {
        LOG_ERROR("Type not supplied to list source");
        return nullptr;
    }
    // check for available reusable cells
    CustomTableCell* reusableCell = reinterpret_cast<CustomTableCell*>(tableView->DequeueReusableCellForIdentifier(reuseIdentifier));
    if(!reusableCell) {
        // create a new cell
        auto cellObject = UnityEngine::GameObject::New_ctor(CSTR("CustomCellGameObject"));
        auto rectTransform = cellObject->AddComponent<UnityEngine::RectTransform*>();
        rectTransform->set_sizeDelta({15, 15});
        reusableCell = reinterpret_cast<CustomTableCell*>(cellObject->AddComponent(type));
        reusableCell->set_reuseIdentifier(reuseIdentifier);
        reusableCell->init(getSprite(idx), getText(idx));
    } else {
        reusableCell->setSprite(getSprite(idx));
        reusableCell->setText(getText(idx));
    }
    return reinterpret_cast<HMUI::TableCell*>(reusableCell);
}

float CustomListSource::CellSize() {
    return 15;
}

int CustomListSource::NumberOfCells() {
    return std::max(sprites.size(), texts.size());
}

void CustomListSource::setType(System::Type* cellType) {
    type = cellType;
}

void CustomListSource::addSprites(std::vector<UnityEngine::Sprite*> newSprites) {
    sprites.reserve(sprites.size() + newSprites.size());
    sprites.insert(sprites.end(), newSprites.begin(), newSprites.end());
}

void CustomListSource::replaceSprites(std::vector<UnityEngine::Sprite*> newSprites) {
    clearSprites();
    addSprites(newSprites);
}

void CustomListSource::clearSprites() {
    sprites.clear();
}

void CustomListSource::addTexts(std::vector<std::string> newTexts) {
    texts.reserve(texts.size() + newTexts.size());
    texts.insert(texts.end(), newTexts.begin(), newTexts.end());
}

void CustomListSource::replaceTexts(std::vector<std::string> newTexts) {
    clearTexts();
    addTexts(newTexts);
}

void CustomListSource::clearTexts() {
    texts.clear();
}

void CustomListSource::clear() {
    clearSprites();
    clearTexts();
}

UnityEngine::Sprite* CustomListSource::getSprite(int index) {
    if(index < 0 || index >= sprites.size())
        return nullptr;
    return sprites[index];
}

std::string CustomListSource::getText(int index) {
    if(index < 0 || index >= texts.size())
        return "";
    return texts[index];
}

void CustomTableCell::SelectionDidChange(HMUI::SelectableCell::TransitionType transitionType) {
    refreshVisuals();
}

void CustomTableCell::HighlightDidChange(HMUI::SelectableCell::TransitionType transitionType) {
    refreshVisuals();
}