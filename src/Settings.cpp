#include "Main.hpp"
#include "Settings.hpp"
#include "Types/PlaylistMenu.hpp"
#include "Types/PlaylistFilters.hpp"
#include "Types/LevelButtons.hpp"
#include "Types/GridViewAddon.hpp"
#include "Types/Config.hpp"
#include "Types/CustomListSource.hpp"
#include "Types/CoverTableCell.hpp"
#include "PlaylistManager.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView_ScrollPositionType.hpp"

using namespace QuestUI;

namespace PlaylistManager {

    void ModSettingsDidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        
        using Vec = UnityEngine::Vector2;

        if(!firstActivation)
            return;

        self->get_gameObject()->AddComponent<HMUI::Touchable*>();

        auto container = BeatSaberUI::CreateScrollableSettingsContainer(self);
        auto parent = container->get_transform();
        
        auto horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(parent);
        horizontal->set_childControlWidth(false);
        horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
        auto reloadNewButton = BeatSaberUI::CreateUIButton(horizontal, "Reload New Playlists", Vec{0, 0}, Vec{40, 10}, [] {
            ReloadPlaylists(false);
        });
        BeatSaberUI::AddHoverHint(reloadNewButton, "Reloads new playlists from the playlist folder");

        horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(parent);
        horizontal->set_childControlWidth(false);
        horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
        auto reloadAllButton = BeatSaberUI::CreateUIButton(horizontal, "Reload All Playlists", Vec{0, 0}, Vec{40, 10}, [] {
            ReloadPlaylists(true);
        });
        BeatSaberUI::AddHoverHint(reloadAllButton, "Reloads all playlists from the playlist folder");

        auto coverModal = BeatSaberUI::CreateModal(self->get_transform(), {83, 17}, nullptr);
        
        auto list = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(coverModal->get_transform(), {70, 15}, [coverModal](int cellIdx) {
            DeleteLoadedImage(cellIdx);
            coverModal->Hide(true, nullptr);
        });
        list->setType(csTypeOf(CoverTableCell*));
        list->tableView->tableType = HMUI::TableView::TableType::Horizontal;
        list->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;

        // scroll arrows
        auto left = BeatSaberUI::CreateUIButton(coverModal->get_transform(), "", "SettingsButton", {-38, 0}, {8, 8}, [list] {
            CustomListSource::ScrollListLeft(list, 4);
        });
        ((UnityEngine::RectTransform*) left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
        BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

        auto right = BeatSaberUI::CreateUIButton(coverModal->get_transform(), "", "SettingsButton", {38, 0}, {8, 8}, [list] {
            CustomListSource::ScrollListRight(list, 4);
        });
        ((UnityEngine::RectTransform*) right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
        BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());

        horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(parent);
        horizontal->set_childControlWidth(false);
        horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
        auto imageButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "Delete Saved Image", Vec{0, 0}, Vec{40, 10}, [list, coverModal] {
            // reload covers from folder
            LoadCoverImages();
            // add cover images and reload
            list->replaceSprites(GetLoadedImages());
            list->tableView->ReloadData();
            list->tableView->ClearSelection();
            coverModal->Show(true, false, nullptr);
        });

        horizontal = BeatSaberUI::CreateHorizontalLayoutGroup(parent);
        horizontal->set_childControlWidth(false);
        horizontal->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
        auto uiButton = BeatSaberUI::CreateUIButton(horizontal->get_transform(), "Reset UI", Vec{0, 0}, Vec{40, 10}, [] {
            DestroyUI();
        });
        BeatSaberUI::AddHoverHint(uiButton->get_gameObject(), "Resets all UI instances");
        uiButton->set_interactable(playlistConfig.Management);

        auto managementToggle = BeatSaberUI::CreateToggle(parent, "Enable playlist management", playlistConfig.Management, [uiButton](bool enabled) {
            uiButton->set_interactable(enabled);
            playlistConfig.Management = enabled;
            WriteToFile(GetConfigPath(), playlistConfig);
            if(!enabled)
                DestroyUI();
        });
        managementToggle->get_transform()->GetParent()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
        
        auto downloadToggle = BeatSaberUI::CreateToggle(parent, "Show download icons in grid", playlistConfig.DownloadIcon, [](bool enabled){
            playlistConfig.DownloadIcon = enabled;
            WriteToFile(GetConfigPath(), playlistConfig);
        });
        downloadToggle->get_transform()->GetParent()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(60);
        BeatSaberUI::AddHoverHint(downloadToggle->get_gameObject(), "Toggles download icons for custom playlists that do not have all their songs downloaded");
    }

    void DestroyUI() {
        if(PlaylistFilters::filtersInstance)
            PlaylistFilters::filtersInstance->Destroy();
        
        if(ButtonsContainer::buttonsInstance)
            ButtonsContainer::buttonsInstance->Destroy();
        
        if(PlaylistMenu::menuInstance)
            PlaylistMenu::menuInstance->Destroy();

        if(GridViewAddon::addonInstance)
            GridViewAddon::addonInstance->Destroy();
    }
};
