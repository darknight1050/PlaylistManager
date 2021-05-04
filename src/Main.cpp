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

extern "C" void setup(ModInfo& info) {
    modInfo.id = "PlaylistManager";
    modInfo.version = VERSION;
    info = modInfo;
    
    auto playlistsPath = GetPlaylistsPath();
    if(!direxists(playlistsPath))
        mkpath(playlistsPath);
}


#include "Types/BPList.hpp"
#include "PlaylistManager.hpp"

#include <dirent.h>
#include "System/Collections/Generic/List_1.hpp"
using namespace RuntimeSongLoader;

System::Collections::Generic::List_1<SongLoaderCustomBeatmapLevelPack*>* playlists;

extern "C" void load() {
    LOG_INFO("Starting PlaylistManager installation...");
    il2cpp_functions::Init();
    QuestUI::Init();
    API::AddRefreshLevelPacksEvent(
        [] (SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            if(!playlists)
                playlists = System::Collections::Generic::List_1<SongLoaderCustomBeatmapLevelPack*>::New_ctor();
            LOG_INFO("playlists");
            std::string fullPath(GetPlaylistsPath());
            DIR *dir;
            struct dirent *ent;
            if((dir = opendir(fullPath.c_str())) != nullptr) {
                while((ent = readdir(dir)) != nullptr) {
                    std::string name = ent->d_name;
                    if(name != "." && name != "..") {
                        auto listOpt = PlaylistManager::ReadFromFile(GetPlaylistsPath() + "/" + name);
                        if(listOpt.has_value()) {
                            auto list = listOpt.value();
                            SongLoaderCustomBeatmapLevelPack* customBeatmapLevelPack = nullptr;
                            for(int i = 0; i < playlists->get_Count(); i++) {
                                auto pack = playlists->get_Item(i);
                                if(to_utf8(csstrtostr(pack->CustomLevelsPack->packName)) == list.GetPlaylistTitle()) {
                                    customBeatmapLevelPack = pack;
                                    break;
                                }
                            }
                            if(!customBeatmapLevelPack) {
                                customBeatmapLevelPack = SongLoaderCustomBeatmapLevelPack::New_ctor(list.GetPlaylistTitle(), list.GetPlaylistTitle());
                                playlists->Add(customBeatmapLevelPack);
                            }
                            auto foundSongs = List<GlobalNamespace::CustomPreviewBeatmapLevel*>::New_ctor();
                            for(auto& song : list.GetSongs()) {
                                auto search = API::GetLevelByHash(song.GetHash());
                                if(search.has_value())
                                    foundSongs->Add(search.value());
                            }
                            customBeatmapLevelPack->SetCustomPreviewBeatmapLevels(foundSongs->ToArray());
                            customBeatmapLevelPack->AddTo(customBeatmapLevelPackCollectionSO, true);
                        }
                    }
                }
                closedir(dir);
            }
            
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}