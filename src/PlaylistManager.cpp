#include "PlaylistManager.hpp"

#include <filesystem>

#include "CustomLogger.hpp"
#include "Paths.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "songloader/shared/API.hpp"
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "System/Collections/Generic/Dictionary_2.hpp"

using namespace RuntimeSongLoader;

namespace PlaylistManager {

    SafePtr<System::Collections::Generic::Dictionary_2<Il2CppString*, SongLoaderCustomBeatmapLevelPack*>> playlists;

    std::optional<BPList> ReadFromFile(std::string_view path) {
        if(!fileexists(path))
            return std::nullopt;
        auto json = readfile(path);
        rapidjson::Document document;
        document.Parse(json);
        if(document.HasParseError() || !document.IsObject())
            return std::nullopt;
        try {
            BPList list;
            list.Deserialize(document.GetObject());
            return list;
        } catch (const char* msg) {
            LOG_ERROR("Error loading playlist %s: %s", path.data(), msg);
        }
        return std::nullopt;
    }
    
    void LoadPlaylists(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh) {
        if(!playlists)
            playlists = System::Collections::Generic::Dictionary_2<Il2CppString*, SongLoaderCustomBeatmapLevelPack*>::New_ctor();
        if(fullRefresh)
            playlists->Clear();
        auto path = GetPlaylistsPath();
        if(!std::filesystem::is_directory(path))
            return;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if(!entry.is_directory()) {
                auto path = entry.path().string();
                auto pathCS = il2cpp_utils::newcsstr(path);
                if(playlists->ContainsKey(pathCS)) {
                    LOG_INFO("Loading playlist file %s from cache", path.c_str());
                    playlists->get_Item(pathCS)->AddTo(customBeatmapLevelPackCollectionSO, true);
                } else {
                    LOG_INFO("Loading playlist file %s", path.c_str());
                    auto listOpt = ReadFromFile(path);
                    if(listOpt.has_value()) {
                        auto list = listOpt.value();
                        UnityEngine::Sprite* coverImage = nullptr;
                        if(list.GetImageString().has_value()) {
                            std::string imageBase64 = list.GetImageString().value();
                            static std::string searchString = "base64,";
                            auto searchIndex = imageBase64.find(searchString);
                            if(searchIndex != std::string::npos)
                                imageBase64 = imageBase64.substr(searchIndex + searchString.length());
                            coverImage = QuestUI::BeatSaberUI::Base64ToSprite(imageBase64);
                        }
                        SongLoaderCustomBeatmapLevelPack* customBeatmapLevelPack = SongLoaderCustomBeatmapLevelPack::New_ctor(list.GetPlaylistTitle(), list.GetPlaylistTitle(), coverImage);
                        playlists->Add(pathCS, customBeatmapLevelPack);
                        auto foundSongs = List<GlobalNamespace::CustomPreviewBeatmapLevel*>::New_ctor();
                        for(auto& song : list.GetSongs()) {
                            auto search = RuntimeSongLoader::API::GetLevelByHash(song.GetHash());
                            if(search.has_value())
                                foundSongs->Add(search.value());
                        }
                        customBeatmapLevelPack->SetCustomPreviewBeatmapLevels(foundSongs->ToArray());
                        customBeatmapLevelPack->AddTo(customBeatmapLevelPackCollectionSO, true);
                    }
                }
            }
        }
    }

}
