#pragma once

#include "TypeMacros.hpp"

DECLARE_JSON_CLASS(PlaylistManager, Folder,
    std::vector<std::string> PlaylistNames;
    std::string FolderName;
    bool ShowDefaults;
    std::optional<std::vector<PlaylistManager::Folder>> Subfolders;
    bool HasSubfolders;
)