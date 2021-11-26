#include "Types/Config.hpp"

DESERIALIZE_METHOD(PlaylistManager, PlaylistConfig,
    DESERIALIZE_VECTOR_BASIC(Order, order, String);
    DESERIALIZE_VECTOR(Folders, folders, BPFolder);
)

SERIALIZE_METHOD(PlaylistManager, PlaylistConfig,
    SERIALIZE_VECTOR_BASIC(Order, order);
    SERIALIZE_VECTOR(Folders, folders);
)