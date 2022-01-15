#pragma once

#include "custom-types/shared/coroutine.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/Button.hpp"

#include "HMUI/ModalView.hpp"

#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"

namespace PlaylistManager {
    class ButtonsContainer {

        private:
        UnityEngine::GameObject* layoutObject;
        UnityEngine::UI::Button *saveCoverButton, *playlistAddButton, *playlistRemoveButton;
        HMUI::ModalView *playlistAddModal;
        class CustomListSource *playlistCovers;
        GlobalNamespace::StandardLevelDetailView *levelDetailView;
        GlobalNamespace::LevelCollectionTableView *levelListTableView;
        GlobalNamespace::IPreviewBeatmapLevel* currentLevel;
        GlobalNamespace::CustomBeatmapLevelPack* currentPack;

        std::vector<GlobalNamespace::CustomBeatmapLevelPack*> loadedPacks;

        custom_types::Helpers::Coroutine initCoroutine();

        void saveCoverButtonPressed();
        void addToPlaylistButtonPressed();
        void removeFromPlaylistButtonPressed();
        void playlistSelected(int listCellIdx);
        void scrollListLeftButtonPressed();
        void scrollListRightButtonPressed();

        public:
        static ButtonsContainer* buttonsInstance;

        void Init(GlobalNamespace::StandardLevelDetailView* levelDetailView);
        void SetVisible(bool visible, bool showRemove);
        void SetLevel(GlobalNamespace::IPreviewBeatmapLevel* level);
        void SetPack(GlobalNamespace::CustomBeatmapLevelPack* pack);
        void RefreshPlaylists();
        void Destroy();
    };
}