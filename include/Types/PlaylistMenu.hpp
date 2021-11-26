#pragma once

#include "custom-types/shared/coroutine.hpp"

#include "Types/BPList.hpp"

#include "UnityEngine/MonoBehaviour.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/Button.hpp"

#include "HMUI/ImageView.hpp"
#include "HMUI/InputFieldView.hpp"
#include "HMUI/ModalView.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsTableView.hpp"

namespace PlaylistManager {
    class CustomCoverListSource;
}

DECLARE_CLASS_CODEGEN(PlaylistManager, PlaylistMenu, UnityEngine::MonoBehaviour,

    private:
    UnityEngine::GameObject *buttonsContainer, *detailsContainer;
    HMUI::InputFieldView *playlistTitle, *playlistAuthor;
    TMPro::TextMeshProUGUI *playlistDescription, *descriptionTitle;
    UnityEngine::UI::Button *coverButton, *createButton, *cancelButton;
    HMUI::ImageView *coverImage;
    HMUI::ModalView *confirmModal, *coverModal;
    CustomCoverListSource* list;

    int coverImageIndex;

    std::string currentTitle, currentAuthor;

    bool detailsVisible, inMovement, addingPlaylist;

    GlobalNamespace::AnnotatedBeatmapLevelCollectionsTableView *gameTableView;
    UnityEngine::GameObject* detailWrapper;
    BPList* playlist;

    custom_types::Helpers::Coroutine moveCoroutine(bool reversed);
    custom_types::Helpers::Coroutine refreshCoroutine();

    custom_types::Helpers::Coroutine initCoroutine();

    void updateDetailsMode();
    void scrollToIndex(int index);

    public:
    static std::function<void()> nextCloseKeyboard;

    void Init(UnityEngine::GameObject* detailWrapper, BPList* list);
    void SetPlaylist(BPList* playlist);

    void SetVisible(bool visible);
    void ShowDetails(bool visible);
    void RefreshDetails();
)