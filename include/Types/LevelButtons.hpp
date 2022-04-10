#pragma once

#include "custom-types/shared/coroutine.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/Button.hpp"

#include "HMUI/ModalView.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"

namespace PlaylistManager {
    class Playlist;

    class ButtonsContainer {

        private:
        UnityEngine::GameObject *layoutObject, *infoButtonContainer;
        UnityEngine::UI::Button *saveCoverButton, *playlistAddButton, *playlistRemoveButton;
        HMUI::ModalView *playlistAddModal, *infoModal;
        TMPro::TextMeshProUGUI *infoText;
        class CustomListSource *playlistCovers;
        GlobalNamespace::StandardLevelDetailView *levelDetailView;
        GlobalNamespace::LevelCollectionTableView *levelListTableView;
        GlobalNamespace::IPreviewBeatmapLevel* currentLevel;
        Playlist* currentPlaylist = nullptr;

        std::vector<Playlist*> loadedPlaylists;

        custom_types::Helpers::Coroutine initCoroutine();

        void saveCoverButtonPressed();
        void addToPlaylistButtonPressed();
        void removeFromPlaylistButtonPressed();
        void playlistSelected(int listCellIdx);
        void scrollListLeftButtonPressed();
        void scrollListRightButtonPressed();
        void confirmRemovalButtonPressed();
        void cancelRemovalButtonPressed();

        public:
        static ButtonsContainer* buttonsInstance;

        void Init(GlobalNamespace::StandardLevelDetailView* levelDetailView);
        void SetVisible(bool visible, bool showRemove);
        void SetLevel(GlobalNamespace::IPreviewBeatmapLevel* level);
        void SetPlaylist(Playlist* playlist);
        void RefreshPlaylists();
        void RefreshHighlightedDifficulties();
        void Destroy();
    };
}