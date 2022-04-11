#pragma once

#include "Types/CustomListSource.hpp"
#include "Types/Folder.hpp"

#include "custom-types/shared/coroutine.hpp"

#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/Toggle.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "HMUI/InputFieldView.hpp"

namespace PlaylistManager {
    class Playlist;

    class PlaylistFilters {

        enum struct State {
            filters,
            folders,
            editing,
            creating
        };

        private:
        CustomListSource *filterList, *folderList, *playlistList;
        UnityEngine::GameObject *folderMenu, *folderEditMenu, *playlistListContainer, *canvas;
        UnityEngine::RectTransform *topLayout;
        UnityEngine::UI::Button *editButton, *deleteButton, *editCreateButton;
        HMUI::InputFieldView *titleField;
        UnityEngine::UI::Toggle *subfoldersToggle, *defaultsToggle;
        TMPro::TextMeshProUGUI *folderTitle, *noFoldersInfo, *noSubfoldersInfo;

        bool currentSubfolders, currentDefaults;
        std::string currentTitle;
        std::vector<std::string> currentPlaylists;

        std::vector<Folder*> parentFolders;
        std::vector<Folder*> currentFolderList;

        std::vector<Playlist*> loadedPlaylists;

        State state;

        custom_types::Helpers::Coroutine initCoroutine();
        void refreshFolderPlaylists();
        
        void setFoldersFilters(bool filtersVisible);
        void setFolderEdit(bool editing);
        void selectFolder(Folder& folder);
        void deselectFolder();

        void filterSelected(int filter);
        void folderSelected(int listCellIdx);
        void backButtonPressed();
        void folderTitleTyped(std::string const& newTitle);
        void editMenuCreateButtonPressed();
        void editButtonPressed();
        void deleteButtonPressed();
        void createButtonPressed();
        void subfoldersToggled(bool enabled);
        void defaultsToggled(bool enabled);
        void playlistSelected(int cellIdx);
        void playlistDeselected(int cellIdx);
        void scrollFolderListLeftButtonPressed();
        void scrollFolderListRightButtonPressed();
        void scrollPlaylistListLeftButtonPressed();
        void scrollPlaylistListRightButtonPressed();

        public:
        static PlaylistFilters* filtersInstance;

        static HMUI::TableView* monitoredTable;
        static std::function<void(int)> deselectCallback;

        void Init();
        void RefreshFolders();
        void RefreshPlaylists();
        void UpdateShownPlaylists();
        void SetVisible(bool visible);
        void Destroy();
    };
}