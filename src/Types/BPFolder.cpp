#include "shared/Types/BPFolder.hpp"

DESERIALIZE_METHOD(PlaylistManager, BPFolder,
    DESERIALIZE_VECTOR_BASIC(PlaylistNames, playlistNames, String);
    DESERIALIZE_VALUE(ShowDefaults, showDefaults, Bool);
)

SERIALIZE_METHOD(PlaylistManager, BPFolder,
    SERIALIZE_VECTOR_BASIC(PlaylistNames, playlistNames);
    SERIALIZE_VALUE(ShowDefaults, showDefaults);
)