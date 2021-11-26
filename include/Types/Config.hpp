#pragma once

#include "shared/Types/TypeMacros.hpp"
#include "shared/Types/BPFolder.hpp"

DECLARE_JSON_CLASS(PlaylistManager, PlaylistConfig,
    std::vector<std::string> Order;
    std::vector<BPFolder> Folders;
)

extern PlaylistManager::PlaylistConfig playlistConfig;
extern PlaylistManager::BPFolder* currentFolder;
// 0: all playlists, 1: just defaults, 2: just customs, 3: use current folder
extern int folderSelectionState;