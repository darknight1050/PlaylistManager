#pragma once
#include <string>
#include "Types/BPList.hpp"
#include "songloader/shared/CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"

namespace PlaylistManager {

    std::optional<BPList> ReadFromFile(std::string_view path);

    void LoadPlaylists(RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh = false);

}
