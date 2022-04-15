#pragma once

#include "rapidjson-macros/shared/macros.hpp"
#include "shared/Types/Folder.hpp"

DECLARE_JSON_CLASS(PlaylistManager, PlaylistConfig,
    bool Management = false;
    bool DownloadIcon = false;
    bool RemoveMissing = true;
    float ScrollSpeed = 2;
    std::vector<std::string> Order;
    std::vector<Folder> Folders;
)

// defined in Main.cpp
extern PlaylistManager::PlaylistConfig playlistConfig;
extern PlaylistManager::Folder* currentFolder;
// 0: all playlists, 1: just defaults, 2: just customs, 3: use current folder
extern int filterSelectionState;
extern bool allowInMultiplayer;

void UpdateScrollSpeed();
