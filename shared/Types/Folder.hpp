#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmacro-redefined"
#include "rapidjson-macros/shared/macros.hpp"
#pragma GCC diagnostic pop

DECLARE_JSON_CLASS(PlaylistManager, Folder,
    std::vector<std::string> Playlists;
    std::string FolderName;
    bool ShowDefaults;
    std::vector<PlaylistManager::Folder> Subfolders;
    bool HasSubfolders;
)