#pragma once

#include "TypeMacros.hpp"

DECLARE_JSON_CLASS(PlaylistManager, BPFolder,
    std::vector<std::string> PlaylistNames;
    std::string FolderName;
    bool ShowDefaults;
)