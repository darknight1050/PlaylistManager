#include "shared/Types/Folder.hpp"

DESERIALIZE_METHOD(PlaylistManager, Folder,
    DESERIALIZE_VECTOR_BASIC(PlaylistNames, playlistNames, String);
    DESERIALIZE_VALUE(FolderName, folderName, String);
    DESERIALIZE_VALUE(ShowDefaults, showDefaults, Bool);
)

SERIALIZE_METHOD(PlaylistManager, Folder,
    SERIALIZE_VECTOR_BASIC(PlaylistNames, playlistNames);
    SERIALIZE_VALUE(FolderName, folderName);
    SERIALIZE_VALUE(ShowDefaults, showDefaults);
)