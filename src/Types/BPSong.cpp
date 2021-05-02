#include "Types/BPSong.hpp"

DESERIALIZE_METHOD(PlaylistManager, BPSong,
    DESERIALIZE_VALUE(Hash, hash, String)
    DESERIALIZE_VALUE_OPTIONAL(SongName, songName, String)
    DESERIALIZE_VALUE_OPTIONAL(Key, key, String)
)