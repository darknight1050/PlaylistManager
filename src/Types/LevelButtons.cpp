#include "Main.hpp"
#include "Types/LevelButtons.hpp"
#include "Types/PlaylistMenu.hpp"
#include "Types/CustomListSource.hpp"
#include "Types/CoverTableCell.hpp"
#include "PlaylistManager.hpp"
#include "ResettableStaticPtr.hpp"
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
    WriteImageToFile(GetCoversPath() + "/" + levelData->get_songName().operator std::string() + "_cover.png", newTexture);
    // reload from file
    LoadCoverImages();
    // show modal for feedback
    static ConstString text("Song cover image saved!");
    infoText->set_text(text);
    infoButtonContainer->SetActive(false);
    infoModal->Show(true, true, nullptr);
    infoModal->get_transform()->set_localPosition({40, -10, 0});
}

void ButtonsContainer::addToPlaylistButtonPressed() {
    // ensure playlists are accurate
    RefreshPlaylists();
    playlistAddModal->Show(true, true, nullptr);
    // needed because of parenting shenanigans or something
    playlistAddModal->get_transform()->set_localPosition({40, -10, 0});
}

void ButtonsContainer::removeFromPlaylistButtonPressed() {
    static ConstString text("Do you really want to remove this song from the playlist?");
    infoText->set_text(text);
    infoButtonContainer->SetActive(true);
    infoModal->Show(true, true, nullptr);
    infoModal->get_transform()->set_localPosition({40, -10, 0});
}

