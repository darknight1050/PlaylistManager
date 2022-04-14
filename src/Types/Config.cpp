#include "Types/Config.hpp"

DESERIALIZE_METHOD(PlaylistManager, PlaylistConfig,
    DESERIALIZE_VALUE(Management, enableManagement, Bool);
    DESERIALIZE_VALUE(DownloadIcon, showDownloadIcon, Bool);
    DESERIALIZE_VALUE(RemoveMissing, removeMissingBeatSaverSongs, Bool);
    DESERIALIZE_VALUE(ScrollSpeed, scrollSpeed, Float);
    DESERIALIZE_VECTOR_BASIC(Order, order, String);
    DESERIALIZE_VECTOR(Folders, folders, Folder);
)

SERIALIZE_METHOD(PlaylistManager, PlaylistConfig,
    SERIALIZE_VALUE(Management, enableManagement);
    SERIALIZE_VALUE(DownloadIcon, showDownloadIcon);
    SERIALIZE_VALUE(RemoveMissing, removeMissingBeatSaverSongs);
    SERIALIZE_VALUE(ScrollSpeed, scrollSpeed);
    SERIALIZE_VECTOR_BASIC(Order, order);
    SERIALIZE_VECTOR(Folders, folders);
)