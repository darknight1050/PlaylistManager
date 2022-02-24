#pragma once

#include "TypeMacros.hpp"

DECLARE_JSON_CLASS(PlaylistManager, Folder,
    std::vector<std::string> Playlists;
    std::string FolderName;
    bool ShowDefaults;
    std::vector<PlaylistManager::Folder> Subfolders;
    bool HasSubfolders;
)