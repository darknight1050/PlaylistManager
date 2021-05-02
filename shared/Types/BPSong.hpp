#pragma once
#include "TypeMacros.hpp"

DECLARE_JSON_CLASS(PlaylistManager, BPSong, 
    GETTER_VALUE(std::string, Hash);
    GETTER_VALUE_OPTIONAL(std::string, SongName);
    GETTER_VALUE_OPTIONAL(std::string, Key);
)
