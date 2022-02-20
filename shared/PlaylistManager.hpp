#pragma once

#include <string>
#include "Types/BPList.hpp"
#include "songloader/shared/CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "songloader/shared/CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Texture2D.hpp"

namespace PlaylistManager {

    struct Playlist {
        BPList playlistJSON;
        SafePtr<GlobalNamespace::CustomBeatmapLevelPack> playlistCS;
        std::string name;
        std::string path;
        int imageIndex = -1;
    };

    extern bool hasLoaded;

    extern std::unordered_set<std::string> staticPackIDs;

    UnityEngine::Sprite* GetDefaultCoverImage();

    UnityEngine::Sprite* GetCoverImage(Playlist* playlist);

    void DeleteLoadedImage(UnityEngine::Sprite* image);

    void LoadCoverImages();

    std::vector<UnityEngine::Sprite*>& GetLoadedImages();

    void ClearLoadedImages();

    void LoadPlaylists(RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullReload = false);

    std::vector<Playlist*> GetLoadedPlaylists();

    Playlist* GetPlaylist(std::string const& path);

    Playlist* GetPlaylistWithPrefix(std::string const& id);

    int GetPlaylistIndex(std::string const& path);

    void AddPlaylist(std::string const& title, std::string const& author, UnityEngine::Sprite* coverImage);

    void MovePlaylist(Playlist* playlist, int index);

    void RenamePlaylist(Playlist* playlist, std::string const& title);

    void ChangePlaylistCover(Playlist* playlist, int coverIndex);

    void DeletePlaylist(Playlist* playlist);

    void ReloadPlaylists(bool fullReload = false);
    
    void MarkPlaylistForReload(Playlist* playlist);

    void AddSongToPlaylist(Playlist* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level);

    void RemoveSongFromPlaylist(Playlist* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level);
}
