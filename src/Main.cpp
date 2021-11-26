#include "Types/PlaylistMenu.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"
#include "Main.hpp"

#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "songloader/shared/API.hpp"

#include "questui/shared/QuestUI.hpp"

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

ModInfo modInfo;

PlaylistManager::PlaylistMenu* playlistMenu;
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

MAKE_HOOK_MATCH(LevelPackDetailViewController_ShowContent, &GlobalNamespace::LevelPackDetailViewController::ShowContent,
        void, GlobalNamespace::LevelPackDetailViewController* self, GlobalNamespace::LevelPackDetailViewController::ContentType contentType, ::Il2CppString* errorText) {
    using namespace GlobalNamespace;
    LevelPackDetailViewController_ShowContent(self, contentType, errorText);

    if(contentType == LevelPackDetailViewController::ContentType::Owned && self->pack->get_packID()->Contains(CSTR("custom_levelPack"))
        && !PlaylistManager::staticPacks.contains(STR(self->pack->get_packName()))) {
        // find playlist json
        auto json = PlaylistManager::GetPlaylistJSON(STR(self->pack->get_packName()));
        // create menu if necessary, if so avoid visibility calls
        bool construction = false;
        if(!playlistMenu) {
            playlistMenu = self->get_gameObject()->AddComponent<PlaylistManager::PlaylistMenu*>();
            playlistMenu->Init(self->detailWrapper, json);
        } else {
            if(json) {
                playlistMenu->SetPlaylist(json);
                playlistMenu->SetVisible(true);
            } else
                playlistMenu->SetVisible(false);
        }
    } else if(playlistMenu) {
        playlistMenu->SetVisible(false);
    }
}

MAKE_HOOK_MATCH(InputFieldView_DeactivateKeyboard, &HMUI::InputFieldView::DeactivateKeyboard, void, HMUI::InputFieldView* self, HMUI::UIKeyboard* keyboard) {
    InputFieldView_DeactivateKeyboard(self, keyboard);
    if(PlaylistManager::PlaylistMenu::nextCloseKeyboard) {
        PlaylistManager::PlaylistMenu::nextCloseKeyboard();
        PlaylistManager::PlaylistMenu::nextCloseKeyboard = nullptr;
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
    INSTALL_HOOK(getLogger(), LevelPackDetailViewController_ShowContent);
    INSTALL_HOOK(getLogger(), InputFieldView_DeactivateKeyboard);
    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}