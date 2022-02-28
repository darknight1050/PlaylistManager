#include "Main.hpp"
#include "Types/SongDownloaderAddon.hpp"
#include "Types/PlaylistMenu.hpp"
#include "Types/PlaylistFilters.hpp"
#include "Types/LevelButtons.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"
#include "Settings.hpp"
#include "ResettableStaticPtr.hpp"

#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "songloader/shared/API.hpp"

#include "questui/shared/QuestUI.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController_ContentType.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionCell.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/SongPreviewPlayer.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/ISpriteAsyncLoader.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/PreviewDifficultyBeatmapSet.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp" // This needs to be included before RectTransform
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
// #include "UnityEngine/Events/InvokableCallList.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/InputFieldView.hpp"
#include "Zenject/DiContainer.hpp"
#include "System/Tuple_2.hpp"
#include "System/Action_1.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"

using namespace GlobalNamespace;
using namespace PlaylistManager;

ModInfo modInfo;

// shared config data
PlaylistConfig playlistConfig;
Folder* currentFolder = nullptr;
int filterSelectionState = 0;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger;
}

std::string GetPlaylistsPath() {
    static std::string playlistsPath(getDataDir(modInfo) + "Playlists");
    return playlistsPath;
}

std::string GetConfigPath() {
    static std::string configPath = Configuration::getConfigFilePath(modInfo);
    return configPath;
}

std::string GetCoversPath() {
    static std::string coversPath(getDataDir(modInfo) + "Covers");
    return coversPath;
}

using TupleType = System::Tuple_2<int, int>;

// small fix for horizontal tables
MAKE_HOOK_MATCH(TableView_GetVisibleCellsIdRange, &HMUI::TableView::GetVisibleCellsIdRange,
        TupleType*, HMUI::TableView* self) {
    
    using namespace HMUI;
    UnityEngine::Rect rect = self->viewportTransform->get_rect();

    float heightWidth = (self->tableType == TableView::TableType::Vertical) ? rect.get_height() : rect.get_width();
    float position = (self->tableType == TableView::TableType::Vertical) ? self->scrollView->get_position() : -self->scrollView->get_position();

    int min = floor(position / self->cellSize + self->cellSize * 0.001f);
    if (min < 0) {
        min = 0;
    }

    int max = floor((position + heightWidth - self->cellSize * 0.001f) / self->cellSize);
    if (max > self->numberOfCells - 1) {
        max = self->numberOfCells - 1;
    }

    return TupleType::New_ctor(min, max);
}

// allow name and author changes to be made on keyboard close
// assumes only one keyboard will be open at a time
MAKE_HOOK_MATCH(InputFieldView_DeactivateKeyboard, &HMUI::InputFieldView::DeactivateKeyboard,
        void, HMUI::InputFieldView* self, HMUI::UIKeyboard* keyboard) {

    InputFieldView_DeactivateKeyboard(self, keyboard);

    if(PlaylistMenu::nextCloseKeyboard) {
        PlaylistMenu::nextCloseKeyboard();
        PlaylistMenu::nextCloseKeyboard = nullptr;
    }
}

// find all official level packs
MAKE_HOOK_MATCH(LevelFilteringNavigationController_SetupBeatmapLevelPacks, &LevelFilteringNavigationController::SetupBeatmapLevelPacks,
        void, LevelFilteringNavigationController* self) {
    
    LevelFilteringNavigationController_SetupBeatmapLevelPacks(self);

    staticPackIDs = { CustomLevelsPackID, CustomWIPLevelsPackID };
    for(auto& levelPack : self->allOfficialBeatmapLevelPacks) {
        staticPackIDs.insert(levelPack->get_packID());
    }
}

// prevent download icon showing up on empty custom playlists
MAKE_HOOK_MATCH(AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync, &AnnotatedBeatmapLevelCollectionCell::RefreshAvailabilityAsync,
        void, AnnotatedBeatmapLevelCollectionCell* self, AdditionalContentModel* contentModel) {
    
    AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync(self, contentModel);

    auto pack = il2cpp_utils::try_cast<IBeatmapLevelPack>(self->annotatedBeatmapLevelCollection);
    if(pack.has_value()) {
        if(!staticPackIDs.contains(pack.value()->get_packID()))
            self->SetDownloadIconVisible(false);
    }
}

