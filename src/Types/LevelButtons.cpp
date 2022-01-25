#include "Main.hpp"
#include "Types/LevelButtons.hpp"
#include "Types/PlaylistMenu.hpp"
#include "Types/CustomListSource.hpp"
#include "Types/CoverTableCell.hpp"
#include "PlaylistManager.hpp"
#include "Utils.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

#include "HMUI/ScrollView.hpp"
#include "HMUI/TextSegmentedControl.hpp"

#include "UnityEngine/Resources.hpp"

#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

using namespace PlaylistManager;
using namespace QuestUI;

ButtonsContainer* ButtonsContainer::buttonsInstance = nullptr;

#pragma region uiFunctions
void ButtonsContainer::saveCoverButtonPressed() {
    // get cover image (blocking async, yay)
    auto levelData = reinterpret_cast<GlobalNamespace::IPreviewBeatmapLevel*>(levelDetailView->level);
    auto sprite = levelData->GetCoverImageAsync(System::Threading::CancellationToken::get_None())->get_Result();
    if(!sprite)
        return;
    // copy sprite texture for downscaling
    auto newTexture = UnityEngine::Texture2D::New_ctor(sprite->get_texture()->get_width(), sprite->get_texture()->get_height());
    newTexture->SetPixels(sprite->get_texture()->GetPixels());
    // save to file
    ProcessImage(newTexture, false);
    WriteImageToFile(GetCoversPath() + "/" + STR(levelData->get_songName()) + "_cover.png", newTexture);
    // reload from file
    if(PlaylistMenu::menuInstance)
        PlaylistMenu::menuInstance->RefreshCovers();
}

void ButtonsContainer::addToPlaylistButtonPressed() {
    playlistAddModal->Show(true, true, nullptr);
    playlistAddModal->get_transform()->set_localPosition({40, -10, 0});
}

void ButtonsContainer::removeFromPlaylistButtonPressed() {
    RemoveSongFromPlaylist(currentPlaylist, (GlobalNamespace::CustomPreviewBeatmapLevel*) currentLevel);
    // keep scroll position
    float anchorPosY = levelListTableView->tableView->get_contentTransform()->get_anchoredPosition().y;
    anchorPosY = std::min(anchorPosY, levelListTableView->CellSize() * levelListTableView->NumberOfCells());
    auto levelsArr = currentPlaylist->playlistCS->customBeatmapLevelCollection->customPreviewBeatmapLevels;
    auto levelsArrCast = *((ArrayW<GlobalNamespace::IPreviewBeatmapLevel*>*) &levelsArr);
    levelListTableView->SetData(levelsArrCast, levelListTableView->favoriteLevelIds, false);
    levelListTableView->tableView->scrollView->ScrollTo(anchorPosY, false);
    // clear selection since the selected song was just deleted
    levelListTableView->tableView->SelectCellWithIdx(0, true);
}

void ButtonsContainer::playlistSelected(int listCellIdx) {
    // add to playlist
    auto& playlist = loadedPlaylists[listCellIdx];
    AddSongToPlaylist(playlist, (GlobalNamespace::CustomPreviewBeatmapLevel*) currentLevel);
    // set cells deselected
    playlistCovers->tableView->ClearSelection();
    playlistAddModal->Hide(true, nullptr);
    // update playlist table if necessary
    if(playlist == currentPlaylist) {
        // keep selected cell
        int currIndex = levelListTableView->selectedRow;
        // keep scroll position
        float anchorPosY = levelListTableView->tableView->get_contentTransform()->get_anchoredPosition().y;
        anchorPosY = std::min(anchorPosY, levelListTableView->CellSize() * levelListTableView->NumberOfCells());
        auto levelsArr = playlist->playlistCS->customBeatmapLevelCollection->customPreviewBeatmapLevels;
        auto levelsArrCast = *((ArrayW<GlobalNamespace::IPreviewBeatmapLevel*>*) &levelsArr);
        levelListTableView->SetData(levelsArrCast, levelListTableView->favoriteLevelIds, false);
        levelListTableView->tableView->SelectCellWithIdx(currIndex, true);
        levelListTableView->tableView->scrollView->ScrollTo(anchorPosY, false);
    }
}

void ButtonsContainer::scrollListLeftButtonPressed() {
    CustomListSource::ScrollListLeft(playlistCovers, 4);
}

void ButtonsContainer::scrollListRightButtonPressed() {
    CustomListSource::ScrollListRight(playlistCovers, 4);
}
#pragma endregion

