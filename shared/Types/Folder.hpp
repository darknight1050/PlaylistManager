#pragma once

#include "rapidjson-macros/shared/macros.hpp"

DECLARE_JSON_CLASS(PlaylistManager, Folder,
    std::vector<std::string> Playlists;
    std::string FolderName;
    bool ShowDefaults;
    std::vector<PlaylistManager::Folder> Subfolders;
    bool HasSubfolders;
)