// override header cell behavior
MAKE_HOOK_MATCH(LevelCollectionViewController_SetData, &LevelCollectionViewController::SetData,
        void, LevelCollectionViewController* self, IBeatmapLevelCollection* beatmapLevelCollection, StringW headerText, UnityEngine::Sprite* headerSprite, bool sortLevels, UnityEngine::GameObject* noDataInfoPrefab) {
    
    // only check for null strings, not empty
    // string will be null for level collections but not level packs
    self->showHeader = (bool) headerText;
    // copy base game method implementation
    self->levelCollectionTableView->Init(headerText, headerSprite);
    self->levelCollectionTableView->showLevelPackHeader = self->showHeader;
    if(self->noDataInfoGO) {
        UnityEngine::Object::Destroy(self->noDataInfoGO);
        self->noDataInfoGO = nullptr;
    }
    // also override check for empty collection
    if(beatmapLevelCollection) {
        self->levelCollectionTableView->get_gameObject()->SetActive(true);
        self->levelCollectionTableView->SetData(beatmapLevelCollection->get_beatmapLevels(), self->playerDataModel->playerData->favoritesLevelIds, sortLevels);
        self->levelCollectionTableView->RefreshLevelsAvailability();
    } else {
        if(noDataInfoPrefab)
            self->noDataInfoGO = self->container->InstantiatePrefab(noDataInfoPrefab, self->noDataInfoContainer);
        self->levelCollectionTableView->get_gameObject()->SetActive(false);
    }
    if(self->get_isInViewControllerHierarchy()) {
        if(self->showHeader)
            self->levelCollectionTableView->SelectLevelPackHeaderCell();
        else
            self->levelCollectionTableView->ClearSelection();
        self->songPreviewPlayer->CrossfadeToDefault();
    }
}

// when to show the playlist menu
MAKE_HOOK_MATCH(LevelPackDetailViewController_ShowContent, &LevelPackDetailViewController::ShowContent,
        void, LevelPackDetailViewController* self, LevelPackDetailViewController::ContentType contentType, StringW errorText) {
    
    LevelPackDetailViewController_ShowContent(self, contentType, errorText);

    if(!playlistConfig.Management)
        return;

    static ConstString customPackName(CustomLevelPackPrefixID);

    if(!PlaylistMenu::menuInstance) {
        PlaylistMenu::menuInstance = self->get_gameObject()->AddComponent<PlaylistMenu*>();
        PlaylistMenu::menuInstance->Init(self->packImage);
    }

    if(contentType == LevelPackDetailViewController::ContentType::Owned && self->pack->get_packID()->Contains(customPackName) && !staticPackIDs.contains(self->pack->get_packID())) {
        // find playlist json
        auto playlist = GetPlaylistWithPrefix(self->pack->get_packID());
        if(playlist) {
            PlaylistMenu::menuInstance->SetPlaylist(playlist);
            PlaylistMenu::menuInstance->SetVisible(true);
        } else
            PlaylistMenu::menuInstance->SetVisible(false);
    } else
        PlaylistMenu::menuInstance->SetVisible(false);

    // disable level buttons (hides modal if necessary)
    if(ButtonsContainer::buttonsInstance)
        ButtonsContainer::buttonsInstance->SetVisible(false, false);
}

