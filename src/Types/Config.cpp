#include "Types/Config.hpp"

DESERIALIZE_METHOD(PlaylistManager, PlaylistConfig,
    DESERIALIZE_VALUE(Management, enableManagement, Bool);
    DESERIALIZE_VECTOR_BASIC(Order, order, String);
    DESERIALIZE_VECTOR(Folders, folders, Folder);
)

SERIALIZE_METHOD(PlaylistManager, PlaylistConfig,
    SERIALIZE_VALUE(Management, enableManagement);
    SERIALIZE_VECTOR_BASIC(Order, order);
    SERIALIZE_VECTOR(Folders, folders);
)