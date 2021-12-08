#pragma once

#include "Types/CustomListSource.hpp"

#include "custom-types/shared/coroutine.hpp"

namespace PlaylistManager {
    class PlaylistFilters {

        private:
        CustomListSource *filterList, *folderList, *playlistList;
        UnityEngine::GameObject *folderMenu, *folderListContainer, *playlistListContainer;

        custom_types::Helpers::Coroutine initCoroutine();
        void reloadFolderPlaylists();

        public:
        static PlaylistFilters* filtersInstance;

        void Init();
        void SetFoldersFilters(bool filtersVisible);
        void ReloadFolders();
    };
}