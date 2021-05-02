#include "Types/BPList.hpp"

DESERIALIZE_METHOD(PlaylistManager, BPList,
    DESERIALIZE_VALUE(PlaylistTitle, playlistTitle, String);
    DESERIALIZE_VALUE_OPTIONAL(PlaylistAuthor, playlistAuthor, String);
    DESERIALIZE_VALUE_OPTIONAL(PlaylistDescription, playlistDescription, String);
    DESERIALIZE_VECTOR(Songs, songs, BPSong);
    DESERIALIZE_VALUE_OPTIONAL(ImageString, image, String);
)