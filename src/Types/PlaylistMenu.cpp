#include "Main.hpp"
#include "Types/Config.hpp"
#include "Types/PlaylistMenu.hpp"
#include "Types/LevelButtons.hpp"
#include "Types/CustomListSource.hpp"
#include "Types/CoverTableCell.hpp"
#include "PlaylistManager.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"

#include "UnityEngine/Mathf.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/UI/Mask.hpp"

#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/UnityWebRequest_UnityWebRequestError.hpp"
#include "UnityEngine/Networking/DownloadHandler.hpp"

#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/UIKeyboard.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/LevelPackHeaderTableCell.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"

#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Action_1.hpp"

#include <math.h>

using namespace PlaylistManager;
using namespace QuestUI;
using namespace GlobalNamespace;

DEFINE_TYPE(PlaylistManager, PlaylistMenu);

#define ANCHOR(component, xmin, ymin, xmax, ymax) auto component##_rect = reinterpret_cast<UnityEngine::RectTransform*>(component->get_transform()); \
component##_rect->set_anchorMin({xmin, ymin}); \
component##_rect->set_anchorMax({xmax, ymax});

std::function<void()> PlaylistMenu::nextCloseKeyboard = nullptr;
PlaylistMenu* PlaylistMenu::menuInstance = nullptr;

UnityEngine::GameObject* anchorContainer(UnityEngine::Transform* parent, float xmin, float ymin, float xmax, float ymax) {
    STATIC_CSTR(name, "BPContainer");
    auto go = UnityEngine::GameObject::New_ctor(name);
    go->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    
    auto rect = go->GetComponent<UnityEngine::RectTransform*>();
    rect->SetParent(parent, false);
    rect->set_anchorMin({xmin, ymin});
    rect->set_anchorMax({xmax, ymax});
    rect->set_sizeDelta({0, 0});

    go->AddComponent<UnityEngine::UI::LayoutElement*>();
    return go;
}

UnityEngine::UI::Button* anchorMiniButton(UnityEngine::Transform* parent, std::string_view buttonText, std::string_view buttonTemplate, std::function<void()> onClick, float x, float y) {
    auto button = BeatSaberUI::CreateUIButton(parent, buttonText, buttonTemplate, {0, 0}, {8, 8}, onClick);

    UnityEngine::GameObject::Destroy(button->get_transform()->Find(CSTR("Content"))->GetComponent<UnityEngine::UI::LayoutElement*>());
    button->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-0.8, 0});

    ANCHOR(button, x, y, x, y);

    return button;
}

float movementEasing(float p) {
    // ease in out quartic
    if (p < 0.5f) {
        return 8 * p * p * p * p;
    } else {
        float f = p - 1;
        return (-8 * f * f * f * f) + 1;
    }
}

custom_types::Helpers::Coroutine PlaylistMenu::moveCoroutine(bool reversed) {
    // no overlap
    if(inMovement)
        co_return;
    inMovement = true;
    float time = 0;
    static const float duration = 0.4;
    static const double pi = 3.141592653589793;
    if(!reversed) {
        // moving into view
        detailsContainer->set_active(true);
        detailsVisible = true;
    }
    auto transf = (UnityEngine::RectTransform*) detailsContainer->get_transform();
    float minY = transf->get_anchorMin().y;
    float maxY = transf->get_anchorMax().y;
    while(time < 1) {
        time += UnityEngine::Time::get_deltaTime()/duration;
        float val = movementEasing(time);
        if(!reversed) {
            transf->set_anchorMin({1 - val, minY});
            transf->set_anchorMax({2 - val, maxY});
        } else {
            transf->set_anchorMin({val, minY});
            transf->set_anchorMax({val + 1, maxY});
        }
        co_yield nullptr;
    }
    if(reversed) {
        // moved out of view
        detailsContainer->set_active(false);
        detailsVisible = false;
    }
    inMovement = false;
    co_return;
}

