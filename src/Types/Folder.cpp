#include "shared/Types/Folder.hpp"

DESERIALIZE_METHOD(PlaylistManager, Folder,
    DESERIALIZE_VECTOR_BASIC(Playlists, playlists, String);
    DESERIALIZE_VALUE(FolderName, folderName, String);
    DESERIALIZE_VALUE(ShowDefaults, showDefaults, Bool);
    DESERIALIZE_VECTOR(Subfolders, subfolders, Folder)
    DESERIALIZE_VALUE(HasSubfolders, hasSubfolders, Bool);
)

SERIALIZE_METHOD(PlaylistManager, Folder,
    SERIALIZE_VECTOR_BASIC(Playlists, playlists);
    SERIALIZE_VALUE(FolderName, folderName);
    SERIALIZE_VALUE(ShowDefaults, showDefaults);
    SERIALIZE_VECTOR(Subfolders, subfolders);
    SERIALIZE_VALUE(HasSubfolders, hasSubfolders);
)