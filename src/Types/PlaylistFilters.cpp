#include "Types/PlaylistFilters.hpp"
#include "Types/BPFolder.hpp"
#include "Types/FolderTableCell.hpp"
#include "Types/CoverTableCell.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"
#include "Main.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"

#include "GlobalNamespace/SharedCoroutineStarter.hpp"

#include "HMUI/TableView_ScrollPositionType.hpp"
#include "HMUI/ScrollView.hpp"

using namespace PlaylistManager;
using namespace QuestUI;

PlaylistFilters* PlaylistFilters::filtersInstance = nullptr;

UnityEngine::GameObject* createContainer(UnityEngine::Transform* parent) {
    auto go = UnityEngine::GameObject::New_ctor(CSTR("PlaylistManagerUIContainer"));
    go->get_transform()->SetParent(parent, false);
    go->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    return go;
}

custom_types::Helpers::Coroutine PlaylistFilters::initCoroutine() {
    // make canvas for display
    auto canvas = BeatSaberUI::CreateCanvas();
    auto cvsTrans = canvas->get_transform();
    cvsTrans->set_position({0, 1, 3});
    cvsTrans->set_eulerAngles({90, 0, 0});

    co_yield nullptr;

    #pragma region filterList
    // set up list
    filterList = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(cvsTrans, {60, 15}, [this](int cellIdx){
        folderSelectionState = cellIdx;
        RefreshPlaylists();
        if(cellIdx == 3) {
            SetFoldersFilters(false);
        }
    });
    filterList->setType(csTypeOf(PlaylistManager::CoverTableCell*));
    filterList->tableView->tableType = HMUI::TableView::TableType::Horizontal;

    // add list data
    filterList->addSprites({AllPacksSprite(), DefaultPacksSprite(), CustomPacksSprite(), PackFoldersSprite()});
    filterList->addTexts({"All level packs", "No custom playlists", "Only custom playlists", "Playlist folders"});
    filterList->tableView->ReloadData();
    filterList->tableView->SelectCellWithIdx(folderSelectionState, false);
    #pragma endregion

    co_yield nullptr;

    #pragma region folderMenu
    folderMenu = createContainer(cvsTrans);
    
    #pragma endregion

    co_yield nullptr;

    #pragma region folderMenuLists
    folderListContainer = createContainer(cvsTrans);
    // set up list
    folderList = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(folderListContainer->get_transform(), {0, -10}, {75, 30}, [this](int cellIdx){
        // set folder selected
        // show menu for folder
    });
    folderList->setType(csTypeOf(PlaylistManager::FolderTableCell*));
    folderList->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    folderList->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    ReloadFolders();
    // paging buttons
    auto left = BeatSaberUI::CreateUIButton(folderListContainer->get_transform(), "", "SettingsButton", {-40, 0}, {8, 8}, [this](){
        // get table view as questui table view
        auto tableView = CRASH_UNLESS(il2cpp_utils::GetFieldValue<QuestUI::TableView*>(reinterpret_cast<Il2CppObject*>(this->folderList), "tableView"));
        int idx = std::min((int)(tableView->get_contentTransform()->get_anchoredPosition().x / tableView->get_cellSize())*-1, tableView->get_numberOfCells() - 1);
        idx -= 5;
        idx = idx > 0 ? idx : 0;
        tableView->ScrollToCellWithIdx(idx, HMUI::TableView::ScrollPositionType::Beginning, true);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    auto right = BeatSaberUI::CreateUIButton(folderListContainer->get_transform(), "", "SettingsButton", {40, 0}, {8, 8}, [this](){
        // get table view as questui table view
        auto tableView = CRASH_UNLESS(il2cpp_utils::GetFieldValue<QuestUI::TableView*>(reinterpret_cast<Il2CppObject*>(this->folderList), "tableView"));
        int idx = std::min((int)(tableView->get_contentTransform()->get_anchoredPosition().x / tableView->get_cellSize())*-1, tableView->get_numberOfCells() - 1);
        idx += 5;
        int max = tableView->get_dataSource()->NumberOfCells();
        idx = idx < max ? idx : max - 1;
        tableView->ScrollToCellWithIdx(idx, HMUI::TableView::ScrollPositionType::Beginning, true);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());

    co_yield nullptr;

    playlistListContainer = createContainer(cvsTrans);
    // list for selecting playlists from folders
    playlistList = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(playlistListContainer->get_transform(), {60, 15}, [this](int cellIdx){
        // multi select somehow
    });
    playlistList->setType(csTypeOf(PlaylistManager::CoverTableCell*));
    playlistList->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    playlistList->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    reloadFolderPlaylists();
    // paging buttons
    left = BeatSaberUI::CreateUIButton(playlistListContainer->get_transform(), "", "SettingsButton", {-30, 0}, {8, 8}, [this](){
        // get table view as questui table view
        auto tableView = CRASH_UNLESS(il2cpp_utils::GetFieldValue<QuestUI::TableView*>(reinterpret_cast<Il2CppObject*>(this->playlistList), "tableView"));
        int idx = std::min((int)(tableView->get_contentTransform()->get_anchoredPosition().x / tableView->get_cellSize())*-1, tableView->get_numberOfCells() - 1);
        idx -= 4;
        idx = idx > 0 ? idx : 0;
        tableView->ScrollToCellWithIdx(idx, HMUI::TableView::ScrollPositionType::Beginning, true);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    right = BeatSaberUI::CreateUIButton(playlistListContainer->get_transform(), "", "SettingsButton", {30, 0}, {8, 8}, [this](){
        // get table view as questui table view
        auto tableView = CRASH_UNLESS(il2cpp_utils::GetFieldValue<QuestUI::TableView*>(reinterpret_cast<Il2CppObject*>(this->playlistList), "tableView"));
        int idx = std::min((int)(tableView->get_contentTransform()->get_anchoredPosition().x / tableView->get_cellSize())*-1, tableView->get_numberOfCells() - 1);
        idx += 4;
        int max = tableView->get_dataSource()->NumberOfCells();
        idx = idx < max ? idx : max - 1;
        tableView->ScrollToCellWithIdx(idx, HMUI::TableView::ScrollPositionType::Beginning, true);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());
    #pragma endregion

    SetFoldersFilters(true);

    co_return;
}

void PlaylistFilters::reloadFolderPlaylists() {
    if(!currentFolder || !playlistList) return;

    std::vector<UnityEngine::Sprite*> covers;
    std::vector<std::string> names;
    for(std::string& name : currentFolder->PlaylistNames) {
        names.emplace_back(name);
        covers.push_back(GetCoverImage(GetPlaylistJSON(name)));
    }
    playlistList->replaceSprites(covers);
    playlistList->replaceTexts(names);
    playlistList->tableView->ReloadData();
}

void PlaylistFilters::Init() {
    // start coroutine
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(
        reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(initCoroutine())));
}

void PlaylistFilters::SetFoldersFilters(bool filtersVisible) {
    filterList->get_transform()->GetParent()->get_gameObject()->SetActive(filtersVisible);
    folderList->get_transform()->GetParent()->get_gameObject()->SetActive(!filtersVisible);
    folderMenu->SetActive(!filtersVisible);
}

void PlaylistFilters::ReloadFolders() {
    if(!folderList) return;

    std::vector<std::string> folderNames;
    for(auto& folder : playlistConfig.Folders) {
        folderNames.push_back(folder.FolderName);
    }
    folderList->replaceTexts(folderNames);
    folderList->tableView->ReloadData();
}