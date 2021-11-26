#pragma once

#include <string>
#include "Types/BPList.hpp"
#include "songloader/shared/CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "UnityEngine/Sprite.hpp"

namespace PlaylistManager {

    extern const std::unordered_set<std::string> staticPacks;
    extern std::vector<UnityEngine::Sprite*> loadedImages;

    std::string WriteImageToFile(std::string_view pathToPng, UnityEngine::Sprite* image);

    bool ReadFromFile(std::string_view path, JSONClass& toDeserialize);

    bool WriteToFile(std::string_view path, JSONClass& toSerialize);

    BPList* GetPlaylistJSON(std::string title);

    int GetPackIndex(std::string title);

    RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetSongloaderPack(BPList* playlist);

    UnityEngine::Sprite* GetDefaultCoverImage();

    UnityEngine::Sprite* GetCoverImage(BPList* playlist);

    void GetCoverImages();

    void LoadPlaylists(RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh = false);

    void MovePlaylist(BPList* playlist, int index);

    void RenamePlaylist(BPList* playlist, std::string title);

    void ChangePlaylistCover(BPList* playlist, UnityEngine::Sprite* newCover, int coverIndex);

    void RefreshPlaylists();
    
}
