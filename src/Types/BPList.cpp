#include "Types/BPList.hpp"

DESERIALIZE_METHOD(PlaylistManager, Difficulty,
    DESERIALIZE_VALUE(Characteristic, characteristic, String);
    DESERIALIZE_VALUE(Name, name, String);
)

SERIALIZE_METHOD(PlaylistManager, Difficulty,
    SERIALIZE_VALUE(Characteristic, characteristic);
    SERIALIZE_VALUE(Name, name);
)

DESERIALIZE_METHOD(PlaylistManager, BPSong,
    DESERIALIZE_VALUE(Hash, hash, String);
    DESERIALIZE_VALUE_OPTIONAL(SongName, songName, String);
    DESERIALIZE_VALUE_OPTIONAL(Key, key, String);
    DESERIALIZE_VECTOR_OPTIONAL(Difficulties, difficulties, Difficulty);
)

SERIALIZE_METHOD(PlaylistManager, BPSong,
    SERIALIZE_VALUE(Hash, hash);
    SERIALIZE_VALUE_OPTIONAL(SongName, songName);
    SERIALIZE_VALUE_OPTIONAL(Key, key);
    SERIALIZE_VECTOR_OPTIONAL(Difficulties, difficulties);
)

DESERIALIZE_METHOD(PlaylistManager, CustomData,
    DESERIALIZE_VALUE_OPTIONAL(SyncURL, syncURL, String);
    DESERIALIZE_VALUE_OPTIONAL(CustomArchiveURL, customArchiveUrl, String);
    DESERIALIZE_VALUE_OPTIONAL(AllowDuplicates, AllowDuplicates, Bool);
    DESERIALIZE_VALUE_OPTIONAL(ReadOnly, ReadOnly, Bool);
)

SERIALIZE_METHOD(PlaylistManager, CustomData,
    SERIALIZE_VALUE_OPTIONAL(SyncURL, syncURL);
    SERIALIZE_VALUE_OPTIONAL(CustomArchiveURL, customArchiveUrl);
    SERIALIZE_VALUE_OPTIONAL(AllowDuplicates, AllowDuplicates);
    SERIALIZE_VALUE_OPTIONAL(ReadOnly, ReadOnly);
)

DESERIALIZE_METHOD(PlaylistManager, BPList,
    DESERIALIZE_VALUE(PlaylistTitle, playlistTitle, String);
    DESERIALIZE_VALUE_OPTIONAL(PlaylistAuthor, playlistAuthor, String);
    DESERIALIZE_VALUE_OPTIONAL(PlaylistDescription, playlistDescription, String);
    DESERIALIZE_VECTOR(Songs, songs, BPSong);
    DESERIALIZE_VALUE_OPTIONAL(ImageString, image, String);
    DESERIALIZE_CLASS_OPTIONAL(CustomData, customData);
    if(jsonValue.HasMember("downloadURL") && jsonValue["downloadURL"].IsString()) {
        if(!CustomData.has_value())
            CustomData.emplace();
        CustomData->SyncURL = jsonValue["downloadURL"].GetString();
    }
)

SERIALIZE_METHOD(PlaylistManager, BPList,
    SERIALIZE_VALUE(PlaylistTitle, playlistTitle);
    SERIALIZE_VALUE_OPTIONAL(PlaylistAuthor, playlistAuthor);
    SERIALIZE_VALUE_OPTIONAL(PlaylistDescription, playlistDescription);
    SERIALIZE_VECTOR(Songs, songs);
    SERIALIZE_VALUE_OPTIONAL(ImageString, image);
    SERIALIZE_CLASS_OPTIONAL(CustomData, customData);
)