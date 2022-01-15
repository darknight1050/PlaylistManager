#pragma once

#include <string>
#include "Types/BPList.hpp"
#include "songloader/shared/CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
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

    std::vector<GlobalNamespace::CustomBeatmapLevelPack*> GetLoadedPlaylists();

    UnityEngine::Sprite* GetDefaultCoverImage();

    UnityEngine::Sprite* GetCoverImage(BPList* playlist);

    void GetCoverImages();

    void ClearLoadedImages();

    bool AvailablePlaylistName(std::string title);

    void LoadPlaylists(RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh = false);

    void AddPlaylist(std::string title, std::string author, UnityEngine::Sprite* coverImage);

    void MovePlaylist(BPList* playlist, int index);

    void RenamePlaylist(BPList* playlist, std::string title);

    void ChangePlaylistCover(BPList* playlist, UnityEngine::Sprite* newCover, int coverIndex);

    void DeletePlaylist(std::string title);

    void RefreshPlaylists();
    
    void AddSongToPlaylist(BPList* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level);

    void RemoveSongFromPlaylist(BPList* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level);
}