custom_types::Helpers::Coroutine PlaylistMenu::refreshCoroutine() {
    // don't mess with other animations
    if(inMovement)
        co_return;
    // don't close if already closed
    if(detailsVisible)
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(
            StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(
                custom_types::Helpers::CoroutineHelper::New(moveCoroutine(true)) )) );
    updateDetailsMode();
    co_yield reinterpret_cast<System::Collections::IEnumerator*>(
            StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(
                custom_types::Helpers::CoroutineHelper::New(moveCoroutine(false)) )) );
    co_return;
}

custom_types::Helpers::Coroutine PlaylistMenu::syncCoroutine() {
    LOG_INFO("syncing");
    auto& customData = playlist->playlistJSON.CustomData;
    if(!customData.has_value())
        co_return;
    LOG_INFO("has custom data");
    auto& syncUrl = customData->SyncURL;
    if(!syncUrl.has_value())
        co_return;
    LOG_INFO("sending request");
    auto webRequest = UnityEngine::Networking::UnityWebRequest::Get(CSTR(syncUrl.value()));

    co_yield (System::Collections::IEnumerator*) webRequest->SendWebRequest();

    if(webRequest->GetError() != UnityEngine::Networking::UnityWebRequest::UnityWebRequestError::OK) {
        LOG_ERROR("Sync request failed! Error: %s", STR(webRequest->GetWebErrorString(webRequest->GetError())).c_str());
        co_return;
    }

    int configIdx = GetPackIndex(playlist->name);
    int tableIdx = gameTableView->selectedCellIndex;
    std::string path = playlist->path;
    // delete outdated playlist so that the new one can be fully reloaded
    DeletePlaylist(playlist);
    // save synced playlist
    std::string text = STR(webRequest->get_downloadHandler()->GetText());
    writefile(path, text);
    // reload playlists
    RefreshPlaylists();
    // playlist should be at the end, since it was removed from the config on deletion
    auto newPlaylist = *(GetLoadedPlaylists().end() - 1);
    // move playlist to correct index
    MovePlaylist(newPlaylist, configIdx);
    // move playlist in table
    using CollectionType = IAnnotatedBeatmapLevelCollection*;
    using EnumerableType = System::Collections::Generic::IEnumerable_1<CollectionType>*;
    // janky casting
    auto collectionList = List<CollectionType>::New_ctor(reinterpret_cast<EnumerableType>(gameTableView->annotatedBeatmapLevelCollections));
    int length = collectionList->get_Count();
    auto movedCollection = collectionList->get_Item(length - 1);
    collectionList->RemoveAt(length - 1);
    collectionList->Insert(tableIdx, movedCollection);
    gameTableView->SetData(reinterpret_cast<System::Collections::Generic::IReadOnlyList_1<CollectionType>*>(collectionList->AsReadOnly()));
    scrollToIndex(tableIdx);
    // update playlist in level buttons
    if(ButtonsContainer::buttonsInstance)
        ButtonsContainer::buttonsInstance->RefreshPlaylists();

    co_return;
}

#pragma region uiFunctions
void PlaylistMenu::infoButtonPressed() {
    if(inMovement)
        return;
    // toggle mode if open to viewing details
    if(!addingPlaylist) {
        // reset texts to current values for playlist when opening
        if(!detailsVisible)
            updateDetailsMode();
        ShowDetails(!detailsVisible);
    } else {
        addingPlaylist = false;
        RefreshDetails();
    }
}

void PlaylistMenu::syncButtonPressed() {
    LOG_INFO("syncButtonPressed");
    SharedCoroutineStarter::get_instance()->StartCoroutine(
        reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(syncCoroutine())));
}

void PlaylistMenu::addButtonPressed() {
    if(inMovement)
        return;
    // do nothing if already open for adding a playlist
    if(addingPlaylist && !detailsVisible) {
        // also reset on reopen
        updateDetailsMode();
        ShowDetails(true);
    } else if(!addingPlaylist) {
        addingPlaylist = true;
        RefreshDetails();
    }
}

