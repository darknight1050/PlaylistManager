#include "Main.hpp"
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

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp" // This needs to be included before RectTransform
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
// #include "UnityEngine/Events/InvokableCallList.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "HMUI/InputFieldView.hpp"
#include "Zenject/DiContainer.hpp"
#include "System/Tuple_2.hpp"
#include "System/Action_1.hpp"

using namespace GlobalNamespace;
using namespace PlaylistManager;

ModInfo modInfo;

// shared config data
PlaylistConfig playlistConfig;
Folder* currentFolder = nullptr;
int folderSelectionState = 0;

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

    staticPacks = { "Custom Levels", "WIP Levels" };
    for(auto& levelPack : self->allOfficialBeatmapLevelPacks) {
        staticPacks.emplace(levelPack->get_packName());
    }
}

// prevent download icon showing up on empty custom playlists
MAKE_HOOK_MATCH(AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync, &AnnotatedBeatmapLevelCollectionCell::RefreshAvailabilityAsync,
        void, AnnotatedBeatmapLevelCollectionCell* self, AdditionalContentModel* contentModel) {
    
    AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync(self, contentModel);

    auto pack = il2cpp_utils::try_cast<IBeatmapLevelPack>(self->annotatedBeatmapLevelCollection);
    if(pack.has_value()) {
        if(!staticPacks.contains(pack.value()->get_packName()))
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

    static ConstString customPackName("custom_levelPack");

    if(contentType == LevelPackDetailViewController::ContentType::Owned && self->pack->get_packID()->Contains(customPackName) && !staticPacks.contains(self->pack->get_packName())) {
        // find playlist json
        auto playlist = GetPlaylist(self->pack->get_packName());
        // create menu if necessary, if so avoid visibility calls
        bool construction = false;
        if(!PlaylistMenu::menuInstance && playlist) {
            auto playlistMenu = self->get_gameObject()->AddComponent<PlaylistMenu*>();
            playlistMenu->Init(self->packImage, playlist);
        } else {
            if(playlist) {
                PlaylistMenu::menuInstance->SetPlaylist(playlist);
                PlaylistMenu::menuInstance->SetVisible(true);
            } else
                PlaylistMenu::menuInstance->SetVisible(false);
        }
    } else if(PlaylistMenu::menuInstance) {
        PlaylistMenu::menuInstance->SetVisible(false);
    }

    // disable level buttons (hides modal if necessary)
    if(ButtonsContainer::buttonsInstance) {
        ButtonsContainer::buttonsInstance->SetVisible(false, false);
    }
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
    std::string name = self->pack->get_packName();
    bool customPack = !staticPacks.contains(name);
    bool customSong = customPack || name == "Custom Levels" || name == "WIP Levels";
    ButtonsContainer::buttonsInstance->SetVisible(customSong, customPack);
    ButtonsContainer::buttonsInstance->SetLevel((IPreviewBeatmapLevel*) self->beatmapLevel);
    ButtonsContainer::buttonsInstance->SetPlaylist(GetPlaylist(name));
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

// throw away objects on a soft restart
MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &MenuTransitionsHelper::RestartGame,
        void, MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback) {
    
    DestroyUI();
    
    ResettableStaticPtr::resetAll();

    ClearLoadedImages();

    hasLoaded = false;

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
                RefreshPlaylists(true);
            }));
        }
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
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_RestartGame);
    INSTALL_HOOK(getLogger(), MainMenuModSettingsViewController_DidActivate);
    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            LoadPlaylists(customBeatmapLevelPackCollectionSO);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}
