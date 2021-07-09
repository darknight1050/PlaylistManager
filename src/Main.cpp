#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "songloader/shared/API.hpp"

#include "questui/shared/QuestUI.hpp"

#include "PlaylistManager.hpp"

#include "CustomLogger.hpp"
#include "Paths.hpp"

#include "UnityEngine/Rect.hpp" // This needs to be included before RectTransform
#include "UnityEngine/RectTransform.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "System/Tuple_2.hpp"

ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

std::string GetPlaylistsPath() {
    static std::string playlistsPath(getDataDir(modInfo) + "Playlists");
    return playlistsPath;
}

using TupleType = System::Tuple_2<int, int>;

MAKE_HOOK_MATCH(TableView_GetVisibleCellsIdRange, &HMUI::TableView::GetVisibleCellsIdRange, TupleType*, HMUI::TableView* self) {
    using namespace HMUI;
    UnityEngine::Rect rect = self->viewportTransform->get_rect();

    float heightWidth = (self->tableType == TableView::TableType::Vertical) ? rect.get_height() : rect.get_width();
    float position = (self->tableType == TableView::TableType::Vertical) ? self->scrollView->get_position() : -self->scrollView->get_position();

    int min = floor(position / self->cellSize + self->cellSize * 0.001f);
    if (min < 0) {
        min = 0;
    }

    int max = floor((position + heightWidth - self->cellSize * 0.001f) / self->cellSize);
    if (max > self->numberOfCells - 1) {
        max = self->numberOfCells - 1;
    }

    return TupleType::New_ctor(min, max);
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "PlaylistManager";
    modInfo.version = VERSION;
    info = modInfo;
    
    auto playlistsPath = GetPlaylistsPath();
    if(!direxists(playlistsPath))
        mkpath(playlistsPath);
}

extern "C" void load() {
    LOG_INFO("Starting PlaylistManager installation...");
    il2cpp_functions::Init();
    QuestUI::Init();
    INSTALL_HOOK(getLogger(), TableView_GetVisibleCellsIdRange);
    RuntimeSongLoader::API::AddRefreshLevelPacksEvent(
        [] (RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO) {
            PlaylistManager::LoadPlaylists(customBeatmapLevelPackCollectionSO);
        }
    );
    LOG_INFO("Successfully installed PlaylistManager!");
}