void PlaylistMenu::moveRightButtonPressed() {
    // use old cell idx because not all configured playlists will always be shown
    int oldCellIdx = gameTableView->selectedCellIndex;
    int configIdx = GetPackIndex(playlist->name);
    if(configIdx == -1)
        configIdx = playlistConfig.Order.size() - 1;
    if(oldCellIdx + 1 == gameTableView->GetNumberOfCells() || configIdx >= playlistConfig.Order.size() - 1)
        return;
    MovePlaylist(playlist, configIdx + 1);
    // move playlist in table
    using CollectionType = IAnnotatedBeatmapLevelCollection*;
    // janky casting
    auto collectionList = List<CollectionType>::New_ctor(reinterpret_cast<System::Collections::Generic::IEnumerable_1<CollectionType>*>(gameTableView->annotatedBeatmapLevelCollections));
    auto movedCollection = collectionList->get_Item(oldCellIdx);
    collectionList->RemoveAt(oldCellIdx);
    collectionList->Insert(oldCellIdx + 1, movedCollection);
    gameTableView->SetData(reinterpret_cast<System::Collections::Generic::IReadOnlyList_1<CollectionType>*>(collectionList->AsReadOnly()));
    scrollToIndex(oldCellIdx + 1);
}

void PlaylistMenu::moveLeftButtonPressed() {
    // use old cell idx because not all configured playlists will always be shown
    int oldCellIdx = gameTableView->selectedCellIndex;
    int configIdx = GetPackIndex(playlist->name);
    if(configIdx == -1)
        configIdx = playlistConfig.Order.size() - 1;
    if(oldCellIdx <= 1 || configIdx == 0)
        return;
    MovePlaylist(playlist, configIdx - 1);
    // move playlist in table
    using CollectionType = IAnnotatedBeatmapLevelCollection*;
    // janky casting
    auto collectionList = List<CollectionType>::New_ctor(reinterpret_cast<System::Collections::Generic::IEnumerable_1<CollectionType>*>(gameTableView->annotatedBeatmapLevelCollections));
    auto movedCollection = collectionList->get_Item(oldCellIdx);
    collectionList->RemoveAt(oldCellIdx);
    collectionList->Insert(oldCellIdx - 1, movedCollection);
    gameTableView->SetData(reinterpret_cast<System::Collections::Generic::IReadOnlyList_1<CollectionType>*>(collectionList->AsReadOnly()));
    scrollToIndex(oldCellIdx - 1);
}

void PlaylistMenu::playlistTitleTyped(std::string_view newValue) {
    currentTitle = newValue.data();
    if(!PlaylistMenu::nextCloseKeyboard) {
        PlaylistMenu::nextCloseKeyboard = [this](){
            // check for valid title
            if(!AvailablePlaylistName(currentTitle)) {
                LOG_INFO("Resetting invalid title");
                if(!addingPlaylist)
                    playlistTitle->SetText(CSTR(playlist->name));
                else
                    playlistTitle->SetText(CSTR("New Playlist"));
                return;
            }
            if(!addingPlaylist) {
                LOG_INFO("Title set to %s", currentTitle.c_str());
                RenamePlaylist(playlist, currentTitle);
                // get header cell and set text
                auto arr = UnityEngine::Resources::FindObjectsOfTypeAll<LevelCollectionTableView*>();
                if(arr.Length() < 1)
                    return;
                auto tableView = arr[0];
                if(!tableView->showLevelPackHeader)
                    return;
                tableView->headerText = CSTR(currentTitle);
                tableView->tableView->RefreshCells(true, true);
                // update hover texts
                if(ButtonsContainer::buttonsInstance)
                    ButtonsContainer::buttonsInstance->RefreshPlaylists();
            }
        };
    }
    // title cleared (x button)
    if(!playlistTitle->hasKeyboardAssigned && System::String::IsNullOrEmpty(playlistTitle->get_text()))
        PlaylistMenu::nextCloseKeyboard = nullptr;
}