// when to show the level buttons
MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent, &StandardLevelDetailViewController::ShowContent, 
        void, StandardLevelDetailViewController* self, StandardLevelDetailViewController::ContentType contentType, StringW errorText, float downloadingProgress, StringW downloadingText) {

    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);

    if(!playlistConfig.Management || contentType != StandardLevelDetailViewController::ContentType::OwnedAndReady)
        return;
    
    if(!ButtonsContainer::buttonsInstance) {
        ButtonsContainer::buttonsInstance = new ButtonsContainer();
        ButtonsContainer::buttonsInstance->Init(self->standardLevelDetailView);
    }
    // note: pack is simply the first level pack it finds that contains the level, if selected from all songs etc.
    std::string id = self->pack->get_packID();
    bool customPack = !staticPackIDs.contains(id);
    bool customSong = customPack || id == CustomLevelPackPrefixID "CustomLevels" || id == CustomLevelPackPrefixID "CustomWIPLevels";
    ButtonsContainer::buttonsInstance->SetVisible(customSong, customPack);
    ButtonsContainer::buttonsInstance->SetLevel((IPreviewBeatmapLevel*) self->beatmapLevel);
    ButtonsContainer::buttonsInstance->SetPlaylist(GetPlaylistWithPrefix(id));
    ButtonsContainer::buttonsInstance->RefreshHighlightedDifficulties();
}

// when to set up the folders
MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, 
        void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(!playlistConfig.Management)
        return;

    if(!PlaylistFilters::filtersInstance) {
        PlaylistFilters::filtersInstance = new PlaylistFilters();
        PlaylistFilters::filtersInstance->Init();
    }
}

// hook to apply changes when deselecting a cell in a multi select
MAKE_HOOK_MATCH(TableView_HandleCellSelectionDidChange, &HMUI::TableView::HandleCellSelectionDidChange,
        void, HMUI::TableView* self, HMUI::SelectableCell* selectableCell, HMUI::SelectableCell::TransitionType transitionType, ::Il2CppObject* changeOwner) {
    
    if(self == PlaylistFilters::monitoredTable) {
        int cellIdx = ((HMUI::TableCell*) selectableCell)->get_idx();
        bool wasSelected = self->selectedCellIdxs->Contains(cellIdx);

        TableView_HandleCellSelectionDidChange(self, selectableCell, transitionType, changeOwner);

        if(!selectableCell->get_selected() && wasSelected)
            PlaylistFilters::deselectCallback(cellIdx);
    } else
        TableView_HandleCellSelectionDidChange(self, selectableCell, transitionType, changeOwner);
}

