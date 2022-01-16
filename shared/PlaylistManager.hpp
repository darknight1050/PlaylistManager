#pragma once

#include <string>
#include "Types/BPList.hpp"
#include "songloader/shared/CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "UnityEngine/Sprite.hpp"

namespace PlaylistManager {

    struct Playlist {
        BPList playlistJSON;
        GlobalNamespace::CustomBeatmapLevelPack* playlistCS;
        std::string name;
        std::string path;
        int imageIndex = -1;
    };

    extern const std::unordered_set<std::string> staticPacks;

    std::string WriteImageToFile(std::string_view pathToPng, UnityEngine::Sprite* image);

    int GetPackIndex(std::string title);

    UnityEngine::Sprite* GetDefaultCoverImage();

    UnityEngine::Sprite* GetCoverImage(Playlist* playlist);

    void GetCoverImages();

    std::vector<UnityEngine::Sprite*>& GetLoadedImages();

    void ClearLoadedImages();

    bool AvailablePlaylistName(std::string title);

    void LoadPlaylists(RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh = false);

    std::vector<GlobalNamespace::CustomBeatmapLevelPack*> GetLoadedPlaylists();

    Playlist* GetPlaylist(std::string title);

    void AddPlaylist(std::string title, std::string author, UnityEngine::Sprite* coverImage);

    void MovePlaylist(Playlist* playlist, int index);

    void RenamePlaylist(Playlist* playlist, std::string title);

    void ChangePlaylistCover(Playlist* playlist, UnityEngine::Sprite* newCover, int coverIndex);

    void DeletePlaylist(Playlist* playlist);

    void RefreshPlaylists();
    
    void AddSongToPlaylist(Playlist* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level);

    void RemoveSongFromPlaylist(Playlist* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level);
}