void PlaylistMenu::playlistAuthorTyped(std::string_view newValue) {
    currentAuthor = newValue.data();
    if(!PlaylistMenu::nextCloseKeyboard) {
        PlaylistMenu::nextCloseKeyboard = [this](){
            LOG_INFO("Author set to %s", currentAuthor.c_str());
            if(!addingPlaylist)
                playlist->playlistJSON.PlaylistAuthor = currentAuthor;
        };
    }
    // author cleared (x button)
    if(!playlistAuthor->hasKeyboardAssigned && System::String::IsNullOrEmpty(playlistAuthor->get_text())) {
        PlaylistMenu::nextCloseKeyboard();
        PlaylistMenu::nextCloseKeyboard = nullptr;
    }
}

void PlaylistMenu::coverButtonPressed() {
    RefreshCovers();
    coverModal->Show(true, false, nullptr);
    // have correct cover selected and shown
    auto index = addingPlaylist ? coverImageIndex + 1 : playlist->imageIndex + 1;
    list->tableView->SelectCellWithIdx(index, false);
    list->tableView->ScrollToCellWithIdx(index, HMUI::TableView::ScrollPositionType::Center, false);
}

void PlaylistMenu::deleteButtonPressed() {
    if(confirmModal)
        confirmModal->Show(true, false, nullptr);
}

void PlaylistMenu::createButtonPressed() {
    // while the keyboard function has a check, names such as "New Playlist" may also be unavailable
    if(!AvailablePlaylistName(currentTitle)) {
        LOG_ERROR("Attempting to create playlist with unavailable name");
        return;
    }
    // create new playlist based on fields
    AddPlaylist(currentTitle, currentAuthor, coverImage->get_sprite());
    RefreshPlaylists();
    if(ButtonsContainer::buttonsInstance)
        ButtonsContainer::buttonsInstance->RefreshPlaylists();
}

void PlaylistMenu::cancelButtonPressed() {
    ShowDetails(false);
}

void PlaylistMenu::confirmDeleteButtonPressed() {
    confirmModal->Hide(true, nullptr);
    // delete playlist
    DeletePlaylist(playlist);
    playlist = nullptr;
    RefreshPlaylists();
    if(ButtonsContainer::buttonsInstance)
        ButtonsContainer::buttonsInstance->RefreshPlaylists();
}

void PlaylistMenu::cancelDeleteButtonPressed() {
    confirmModal->Hide(true, nullptr);
}

void PlaylistMenu::coverSelected(int listCellIdx) {
    auto sprite = list->getSprite(listCellIdx);
    // don't set playlist cover when adding a new one
    if(!addingPlaylist) {
        // change in list and json
        if(listCellIdx == 0)
            ChangePlaylistCover(playlist, nullptr, listCellIdx - 1);
        else
            ChangePlaylistCover(playlist, sprite, listCellIdx - 1);
        // change background pack image
        packImage->set_sprite(sprite);
        // change image in playlist bar
        gameTableView->gridView->ReloadData();
    }
    coverImage->set_sprite(sprite);
    // one less because the default image is not included
    coverImageIndex = listCellIdx - 1;
    coverModal->Hide(true, nullptr);
}

void PlaylistMenu::scrollListLeftButtonPressed() {
    CustomListSource::ScrollListLeft(list, 4);
}

void PlaylistMenu::scrollListRightButtonPressed() {
    CustomListSource::ScrollListRight(list, 4);
}
#pragma endregion

