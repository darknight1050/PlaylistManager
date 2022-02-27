#pragma once

#include "songdownloader/shared/Types/BeatSaver/Beatmap.hpp"
#include "songdownloader/shared/Types/BeastSaber/Song.hpp"
#include "songdownloader/shared/Types/ScoreSaber/Leaderboard.hpp"

#include "custom-types/shared/macros.hpp"

#include "UnityEngine/GameObject.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/UI/Button.hpp"

class SearchEntryHack {
    public:
    BeatSaver::Beatmap map;
    BeastSaber::Song BSsong;
    ScoreSaber::Leaderboard SSsong;
    UnityEngine::GameObject* gameObject;
    TMPro::TextMeshProUGUI* line1Component;
    TMPro::TextMeshProUGUI* line2Component;
    HMUI::ImageView* coverImageView;
    UnityEngine::UI::Button* downloadButton;

    enum class MapType {
        BeatSaver,
        BeastSaber,
        ScoreSaber
    };

    float downloadProgress;

    MapType MapType;
};

namespace PlaylistManager {
    class Playlist;
}

DECLARE_CLASS_CODEGEN(PlaylistManager, SongDownloaderAddon, HMUI::ViewController,

    private:
    class CustomListSource *list;

    std::vector<Playlist*> loadedPlaylists;

    bool downloadToPlaylistEnabled = true;
    Playlist* selectedPlaylist = nullptr;

    void playlistSelected(int cellIdx);
    void scrollListLeftButtonPressed();
    void scrollListRightButtonPressed();
    
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

    public:
    static Playlist* SelectedPlaylist;
    
    static SongDownloaderAddon* Create();

    void RefreshPlaylists();
)