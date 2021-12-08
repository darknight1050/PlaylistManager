#include "Types/LevelButtons.hpp"
#include "Types/PlaylistMenu.hpp"
#include "Types/CustomListSource.hpp"
#include "Types/CoverTableCell.hpp"
#include "PlaylistManager.hpp"
#include "Icons.hpp"
#include "Main.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"

#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"

#include "UnityEngine/Resources.hpp"

#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

using namespace PlaylistManager;
using namespace QuestUI;

ButtonsContainer* ButtonsContainer::buttonsInstance = nullptr;

custom_types::Helpers::Coroutine ButtonsContainer::initCoroutine() {
    auto levelDetailTransform = levelDetailView->get_transform();

    #pragma region buttons
    auto horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(levelDetailTransform);
    horizontal->set_childScaleWidth(false);
    horizontal->set_childControlWidth(false);
    horizontal->set_childForceExpandWidth(false);
    horizontal->set_spacing(1);
    horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleRight);
    horizontal->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({-1, 17});

    saveCoverButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {4, 4}, [this](){
        LOG_INFO("save cover pressed");
        // get cover image (blocking async, yay)
        auto levelData = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(levelDetailView->level);
        auto sprite = levelData->GetCoverImageAsync(System::Threading::CancellationToken::get_None())->get_Result();
        // save to file
        WriteImageToFile(GetCoversPath() + "/" + STR(levelData->get_songName()) + "_cover.png", sprite);
        // reload from file
        if(PlaylistMenu::menuInstance)
            PlaylistMenu::menuInstance->RefreshCovers();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(saveCoverButton->get_transform()->GetChild(0))->set_sizeDelta({4, 4});
    saveCoverButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(saveCoverButton, SaveCoverInactiveSprite(), SaveCoverSprite());

    playlistAddButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {4, 4}, [this](){
        playlistAddModal->Show(true, false, nullptr);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(playlistAddButton->get_transform()->GetChild(0))->set_sizeDelta({4, 4});
    playlistAddButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(playlistAddButton, AddToPlaylistInactiveSprite(), AddToPlaylistSprite());

    playlistRemoveButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {4, 4}, [this](){
        // remove song from playlist
        auto json = GetPlaylistJSON(STR(currentPack->get_packName()));
        auto levelArr = currentPack->customBeatmapLevelCollection->customPreviewBeatmapLevels;
        // using a list because arrays are hell
        using LevelType = GlobalNamespace::CustomPreviewBeatmapLevel*;
        auto newLevels = List<LevelType>::New_ctor();
        LevelType removeLevel = reinterpret_cast<LevelType>(currentLevel);
        // remove only one level if duplicates
        bool removed = false;
        for(int i = 0; i < levelArr->Length(); i++) {
            // comparison should work
            auto level = levelArr->get(i);
            if(removed || level != removeLevel)
                newLevels->Add(level);
            else
                removed = true;
        }
        auto newLevelsArr = newLevels->ToArray();
        currentPack->customBeatmapLevelCollection->customPreviewBeatmapLevels = newLevelsArr;
        // keep scroll position
        float anchorPosY = levelListTableView->tableView->get_contentTransform()->get_anchoredPosition().y;
        anchorPosY = std::min(anchorPosY, levelListTableView->CellSize() * levelListTableView->NumberOfCells());
        auto newLevelsArrCast = reinterpret_cast<Array<GlobalNamespace::IPreviewBeatmapLevel*>*>(newLevelsArr);
        levelListTableView->SetData(newLevelsArrCast, levelListTableView->favoriteLevelIds, false);
        levelListTableView->tableView->scrollView->ScrollTo(anchorPosY, false);
        // clear selection since the selected song was just deleted
        levelListTableView->ClearSelection();
        // update jsons
        RemoveSongFromPlaylist(json, removeLevel);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(playlistRemoveButton->get_transform()->GetChild(0))->set_sizeDelta({4, 4});
    playlistRemoveButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(playlistRemoveButton, RemoveFromPlaylistInactiveSprite(), RemoveFromPlaylistSprite());
    #pragma endregion

    co_yield nullptr;

    #pragma region modals
    playlistAddModal = BeatSaberUI::CreateModal(levelDetailTransform, {83, 17}, nullptr);

    playlistCovers = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(playlistAddModal->get_transform(), {70, 15}, [this](int cellIdx){
        // add to playlist
        auto pack = loadedPacks[cellIdx];
        auto json = GetPlaylistJSON(STR(pack->get_packName()));
        auto levelArr = pack->customBeatmapLevelCollection->customPreviewBeatmapLevels;
        // using a list because arrays are hell
        using LevelType = GlobalNamespace::CustomPreviewBeatmapLevel*;
        auto newLevels = List<LevelType>::New_ctor();
        for(int i = 0; i < levelArr->Length(); i++) {
            newLevels->Add(levelArr->get(i));
        }
        newLevels->Add(reinterpret_cast<LevelType>(currentLevel));
        auto newLevelsArr = newLevels->ToArray();
        pack->customBeatmapLevelCollection->customPreviewBeatmapLevels = newLevelsArr;
        // set cells deselected
        playlistCovers->tableView->ClearSelection();
        playlistAddModal->Hide(true, nullptr);
        // update playlist table if necessary
        if(pack == currentPack) {
            // keep selected cell
            int currIndex = levelListTableView->selectedRow;
            // keep scroll position
            float anchorPosY = levelListTableView->tableView->get_contentTransform()->get_anchoredPosition().y;
            anchorPosY = std::min(anchorPosY, levelListTableView->CellSize() * levelListTableView->NumberOfCells());
            auto newLevelsArrCast = reinterpret_cast<Array<GlobalNamespace::IPreviewBeatmapLevel*>*>(newLevelsArr);
            levelListTableView->SetData(newLevelsArrCast, levelListTableView->favoriteLevelIds, false);
            levelListTableView->tableView->SelectCellWithIdx(currIndex, true);
            levelListTableView->tableView->scrollView->ScrollTo(anchorPosY, false);
        }
        // update jsons
        AddSongToPlaylist(json, reinterpret_cast<LevelType>(currentLevel));
    });
    playlistCovers->setType(csTypeOf(PlaylistManager::CoverTableCell*));
    playlistCovers->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    playlistCovers->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    RefreshPlaylists();

    co_yield nullptr;
    
    // scroll arrows
    auto left = BeatSaberUI::CreateUIButton(playlistAddModal->get_transform(), "", "SettingsButton", {-38, 0}, {8, 8}, [this](){
        // get table view as questui table view
        auto tableView = CRASH_UNLESS(il2cpp_utils::GetFieldValue<QuestUI::TableView*>(reinterpret_cast<Il2CppObject*>(this->playlistCovers), "tableView"));
        int idx = std::min((int)(tableView->get_contentTransform()->get_anchoredPosition().x / tableView->get_cellSize())*-1, tableView->get_numberOfCells() - 1);
        idx -= 4;
        idx = idx > 0 ? idx : 0;
        tableView->ScrollToCellWithIdx(idx, HMUI::TableView::ScrollPositionType::Beginning, true);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    auto right = BeatSaberUI::CreateUIButton(playlistAddModal->get_transform(), "", "SettingsButton", {38, 0}, {8, 8}, [this](){
        // get table view as questui table view
        auto tableView = CRASH_UNLESS(il2cpp_utils::GetFieldValue<QuestUI::TableView*>(reinterpret_cast<Il2CppObject*>(this->playlistCovers), "tableView"));
        int idx = std::min((int)(tableView->get_contentTransform()->get_anchoredPosition().x / tableView->get_cellSize())*-1, tableView->get_numberOfCells() - 1);
        idx += 4;
        int max = tableView->get_dataSource()->NumberOfCells();
        idx = idx < max ? idx : max - 1;
        tableView->ScrollToCellWithIdx(idx, HMUI::TableView::ScrollPositionType::Beginning, true);
    });
    reinterpret_cast<UnityEngine::RectTransform*>(right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());
    #pragma endregion

    co_return;
}

void ButtonsContainer::Init(GlobalNamespace::StandardLevelDetailView* levelDetailView) {
    // get constant references
    this->levelDetailView = levelDetailView;
    auto arr = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::LevelCollectionTableView*>();
    if(arr->Length() < 1) {
        LOG_ERROR("Unable to find level collection table view!");
        SAFE_ABORT();
    }
    this->levelListTableView = arr->get(0);
    
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(
        reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(initCoroutine())));
}

void ButtonsContainer::SetVisible(bool visible, bool showRemove) {
    saveCoverButton->get_gameObject()->set_active(visible);
    playlistAddButton->get_gameObject()->set_active(visible);
    playlistRemoveButton->get_gameObject()->set_active(visible && showRemove);
}

void ButtonsContainer::SetLevel(GlobalNamespace::IPreviewBeatmapLevel* level) {
    currentLevel = level;
}

void ButtonsContainer::SetPack(GlobalNamespace::CustomBeatmapLevelPack* pack) {
    currentPack = pack;
}

void ButtonsContainer::RefreshPlaylists() {
    loadedPacks = PlaylistManager::GetLoadedPlaylists();
    std::vector<UnityEngine::Sprite*> newCovers;
    std::vector<std::string> newHovers;
    for(auto pack : loadedPacks) {
        newCovers.emplace_back(pack->get_coverImage());
        newHovers.emplace_back(STR(pack->get_packName()));
    }
    playlistCovers->replaceSprites(newCovers);
    playlistCovers->replaceTexts(newHovers);
    playlistCovers->tableView->ReloadData();
}