custom_types::Helpers::Coroutine PlaylistMenu::initCoroutine() {
    #pragma region details
    // use pack image as a mask for details
    packImage->get_gameObject()->AddComponent<UnityEngine::UI::Mask*>();

    // details container
    detailsContainer = anchorContainer(packImage->get_transform(), 1, 0, 2, 1);
    auto detailsBackground = BeatSaberUI::CreateImage(detailsContainer->get_transform(), WhiteSprite(), {0, 0}, {0, 0});
    ANCHOR(detailsBackground, 0, 0.15, 1, 1);
    detailsBackground->set_color({0.15, 0.15, 0.15, 0.93});

    playlistTitle = BeatSaberUI::CreateStringSetting(detailsContainer->get_transform(), "Playlist Title", "", [this](std::string_view newValue){
        playlistTitleTyped(newValue);
    });
    playlistTitle->GetComponent<UnityEngine::RectTransform*>()->set_sizeDelta({19.8, 3});
    playlistTitle->textView->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    ANCHOR(playlistTitle, 0.2, 0.89, 0.8, 0.99);

    playlistAuthor = BeatSaberUI::CreateStringSetting(detailsContainer->get_transform(), "Playlist Author", "", {0, 0}, {0, -0.1, 0}, [this](std::string_view newValue){
        playlistAuthorTyped(newValue);
    });
    playlistAuthor->GetComponent<UnityEngine::RectTransform*>()->set_sizeDelta({19.8, 3});
    playlistAuthor->textView->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    ANCHOR(playlistAuthor, 0.2, 0.74, 0.8, 0.84);

    coverButton = BeatSaberUI::CreateUIButton(detailsContainer->get_transform(), "Change Cover", UnityEngine::Vector2{0, 0}, {15, 5}, [this](){
        coverButtonPressed();
    });
    ANCHOR(coverButton, 0.17, 0.57, 0.35, 0.64);

    deleteButton = BeatSaberUI::CreateUIButton(detailsContainer->get_transform(), "Delete", UnityEngine::Vector2{0, 0}, {15, 5}, [this](){
        deleteButtonPressed();
    });
    deleteButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-5, 0});
    ANCHOR(deleteButton, 0.65, 0.57, 0.73, 0.64);

    // description
    descriptionTitle = BeatSaberUI::CreateText(detailsContainer->get_transform(), "Description");
    ANCHOR(descriptionTitle, 0.565, 0.37, 0.6, 0.42);
    playlistDescription = BeatSaberUI::CreateText(detailsContainer->get_transform(), "", false, {0, 0}, {29, 5});
    playlistDescription->set_enableWordWrapping(true);
    playlistDescription->set_fontSize(3.5);
    playlistDescription->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    playlistDescription->set_alignment(TMPro::TextAlignmentOptions::TopLeft);
    ANCHOR(playlistDescription, 0.29, 0.19, 0.7, 0.37);

    // use playlist sprite if playlist has been set
    coverImage = BeatSaberUI::CreateImage(detailsContainer->get_transform(), playlist ? GetCoverImage(playlist) : GetDefaultCoverImage(), {0, 0}, {0, 0});
    // set rounded corner material
    coverImage->set_material(packImage->get_material());
    ANCHOR(coverImage, 0.05, 0.4, 0.3, 0.65);

    createButton = BeatSaberUI::CreateUIButton(detailsContainer->get_transform(), "Create", "ActionButton", {0, 0}, {13, 5}, [this](){
        createButtonPressed();
    });
    // of course the text isn't centered
    createButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-2, 0});
    ANCHOR(createButton, 0.17, 0.2, 0.35, 0.27);
    
    cancelButton = BeatSaberUI::CreateUIButton(detailsContainer->get_transform(), "Cancel", UnityEngine::Vector2{0, 0}, {13, 5}, [this](){
        cancelButtonPressed();
    });
    ANCHOR(cancelButton, 0.64, 0.2, 0.84, 0.27);
    #pragma endregion
    
    co_yield nullptr;

    #pragma region sideButtons
    buttonsContainer = anchorContainer(packImage->get_transform(), 0, 0, 1, 0.15);
    auto buttonsBackgroundImage = BeatSaberUI::CreateImage(buttonsContainer->get_transform(), WhiteSprite(), {0, 0}, {0, 0});
    ANCHOR(buttonsBackgroundImage, 0, 0.02, 1, 1);
    buttonsBackgroundImage->set_color({0.15, 0.15, 0.15, 0.93});

    // side menu buttons, from bottom to top
    auto infoButton = anchorMiniButton(buttonsContainer->get_transform(), "i", "ActionButton", [this](){
        infoButtonPressed();
    }, 0.9, 0.5);
    // disable all caps mode
    infoButton->GetComponentInChildren<HMUI::CurvedTextMeshPro*>()->set_fontStyle(2);
    BeatSaberUI::AddHoverHint(infoButton->get_gameObject(), "Playlist information");
    
    syncButton = anchorMiniButton(buttonsContainer->get_transform(), "", "ActionButton", [this](){
        syncButtonPressed();
    }, 0.74, 0.5);
    auto img = BeatSaberUI::CreateImage(syncButton->get_transform(), SyncSprite(), {0, 0}, {0, 0});
    img->set_preserveAspect(true);
    img->get_transform()->set_localScale({0.55, 0.55, 0.55});
    BeatSaberUI::AddHoverHint(syncButton->get_gameObject(), "Sync playlist");
    bool syncActive = false;
    if(playlist->playlistJSON.CustomData.has_value())
        sync_active = playlist->playlistJSON.CustomData->SyncURL.has_value();
    syncButton->set_interactable(syncActive);
    
    auto addButton = anchorMiniButton(buttonsContainer->get_transform(), "+", "PracticeButton", [this](){
        addButtonPressed();
    }, 0.58, 0.5);
    BeatSaberUI::AddHoverHint(addButton->get_gameObject(), "Create a new playlist");
    
    auto rightButton = anchorMiniButton(buttonsContainer->get_transform(), ">", "PracticeButton", [this](){
        moveRightButtonPressed();
    }, 0.42, 0.5);
    BeatSaberUI::AddHoverHint(rightButton->get_gameObject(), "Move playlist right");
    
    auto leftButton = anchorMiniButton(buttonsContainer->get_transform(), "<", "PracticeButton", [this](){
        moveLeftButtonPressed();
    }, 0.26, 0.5);
    BeatSaberUI::AddHoverHint(leftButton->get_gameObject(), "Move playlist left");
    #pragma endregion

    co_yield nullptr;

    #pragma region modals
    // confirmation modal for deletion
    confirmModal = BeatSaberUI::CreateModal(get_transform(), {38, 20}, {-7, 0}, nullptr);

    auto confirmText = BeatSaberUI::CreateText(confirmModal->get_transform(), "Are you sure you would like to delete this playlist?", false, {0, 0}, {30, 7});
    confirmText->set_enableWordWrapping(true);
    confirmText->set_alignment(TMPro::TextAlignmentOptions::Center);
    ANCHOR(confirmText, 0.4, 0.5, 0.6, 0.9);

    auto yesButton = BeatSaberUI::CreateUIButton(confirmModal->get_transform(), "Delete", "ActionButton", {0, 0}, {7, 5}, [this](){
        confirmDeleteButtonPressed();
    });
    yesButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-10, 0});
    ANCHOR(yesButton, 0.17, 0.15, 0.37, 0.35);

    auto noButton = BeatSaberUI::CreateUIButton(confirmModal->get_transform(), "Cancel", UnityEngine::Vector2{0, 0}, {7, 5}, [this](){
        cancelDeleteButtonPressed();
    });
    noButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-9.5, 0});
    ANCHOR(noButton, 0.63, 0.15, 0.83, 0.35);

    // playlist cover changing modal
    coverModal = BeatSaberUI::CreateModal(get_transform(), {83, 17}, {-6, -11}, nullptr);

    list = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(coverModal->get_transform(), {70, 15}, [this](int cellIdx){
        coverSelected(cellIdx);
    });
    list->setType(csTypeOf(CoverTableCell*));
    list->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    list->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;

    co_yield nullptr;

    RefreshCovers();

    co_yield nullptr;

    // scroll arrows
    auto left = BeatSaberUI::CreateUIButton(coverModal->get_transform(), "", "SettingsButton", {-38, 0}, {8, 8}, [this](){
        scrollListLeftButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    auto right = BeatSaberUI::CreateUIButton(coverModal->get_transform(), "", "SettingsButton", {38, 0}, {8, 8}, [this](){
        scrollListRightButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());
    #pragma endregion
    
    detailsContainer->set_active(false);
    detailsVisible = false;

    co_return;
}

void PlaylistMenu::updateDetailsMode() {
    descriptionTitle->get_gameObject()->set_active(!addingPlaylist);
    playlistDescription->get_gameObject()->set_active(!addingPlaylist);
    deleteButton->get_gameObject()->set_active(!addingPlaylist);
    coverImage->get_gameObject()->set_active(addingPlaylist);
    createButton->get_gameObject()->set_active(addingPlaylist);
    cancelButton->get_gameObject()->set_active(addingPlaylist);

    if(!addingPlaylist) {
        playlistTitle->SetText(CSTR(playlist->name));

        std::string auth = playlist->playlistJSON.PlaylistAuthor ? playlist->playlistJSON.PlaylistAuthor.value() : "";
        playlistAuthor->SetText(CSTR(auth));

        std::string desc = playlist->playlistJSON.PlaylistDescription ? playlist->playlistJSON.PlaylistDescription.value() : "...";
        playlistDescription->SetText(CSTR(desc));

        ANCHOR(coverButton, 0.17, 0.57, 0.35, 0.64);
    } else {
        currentTitle = "New Playlist";
        playlistTitle->SetText(CSTR(currentTitle));
        currentAuthor = "Playlist Manager";
        playlistAuthor->SetText(CSTR(currentAuthor));
        playlistDescription->SetText(CSTR(""));

        ANCHOR(coverButton, 0.55, 0.5, 0.73, 0.57);
    }
}

void PlaylistMenu::scrollToIndex(int index) {
    gameTableView->SelectAndScrollToCellWithIdx(index);
    gameTableView->didSelectAnnotatedBeatmapLevelCollectionEvent->Invoke(gameTableView->annotatedBeatmapLevelCollections->get_Item(index));
}

void PlaylistMenu::Init(HMUI::ImageView* imageView, Playlist* list) {
    // get table view for setting selected cell
    gameTableView = UnityEngine::Resources::FindObjectsOfTypeAll<AnnotatedBeatmapLevelCollectionsGridView*>()[0];
    playlist = list;
    packImage = imageView;
    // make sure playlist cover image is present
    GetCoverImage(playlist);
    coverImageIndex = playlist->imageIndex;
    
    // don't let it get stopped by set visible
    SharedCoroutineStarter::get_instance()->StartCoroutine(
        reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(initCoroutine())));
    
    PlaylistMenu::menuInstance = this;
}

void PlaylistMenu::SetPlaylist(Playlist* list) {
    LOG_INFO("Playlist set to %s", list->name.c_str());
    playlist = list;
    // ensure cover is present
    auto cover = GetCoverImage(playlist);
    coverImageIndex = playlist->imageIndex;
    if(coverImage)
        coverImage->set_sprite(cover);
    if(coverModal)
        coverModal->Hide(true, nullptr);
    if(confirmModal)
        confirmModal->Hide(true, nullptr);
    if(syncButton) {
        bool syncActive = false;
        if(playlist->playlistJSON.CustomData.has_value())
            sync_active = playlist->playlistJSON.CustomData->SyncURL.has_value();
        syncButton->set_interactable(syncActive);
    }
}

void PlaylistMenu::RefreshCovers() {
    if(!list) return;
    // reload covers from folder
    GetCoverImages();
    // add cover images and reload
    list->replaceSprites({GetDefaultCoverImage()});
    list->addSprites(GetLoadedImages());
    list->tableView->ReloadData();
}

void PlaylistMenu::ShowDetails(bool visible) {
    StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(moveCoroutine(!visible))));
}

void PlaylistMenu::RefreshDetails() {
    StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(refreshCoroutine())));
}

void PlaylistMenu::SetVisible(bool visible) {
    StopAllCoroutines();
    detailsVisible = false;
    if(buttonsContainer)
        buttonsContainer->set_active(visible);
    if(detailsContainer)
        detailsContainer->set_active(false);
    if(confirmModal)
        confirmModal->Hide(false, nullptr);
    if(coverModal)
        coverModal->Hide(false, nullptr);
}

void PlaylistMenu::Destroy() {
    StopAllCoroutines();
    UnityEngine::Object::Destroy(detailsContainer);
    UnityEngine::Object::Destroy(buttonsContainer);
    PlaylistMenu::menuInstance = nullptr;
    PlaylistMenu::nextCloseKeyboard = nullptr;
    UnityEngine::Object::Destroy(this);
}