custom_types::Helpers::Coroutine ButtonsContainer::initCoroutine() {
    auto levelDetailTransform = levelDetailView->get_transform();

    #pragma region buttons
    auto horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(levelDetailTransform);
    layoutObject = horizontal->get_gameObject();
    horizontal->set_childScaleWidth(false);
    horizontal->set_childControlWidth(false);
    horizontal->set_childForceExpandWidth(false);
    horizontal->set_spacing(1);
    horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleRight);
    horizontal->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({-1, 17});

    saveCoverButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {4, 4}, [this](){
        saveCoverButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(saveCoverButton->get_transform()->GetChild(0))->set_sizeDelta({4, 4});
    saveCoverButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(saveCoverButton, SaveCoverInactiveSprite(), SaveCoverSprite());

    playlistAddButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {4, 4}, [this](){
        addToPlaylistButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(playlistAddButton->get_transform()->GetChild(0))->set_sizeDelta({4, 4});
    playlistAddButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(playlistAddButton, AddToPlaylistInactiveSprite(), AddToPlaylistSprite());

    playlistRemoveButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {4, 4}, [this](){
        removeFromPlaylistButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(playlistRemoveButton->get_transform()->GetChild(0))->set_sizeDelta({4, 4});
    playlistRemoveButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(playlistRemoveButton, RemoveFromPlaylistInactiveSprite(), RemoveFromPlaylistSprite());
    #pragma endregion

    co_yield nullptr;

    #pragma region modals
    playlistAddModal = BeatSaberUI::CreateModal(levelDetailTransform, {83, 17}, nullptr);

    playlistCovers = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(playlistAddModal->get_transform(), {70, 15}, [this](int cellIdx){
        playlistSelected(cellIdx);
    });
    playlistCovers->setType(csTypeOf(CoverTableCell*));
    playlistCovers->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    playlistCovers->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    RefreshPlaylists();

    co_yield nullptr;
    
    // scroll arrows
    auto left = BeatSaberUI::CreateUIButton(playlistAddModal->get_transform(), "", "SettingsButton", {-38, 0}, {8, 8}, [this](){
        scrollListLeftButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    auto right = BeatSaberUI::CreateUIButton(playlistAddModal->get_transform(), "", "SettingsButton", {38, 0}, {8, 8}, [this](){
        scrollListRightButtonPressed();
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
    if(arr.Length() < 1) {
        LOG_ERROR("Unable to find level collection table view!");
        SAFE_ABORT();
    }
    this->levelListTableView = arr[0];
    
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(
        reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(initCoroutine())));
}

void ButtonsContainer::SetVisible(bool visible, bool showRemove) {
    if(saveCoverButton)
        saveCoverButton->get_gameObject()->set_active(visible);
    if(playlistAddButton)
        playlistAddButton->get_gameObject()->set_active(visible);
    if(playlistRemoveButton)
        playlistRemoveButton->get_gameObject()->set_active(visible && showRemove);
    if(playlistAddModal)
        playlistAddModal->Hide(false, nullptr);
}

void ButtonsContainer::SetLevel(GlobalNamespace::IPreviewBeatmapLevel* level) {
    currentLevel = level;
    if(playlistAddModal)
        playlistAddModal->Hide(false, nullptr);
}

void ButtonsContainer::SetPlaylist(Playlist* playlist) {
    currentPlaylist = playlist;
    if(playlistAddModal)
        playlistAddModal->Hide(false, nullptr);
}

void ButtonsContainer::RefreshPlaylists() {
    if(!playlistCovers)
        return;
    loadedPlaylists = GetLoadedPlaylists();
    std::vector<UnityEngine::Sprite*> newCovers;
    std::vector<std::string> newHovers;
    for(auto& playlist : loadedPlaylists) {
        newCovers.emplace_back(GetCoverImage(playlist));
        newHovers.emplace_back(playlist->name);
    }
    playlistCovers->replaceSprites(newCovers);
    playlistCovers->replaceTexts(newHovers);
    playlistCovers->tableView->ReloadData();
}

void ButtonsContainer::RefreshHighlightedDifficulties() {
    if(!currentPlaylist)
        return;
    // update current level based on level detail view, due to function order for hooks
    currentLevel = (GlobalNamespace::IPreviewBeatmapLevel*) levelDetailView->level;
    // get difficulty display object
    auto segmentedController = levelDetailView->beatmapDifficultySegmentedControlController;
    auto cells = segmentedController->difficultySegmentedControl->cells;
    // get selected characteristic
    auto selected = levelDetailView->beatmapCharacteristicSegmentedControlController->selectedBeatmapCharacteristic;
    // might not be selected at first
    if(!selected)
        return;
    std::string characteristic = STR(selected->serializedName);
    LOWER(characteristic);
    // get current song difficulties
    auto& songs = currentPlaylist->playlistJSON.Songs;
    auto currentHash = GetLevelHash((GlobalNamespace::CustomPreviewBeatmapLevel*) currentLevel);
    std::vector<Difficulty> difficulties;
    for(auto& song : songs) {
        LOWER(song.Hash);
        if(song.Hash == currentHash && song.Difficulties.has_value()) {
            difficulties = song.Difficulties.value();
            break;
        }
    }
    if(difficulties.size() == 0)
        return;
    // set highlights based on found difficulties
    for(auto& difficulty : difficulties) {
        LOWER(difficulty.Characteristic);
        if(difficulty.Characteristic == characteristic) {
            // attempt to find difficulty
            LOWER(difficulty.Name);
            int diff = -1;
            static const std::vector<std::string> diffNames = {"easy", "normal", "hard", "expert", "expertPlus"};
            for(int i = 0; i < diffNames.size(); i++) {
                if(diffNames[i] == difficulty.Name) {
                    diff = i;
                    break;
                }
            } if(diff < 0)
                continue;
            // get closest index for pc parity
            auto text = cells->get_Item(segmentedController->GetClosestDifficultyIndex(diff))->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
            if(text)
                text->set_faceColor({255, 255, 0, 255});
        }
    }
    
}

void ButtonsContainer::Destroy() {
    ButtonsContainer::buttonsInstance = nullptr;
    UnityEngine::Object::Destroy(layoutObject);
    // assumes it's always allocated with new
    delete this;
}
