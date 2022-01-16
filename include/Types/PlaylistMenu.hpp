#pragma once

#include "custom-types/shared/coroutine.hpp"

#include "UnityEngine/MonoBehaviour.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/Button.hpp"

#include "HMUI/ImageView.hpp"
#include "HMUI/InputFieldView.hpp"
#include "HMUI/ModalView.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsGridView.hpp"

namespace PlaylistManager {
    class Playlist;
}

DECLARE_CLASS_CODEGEN(PlaylistManager, PlaylistMenu, UnityEngine::MonoBehaviour,

    private:
    UnityEngine::GameObject *buttonsContainer, *detailsContainer;
    HMUI::InputFieldView *playlistTitle, *playlistAuthor;
    TMPro::TextMeshProUGUI *playlistDescription, *descriptionTitle;
    UnityEngine::UI::Button *coverButton, *createButton, *cancelButton, *deleteButton;
    HMUI::ImageView *coverImage, *packImage;
    HMUI::ModalView *confirmModal, *coverModal;
    class CustomListSource *list;

    int coverImageIndex;

    std::string currentTitle, currentAuthor;

    bool detailsVisible, inMovement, addingPlaylist;

    GlobalNamespace::AnnotatedBeatmapLevelCollectionsGridView* gameTableView;
    Playlist* playlist;

    custom_types::Helpers::Coroutine moveCoroutine(bool reversed);
    custom_types::Helpers::Coroutine refreshCoroutine();

    custom_types::Helpers::Coroutine initCoroutine();

    void updateDetailsMode();
    void scrollToIndex(int index);

    void infoButtonPressed();
    void syncButtonPressed();
    void addButtonPressed();
    void moveRightButtonPressed();
    void moveLeftButtonPressed();
    void playlistTitleTyped(std::string_view newValue);
    void playlistAuthorTyped(std::string_view newValue);
    void coverButtonPressed();
    void deleteButtonPressed();
    void createButtonPressed();
    void cancelButtonPressed();
    void confirmDeleteButtonPressed();
    void cancelDeleteButtonPressed();
    void coverSelected(int listCellIdx);
    void scrollListLeftButtonPressed();
    void scrollListRightButtonPressed();

    public:
    static std::function<void()> nextCloseKeyboard;
    static PlaylistMenu* menuInstance;

    void Init(HMUI::ImageView* packImage, Playlist* playlist);
    void SetPlaylist(Playlist* playlist);
    void RefreshCovers();

    void SetVisible(bool visible);
    void ShowDetails(bool visible);
    void RefreshDetails();
    void Destroy();
)