// throw away objects on a soft restart
MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &MenuTransitionsHelper::RestartGame,
        void, MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback) {
    
    DestroyUI();
    
    ResettableStaticPtr::resetAll();

    ClearLoadedImages();

    hasLoaded = false;
    filterSelectionState = 0;

    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

// override the main menu button to reload playlists
MAKE_HOOK_FIND_CLASS_INSTANCE(MainMenuModSettingsViewController_DidActivate, "QuestUI", "MainMenuModSettingsViewController", "DidActivate",
        void, HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    
    MainMenuModSettingsViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(!firstActivation)
        return;

    for(auto& button : self->GetComponentsInChildren<UnityEngine::UI::Button*>()) {
        auto text = button->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if(text->get_text() == "Reload Playlists") {
            // auto eventListener = button->get_onClick();
            // not sure why RemoveAllListeners is stripped, but this is what it does
            // actually the m_RuntimeCalls list has a problem with its generic so I guess we just make a new event
            // eventListener->m_Calls->m_RuntimeCalls->Clear();
            // eventListener->m_Calls->m_NeedsUpdate = true;
            button->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
            button->get_onClick()->AddListener(il2cpp_utils::MakeDelegate<UnityEngine::Events::UnityAction*>((std::function<void()>) [] {
                ReloadPlaylists(true);
            }));
        }
    }
}

// add our playlist selection view controller to the song downloader menu
MAKE_HOOK_FIND_CLASS_INSTANCE(DownloadSongsFlowCoordinator_DidActivate, "SongDownloader", "DownloadSongsFlowCoordinator", "DidActivate",
        void, HMUI::FlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    
    DownloadSongsFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(firstActivation) {
        STATIC_AUTO(AddonViewController, SongDownloaderAddon::Create());
        self->providedRightScreenViewController = AddonViewController;
    }
}

// add a playlist callback to the song downloader buttons
MAKE_HOOK_FIND_CLASS_INSTANCE(DownloadSongsSearchViewController_DidActivate, "SongDownloader", "DownloadSongsSearchViewController", "DidActivate",
        void, HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    
    DownloadSongsSearchViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(!firstActivation)
        return;

    using namespace UnityEngine;
    
    // add listeners to search entry buttons
    auto entryList = self->GetComponentsInChildren<UI::VerticalLayoutGroup*>().First([](UI::VerticalLayoutGroup* x) {
        return x->get_name() == "QuestUIScrollViewContentContainer";
    })->get_transform();

    // offset by one because the prefab doesn't seem to be destroyed yet here
    int entryCount = entryList->GetChildCount() - 1;
    for(int i = 0; i < entryCount; i++) {
        // undo offset to skip first element
        auto downloadButton = entryList->GetChild(i + 1)->GetComponentInChildren<UI::Button*>();
        // get entry at index with some lovely pointer addition
        SearchEntryHack* entryArrStart = (SearchEntryHack*) (((char*) self) + sizeof(HMUI::ViewController));
        // capture button array start and index
        downloadButton->get_onClick()->AddListener(il2cpp_utils::MakeDelegate<Events::UnityAction*>((std::function<void()>) [entryArrStart, i] {
            auto& entry = *(entryArrStart + i);
            // get hash from entry
            std::string hash;
            if(entry.MapType == SearchEntryHack::MapType::BeatSaver)
                hash = entry.map.GetVersions().front().GetHash();
            else if(entry.MapType == SearchEntryHack::MapType::BeastSaber)
                hash = entry.BSsong.GetHash();
            else if(entry.MapType == SearchEntryHack::MapType::ScoreSaber)
                hash = entry.SSsong.GetId();
            // get json object from playlist
            auto playlist = SongDownloaderAddon::SelectedPlaylist;
            if(!playlist)
                return;
            auto& json = playlist->playlistJSON;
            // add a blank song
            json.Songs.emplace_back(BPSong());
            // set info
            auto& songJson = *(json.Songs.end() - 1);
            songJson.Hash = hash;
            // write to file
            WriteToFile(playlist->path, json);
            // have changes be updated
            MarkPlaylistForReload(playlist);
        }));
    }
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "PlaylistManager";
    modInfo.version = VERSION;
    info = modInfo;
    
    auto playlistsPath = GetPlaylistsPath();
    if(!direxists(playlistsPath))
        mkpath(playlistsPath);
    
    auto coversPath = GetCoversPath();
    if(!direxists(coversPath))
        mkpath(coversPath);
    
    auto configPath = GetConfigPath();
    if(fileexists(configPath))
        ReadFromFile(configPath, playlistConfig);
    else
        WriteToFile(configPath, playlistConfig);
}

extern "C" void load() {
    LOG_INFO("Starting PlaylistManager installation...");
    il2cpp_functions::Init();
    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, "Playlist Manager", ModSettingsDidActivate);
    // create fake modInfo for reload playlists button
    ModInfo fakeModInfo;
    fakeModInfo.id = "Reload Playlists";
    QuestUI::Register::RegisterMainMenuModSettingsViewController(fakeModInfo);
    INSTALL_HOOK_ORIG(getLogger(), TableView_GetVisibleCellsIdRange);
    INSTALL_HOOK(getLogger(), InputFieldView_DeactivateKeyboard);
    INSTALL_HOOK(getLogger(), LevelFilteringNavigationController_SetupBeatmapLevelPacks);
    INSTALL_HOOK(getLogger(), AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync);
    INSTALL_HOOK_ORIG(getLogger(), LevelCollectionViewController_SetData);
    INSTALL_HOOK(getLogger(), LevelPackDetailViewController_ShowContent);
    INSTALL_HOOK(getLogger(), StandardLevelDetailViewController_ShowContent);
    INSTALL_HOOK(getLogger(), MainMenuViewController_DidActivate);
    INSTALL_HOOK(getLogger(), TableView_HandleCellSelectionDidChange);
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_RestartGame);
    INSTALL_HOOK(getLogger(), MainMenuModSettingsViewController_DidActivate);
    INSTALL_HOOK(getLogger(), DownloadSongsFlowCoordinator_DidActivate);
    INSTALL_HOOK(getLogger(), DownloadSongsSearchViewController_DidActivate);
    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            LoadPlaylists(customBeatmapLevelPackCollectionSO);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}
