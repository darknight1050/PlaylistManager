#pragma once

#include "TypeMacros.hpp"

#include <vector>

DECLARE_JSON_CLASS(PlaylistManager, BPSong,
    std::string Hash;
    std::optional<std::string> SongName;
    std::optional<std::string> Key;
)

DECLARE_JSON_CLASS(PlaylistManager, BPList,
    std::string PlaylistTitle;
    std::optional<std::string> PlaylistAuthor;
    std::optional<std::string> PlaylistDescription;
    std::vector<PlaylistManager::BPSong> Songs;
    std::optional<std::string> ImageString;
    // not serialized
    int imageIndex = -1;
    std::string path;
)