#pragma once
#include "BPSong.hpp"
#include "TypeMacros.hpp"

#include <vector>

DECLARE_JSON_CLASS(PlaylistManager, BPList,
    GETTER_VALUE(std::string, PlaylistTitle);
    GETTER_VALUE_OPTIONAL(std::string, PlaylistAuthor);
    GETTER_VALUE_OPTIONAL(std::string, PlaylistDescription);
    GETTER_VALUE(std::vector<PlaylistManager::BPSong>, Songs);
    GETTER_VALUE_OPTIONAL(std::string, ImageString);
)