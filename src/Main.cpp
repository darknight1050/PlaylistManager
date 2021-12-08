#include "Types/PlaylistMenu.hpp"
#include "Types/PlaylistFilters.hpp"
#include "Types/LevelButtons.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"
#include "Main.hpp"

#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "songloader/shared/API.hpp"

#include "questui/shared/QuestUI.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController.hpp"
#include "GlobalNamespace/LevelPackDetailViewController_ContentType.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp" // This needs to be included before RectTransform
#include "UnityEngine/RectTransform.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/ViewController_AnimationType.hpp"
#include "HMUI/InputFieldView.hpp"
#include "System/Tuple_2.hpp"
#include "System/Threading/CancellationToken.hpp"

using namespace GlobalNamespace;

ModInfo modInfo;

// shared config data
PlaylistManager::PlaylistConfig playlistConfig;
PlaylistManager::BPFolder* currentFolder;
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
MAKE_HOOK_MATCH(TableView_GetVisibleCellsIdRange, &HMUI::TableView::GetVisibleCellsIdRange, TupleType*, HMUI::TableView* self) {
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
MAKE_HOOK_MATCH(InputFieldView_DeactivateKeyboard, &HMUI::InputFieldView::DeactivateKeyboard, void, HMUI::InputFieldView* self, HMUI::UIKeyboard* keyboard) {
    InputFieldView_DeactivateKeyboard(self, keyboard);
    if(PlaylistManager::PlaylistMenu::nextCloseKeyboard) {
        PlaylistManager::PlaylistMenu::nextCloseKeyboard();
        PlaylistManager::PlaylistMenu::nextCloseKeyboard = nullptr;
    }
}

// when to show the playlist menu
MAKE_HOOK_MATCH(LevelPackDetailViewController_ShowContent, &LevelPackDetailViewController::ShowContent,
        void, LevelPackDetailViewController* self, LevelPackDetailViewController::ContentType contentType, ::Il2CppString* errorText) {
    using namespace PlaylistManager;
    LevelPackDetailViewController_ShowContent(self, contentType, errorText);

    if(contentType == LevelPackDetailViewController::ContentType::Owned && self->pack->get_packID()->Contains(CSTR("custom_levelPack"))
        && !staticPacks.contains(STR(self->pack->get_packName()))) {
        // find playlist json
        auto json = GetPlaylistJSON(STR(self->pack->get_packName()));
        // create menu if necessary, if so avoid visibility calls
        bool construction = false;
        if(!PlaylistMenu::menuInstance) {
            auto playlistMenu = self->get_gameObject()->AddComponent<PlaylistMenu*>();
            playlistMenu->Init(self->detailWrapper, json);
        } else {
            if(json) {
                PlaylistMenu::menuInstance->SetPlaylist(json);
                PlaylistMenu::menuInstance->SetVisible(true);
            } else
                PlaylistMenu::menuInstance->SetVisible(false);
        }
    } else if(PlaylistMenu::menuInstance) {
        PlaylistMenu::menuInstance->SetVisible(false);
    }
}

// when to show the level buttons
MAKE_HOOK_MATCH(StandardLevelDetailViewController_LoadBeatmapLevelAsync, &StandardLevelDetailViewController::LoadBeatmapLevelAsync, 
        System::Threading::Tasks::Task*, StandardLevelDetailViewController* self, System::Threading::CancellationToken cancellationToken) {
    LOG_INFO("Loading level");
    auto ret = StandardLevelDetailViewController_LoadBeatmapLevelAsync(self, cancellationToken);
    
    using namespace PlaylistManager;
    if(!ButtonsContainer::buttonsInstance) {
        ButtonsContainer::buttonsInstance = new ButtonsContainer();
        ButtonsContainer::buttonsInstance->Init(self->standardLevelDetailView);
    }
    std::string name = STR(self->pack->get_packName());
    bool customPack = !staticPacks.contains(name);
    bool customSong = customPack || name == "Custom Levels" || name == "Custom WIP Levels";
    ButtonsContainer::buttonsInstance->SetVisible(customSong, customPack);
    if(customSong)
        ButtonsContainer::buttonsInstance->SetLevel(self->previewBeatmapLevel);
    if(customPack)
        ButtonsContainer::buttonsInstance->SetPack(reinterpret_cast<CustomBeatmapLevelPack*>(self->pack));
    return ret;
}

// when to set up the folders
MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    using namespace PlaylistManager;
    if(!PlaylistFilters::filtersInstance) {
        PlaylistFilters::filtersInstance = new PlaylistFilters();
        PlaylistFilters::filtersInstance->Init();
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
    if(fileexists(configPath)) {
        PlaylistManager::ReadFromFile(configPath, playlistConfig);
    }
}

extern "C" void load() {
    LOG_INFO("Starting PlaylistManager installation...");
    il2cpp_functions::Init();
    QuestUI::Init();
    INSTALL_HOOK(getLogger(), TableView_GetVisibleCellsIdRange);
    INSTALL_HOOK(getLogger(), InputFieldView_DeactivateKeyboard);
    INSTALL_HOOK(getLogger(), LevelPackDetailViewController_ShowContent);
    INSTALL_HOOK(getLogger(), StandardLevelDetailViewController_LoadBeatmapLevelAsync);
    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}
