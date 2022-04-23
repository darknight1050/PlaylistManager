#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmacro-redefined"
#include "rapidjson-macros/shared/macros.hpp"
#pragma GCC diagnostic pop

DECLARE_JSON_CLASS(PlaylistManager, Difficulty,
    std::string Characteristic;
    std::string Name;
)

DECLARE_JSON_CLASS(PlaylistManager, BPSong,
    std::string Hash;
    std::optional<std::string> SongName;
    std::optional<std::string> Key;
    std::optional<std::vector<PlaylistManager::Difficulty>> Difficulties;
)

DECLARE_JSON_CLASS(PlaylistManager, CustomData,
    std::optional<std::string> SyncURL;
)

DECLARE_JSON_CLASS(PlaylistManager, BPList,
    std::string PlaylistTitle;
    std::optional<std::string> PlaylistAuthor;
    std::optional<std::string> PlaylistDescription;
    std::vector<PlaylistManager::BPSong> Songs;
    std::optional<std::string> ImageString;
    std::optional<PlaylistManager::CustomData> CustomData;
)