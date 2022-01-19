#pragma once

#include "shared/Types/TypeMacros.hpp"
#include "shared/Types/Folder.hpp"

DECLARE_JSON_CLASS(PlaylistManager, PlaylistConfig,
    bool Management = false;
    std::vector<std::string> Order;
    std::vector<Folder> Folders;
)

extern PlaylistManager::PlaylistConfig playlistConfig;
extern PlaylistManager::Folder* currentFolder;
// 0: all playlists, 1: just defaults, 2: just customs, 3: use current folder
extern int folderSelectionState;