void ButtonsContainer::playlistSelected(int listCellIdx) {
    // add to playlist
    auto& playlist = loadedPlaylists[listCellIdx];
    AddSongToPlaylist(playlist, currentLevel);
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
        auto levelList = currentPlaylist->playlistCS->beatmapLevelCollection->get_beatmapLevels();
        levelListTableView->SetData(levelList, levelListTableView->favoriteLevelIds, false);
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

void ButtonsContainer::confirmRemovalButtonPressed() {
    RemoveSongFromPlaylist(currentPlaylist, currentLevel);
    // keep scroll position
    float anchorPosY = levelListTableView->tableView->get_contentTransform()->get_anchoredPosition().y;
    anchorPosY = std::min(anchorPosY, levelListTableView->CellSize() * levelListTableView->NumberOfCells());
    auto levelList = currentPlaylist->playlistCS->beatmapLevelCollection->get_beatmapLevels();
    levelListTableView->SetData(levelList, levelListTableView->favoriteLevelIds, false);
    // clear selection since the selected song was just deleted
    levelListTableView->tableView->SelectCellWithIdx(0, true);
    levelListTableView->tableView->scrollView->ScrollTo(anchorPosY, false);
}

void ButtonsContainer::cancelRemovalButtonPressed() {
    infoModal->Hide(true, nullptr);
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
    horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleRight);
    horizontal->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({-1, 17});

    saveCoverButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {5, 5}, [this](){
        saveCoverButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(saveCoverButton->get_transform()->GetChild(0))->set_sizeDelta({5, 5});
    saveCoverButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(saveCoverButton, SaveCoverInactiveSprite(), SaveCoverSprite());
    BeatSaberUI::AddHoverHint(saveCoverButton, "Save the cover image of the song for use as a playlist cover");

    playlistAddButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {5, 5}, [this](){
        addToPlaylistButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(playlistAddButton->get_transform()->GetChild(0))->set_sizeDelta({5, 5});
    playlistAddButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(playlistAddButton, AddToPlaylistInactiveSprite(), AddToPlaylistSprite());
    BeatSaberUI::AddHoverHint(playlistAddButton, "Add this song to a playlist");

    playlistRemoveButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "", "SettingsButton", {0, 0}, {5, 5}, [this](){
        removeFromPlaylistButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(playlistRemoveButton->get_transform()->GetChild(0))->set_sizeDelta({5, 5});
    playlistRemoveButton->GetComponentInChildren<HMUI::ImageView*>()->skew = 0.18;
    BeatSaberUI::SetButtonSprites(playlistRemoveButton, RemoveFromPlaylistInactiveSprite(), RemoveFromPlaylistSprite());
    BeatSaberUI::AddHoverHint(playlistRemoveButton, "Remove this song from this playlist");
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
    playlistCovers->tableView->LazyInit();
    
    RefreshPlaylists();

    co_yield nullptr;
    
    // scroll arrows
    auto left = BeatSaberUI::CreateUIButton(playlistAddModal->get_transform(), "", "SettingsButton", {-38, 0}, {8, 8}, [this](){
        scrollListLeftButtonPressed();
    });
    ((UnityEngine::RectTransform*) left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    auto right = BeatSaberUI::CreateUIButton(playlistAddModal->get_transform(), "", "SettingsButton", {38, 0}, {8, 8}, [this](){
        scrollListRightButtonPressed();
    });
    ((UnityEngine::RectTransform*) right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());

    infoModal = BeatSaberUI::CreateModal(levelDetailTransform, {50, 25}, nullptr);
    auto infoModalLayout = BeatSaberUI::CreateVerticalLayoutGroup(infoModal);
    infoModalLayout->set_spacing(-1);

    infoText = BeatSaberUI::CreateText(infoModalLayout, "Song cover image saved!");
    infoText->set_enableWordWrapping(true);
    infoText->set_alignment(TMPro::TextAlignmentOptions::Center);
    auto textLayout = infoText->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
    textLayout->set_preferredWidth(45);
    textLayout->set_preferredHeight(10);

    infoButtonContainer = UnityEngine::GameObject::New_ctor("PlaylistManagerModalButtonContainer");
    infoButtonContainer->get_transform()->SetParent(infoModalLayout->get_transform(), false);
    auto buttonLayout = infoButtonContainer->AddComponent<UnityEngine::UI::LayoutElement*>();
    buttonLayout->set_preferredWidth(40);
    buttonLayout->set_preferredHeight(10);
    static ConstString contentName("Content");
    auto confirmRemovalButton = BeatSaberUI::CreateUIButton(infoButtonContainer->get_transform(), "Remove", "ActionButton", {-11.5, 0}, {20, 10}, [this] {
        confirmRemovalButtonPressed();
    });
    UnityEngine::Object::Destroy(confirmRemovalButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    auto cancelRemovalButton = BeatSaberUI::CreateUIButton(infoButtonContainer->get_transform(), "Cancel", {11.5, 0}, {20, 10}, [this] {
        cancelRemovalButtonPressed();
    });
    UnityEngine::Object::Destroy(cancelRemovalButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    #pragma endregion

    co_return;
}

void ButtonsContainer::Init(GlobalNamespace::StandardLevelDetailView* detailView) {
    // get constant references
    levelDetailView = detailView;
    levelListTableView = FindComponent<GlobalNamespace::LevelCollectionTableView*>();
    
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(
        custom_types::Helpers::CoroutineHelper::New(initCoroutine()));
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
    if(infoModal)
        infoModal->Hide(false, nullptr);
}

void ButtonsContainer::SetLevel(GlobalNamespace::IPreviewBeatmapLevel* level) {
    currentLevel = level;
    if(playlistAddModal)
        playlistAddModal->Hide(false, nullptr);
    if(infoModal)
        infoModal->Hide(false, nullptr);
}

void ButtonsContainer::SetPlaylist(Playlist* playlist) {
    currentPlaylist = playlist;
    if(playlistAddModal)
        playlistAddModal->Hide(false, nullptr);
    if(infoModal)
        infoModal->Hide(false, nullptr);
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
    playlistCovers->tableView->ReloadDataKeepingPosition();
}

void ButtonsContainer::RefreshHighlightedDifficulties() {
    if(!currentPlaylist)
        return;
    // get difficulty display object
    auto segmentedController = levelDetailView->beatmapDifficultySegmentedControlController;
    auto cells = segmentedController->difficultySegmentedControl->cells;
    // get selected characteristic
    auto selected = levelDetailView->beatmapCharacteristicSegmentedControlController->selectedBeatmapCharacteristic;
    // might not be selected at first
    if(!selected)
        return;
    std::string characteristic = selected->serializedName;
    LOWER(characteristic);
    // get current song difficulties
    auto& songs = currentPlaylist->playlistJSON.Songs;
    auto currentHash = GetLevelHash(currentLevel);
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
