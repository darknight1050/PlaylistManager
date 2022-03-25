#include "Main.hpp"
#include "Types/GridViewAddon.hpp"
#include "Types/PlaylistMenu.hpp"
#include "ResettableStaticPtr.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsGridView.hpp"

using namespace PlaylistManager;
using namespace QuestUI;

GridViewAddon* GridViewAddon::addonInstance;

void GridViewAddon::createButtonPressed() {
    // select first playlist
    gridView->SelectAndScrollToCellWithIdx(0);
    // select header cell
    FindComponent<GlobalNamespace::LevelCollectionTableView*>()->SelectLevelPackHeaderCell();
    // open create menu
    PlaylistMenu::menuInstance->ShowCreateMenu();
}

void GridViewAddon::Init(GlobalNamespace::AnnotatedBeatmapLevelCollectionsViewController* parent) {
    gridView = parent->annotatedBeatmapLevelCollectionsGridView;

    static ConstString name("PlaylistManagerGridViewAddon");
    container = UnityEngine::GameObject::New_ctor(name);
    container->get_transform()->SetParent(parent->get_transform(), false);

    static ConstString contentName("Content");

    auto createButton = BeatSaberUI::CreateUIButton(container->get_transform(), "+", "ActionButton", {40, -2.5}, [this] {
        createButtonPressed();
    });
    UnityEngine::Object::Destroy(createButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    auto sizeFitter = createButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
}

void GridViewAddon::SetVisible(bool visible) {
    container->SetActive(visible);
}

void GridViewAddon::Destroy() {
    GridViewAddon::addonInstance = nullptr;
    UnityEngine::Object::Destroy(container);
    delete this;
}
