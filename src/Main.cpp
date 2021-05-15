#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "songloader/shared/API.hpp"

#include "questui/shared/QuestUI.hpp"

#include "PlaylistManager.hpp"

#include "CustomLogger.hpp"
#include "Paths.hpp"

ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

std::string GetPlaylistsPath() {
    static std::string playlistsPath(getDataDir(modInfo) + "Playlists");
    return playlistsPath;
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "PlaylistManager";
    modInfo.version = VERSION;
    info = modInfo;
    
    auto playlistsPath = GetPlaylistsPath();
    if(!direxists(playlistsPath))
        mkpath(playlistsPath);
}


extern "C" void load() {
    LOG_INFO("Starting PlaylistManager installation...");
    il2cpp_functions::Init();
    QuestUI::Init();
    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}