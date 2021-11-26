#include "Types/BPList.hpp"

DESERIALIZE_METHOD(PlaylistManager, BPSong,
    DESERIALIZE_VALUE(Hash, hash, String)
    DESERIALIZE_VALUE_OPTIONAL(SongName, songName, String)
    DESERIALIZE_VALUE_OPTIONAL(Key, key, String)
)

SERIALIZE_METHOD(PlaylistManager, BPSong,
    SERIALIZE_VALUE(Hash, hash)
    SERIALIZE_VALUE_OPTIONAL(SongName, songName)
    SERIALIZE_VALUE_OPTIONAL(Key, key)
)

DESERIALIZE_METHOD(PlaylistManager, BPList,
    DESERIALIZE_VALUE(PlaylistTitle, playlistTitle, String);
    DESERIALIZE_VALUE_OPTIONAL(PlaylistAuthor, playlistAuthor, String);
    DESERIALIZE_VALUE_OPTIONAL(PlaylistDescription, playlistDescription, String);
    DESERIALIZE_VECTOR(Songs, songs, BPSong);
    DESERIALIZE_VALUE_OPTIONAL(ImageString, image, String);
)

SERIALIZE_METHOD(PlaylistManager, BPList,
    SERIALIZE_VALUE(PlaylistTitle, playlistTitle);
    SERIALIZE_VALUE_OPTIONAL(PlaylistAuthor, playlistAuthor);
    SERIALIZE_VALUE_OPTIONAL(PlaylistDescription, playlistDescription);
    SERIALIZE_VECTOR(Songs, songs);
    SERIALIZE_VALUE_OPTIONAL(ImageString, image);
)