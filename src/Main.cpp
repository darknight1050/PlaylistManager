#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "songloader/shared/API.hpp"
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "questui/shared/QuestUI.hpp"

#include "CustomLogger.hpp"

ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

std::string GetPlaylistsPath() {
    static std::string playlistsPath(getDataDir(modInfo) + "/Playlists");
    return playlistsPath;
}

#include "Types/BPList.hpp"
#include "PlaylistManager.hpp"

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
            LOG_INFO("songsLoaded");
            

            auto list = *PlaylistManager::ReadFromFile(GetPlaylistsPath() + "/test.json");
            auto pack = RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack::New_ctor(list.GetPlaylistTitle(), list.GetPlaylistTitle());
            auto stuff = Array<GlobalNamespace::CustomPreviewBeatmapLevel*>::NewLength(1);
            stuff->values[0] = RuntimeSongLoader::API::GetLoadedSongs()[0];
            pack->SetCustomPreviewBeatmapLevels(stuff);
            pack->AddTo(customBeatmapLevelPackCollectionSO, false);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}