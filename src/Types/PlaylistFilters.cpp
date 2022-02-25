#include "Main.hpp"
#include "Types/PlaylistFilters.hpp"
#include "Types/Folder.hpp"
#include "Types/FolderTableCell.hpp"
#include "Types/CoverTableCell.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"
#include "Icons.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "GlobalNamespace/SharedCoroutineStarter.hpp"

#include "HMUI/ScrollView.hpp"
#include "HMUI/AnimatedSwitchView.hpp"
#include "HMUI/Screen.hpp"

#include "UnityEngine/Rect.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "UnityEngine/UI/RectMask2D.hpp"

#include "System/Collections/Generic/HashSet_1.hpp"

using namespace PlaylistManager;
using namespace QuestUI;

PlaylistFilters* PlaylistFilters::filtersInstance = nullptr;
HMUI::TableView* PlaylistFilters::monitoredTable;
std::function<void(int)> PlaylistFilters::deselectCallback;

UnityEngine::GameObject* createContainer(UnityEngine::Transform* parent) {
    static ConstString name("PlaylistManagerUIContainer");
    auto go = UnityEngine::GameObject::New_ctor(name);
    go->get_transform()->SetParent(parent, false);
    go->AddComponent<UnityEngine::RectTransform*>();
    return go;
}

void instantSetToggle(UnityEngine::UI::Toggle* toggle, bool value) {
    if(toggle->m_IsOn == value)
        return;
    
    toggle->m_IsOn = value;
    auto animatedSwitch = toggle->GetComponent<HMUI::AnimatedSwitchView*>();
    animatedSwitch->HandleOnValueChanged(value);
    int intValue = value ? 1 : 0;
    animatedSwitch->switchAmount = intValue;
    animatedSwitch->LerpPosition(intValue);
    animatedSwitch->LerpColors(intValue, animatedSwitch->highlightAmount, animatedSwitch->disabledAmount);
}

#pragma region uiFunctions
void PlaylistFilters::filterSelected(int filter) {
    if(!hasLoaded) {
        // setting selected cells calls ReloadData, which breaks the filter list because of... honestly idk
        // something to do with the scroll view or viewport at least - GetVisibleCellsIdRange returns the wrong values
        auto list = filterList->tableView->visibleCells;
        for(int i = 0; i < list->get_Count(); i++)
            list->get_Item(i)->SetSelected(i == 0, HMUI::SelectableCell::TransitionType::Instant, filterList->tableView, false);
        filterList->tableView->selectedCellIdxs->Clear();
        filterList->tableView->selectedCellIdxs->Add(0);
        filterSelectionState = 0;
        return;
    }
    filterSelectionState = filter;
    if(filter == 3)
        setFoldersFilters(false);
    else
        ReloadPlaylists();
}

void PlaylistFilters::folderSelected(int listCellIdx) {
    LOG_DEBUG("Selected folder %i", listCellIdx);
    selectFolder(*currentFolderList[listCellIdx]);
}

void PlaylistFilters::backButtonPressed() {
    switch(state) {
        case State::filters: {
            break;
        } case State::folders: {
            if(parentFolders.size() == 0) {
                LOG_DEBUG("Leaving folder menu");
                // top level, go back to filter menu
                setFoldersFilters(true);
                deselectFolder();
                // setting selected cells calls ReloadData, which breaks the filter list because of... honestly idk
                // something to do with the scroll view or viewport at least - GetVisibleCellsIdRange returns the wrong values
                auto list = filterList->tableView->visibleCells;
                for(int i = 0; i < list->get_Count(); i++)
                    list->get_Item(i)->SetSelected(i == 0, HMUI::SelectableCell::TransitionType::Instant, filterList->tableView, false);
                filterList->tableView->selectedCellIdxs->Clear();
                filterList->tableView->selectedCellIdxs->Add(0);
                filterSelectionState = 0;
                ReloadPlaylists();
                break;
            } else if(parentFolders.size() == 1) {
                LOG_DEBUG("Going to top level folder menu");
                // remove current parent folder from parent folders
                parentFolders.erase(parentFolders.end() - 1);
                deselectFolder();
                break;
            } else {
                LOG_DEBUG("Going a level up in folder menu");
                // remove current parent folder from parent folders
                parentFolders.erase(parentFolders.end() - 1);
                // go to next parent folder up
                selectFolder(*parentFolders.back());
                // remove the parent folder again since it would be added redundantly on selection
                parentFolders.erase(parentFolders.end() - 1);
                break;
            }
        } case State::editing: {
            LOG_DEBUG("Exiting edit menu");
            // changes are saved as edits are made
            setFoldersFilters(false);
            // remove the folder from parent folders so it can be readded or not based on its edits
            if(parentFolders.size() > 0 && parentFolders.back() == currentFolder)
                parentFolders.erase(parentFolders.end() - 1);
            selectFolder(*currentFolder);
            // might refresh twice but it's ok
            RefreshFolders();
            break;
        } case State::creating: {
            LOG_DEBUG("Exiting create menu");
            // no changes will have been made if exiting the create menu with the back button
            setFoldersFilters(false);
            break;
        } default: {
            LOG_ERROR("Back button failed! (something really weird must have happened)");
            setFoldersFilters(true);
            break;
        }
    }
}

void PlaylistFilters::folderTitleTyped(std::string const& newTitle) {
    currentTitle = newTitle;
    if(state == State::editing) {
        currentFolder->FolderName = newTitle;
        WriteToFile(GetConfigPath(), playlistConfig);
    }
}

void PlaylistFilters::editMenuCreateButtonPressed() {
    LOG_DEBUG("Creating new folder");
    // add new folder as subfolder if open
    auto& parentArr = parentFolders.size() > 0 ? parentFolders.back()->Subfolders : playlistConfig.Folders;
    parentArr.emplace_back(Folder());
    // populate folder fields
    auto& folder = parentArr.back();
    folder.FolderName = currentTitle;
    folder.HasSubfolders = currentSubfolders;
    folder.ShowDefaults = currentDefaults;
    folder.Playlists = currentPlaylists;
    // write changes
    WriteToFile(GetConfigPath(), playlistConfig);
    setFoldersFilters(false);
    RefreshFolders();
}

void PlaylistFilters::editButtonPressed() {
    setFolderEdit(true);
}

void PlaylistFilters::deleteButtonPressed() {
    LOG_DEBUG("Deleting selected folder");
    // get parent array of current folder
    std::vector<Folder>* folderVecPtr = nullptr;
    // remove from parent folders if it is present
    if(parentFolders.size() > 0 && parentFolders.back() == currentFolder)
            parentFolders.erase(parentFolders.end() - 1);
    // get new parent folder list
    auto& folders = parentFolders.size() > 0 ? parentFolders.back()->Subfolders : playlistConfig.Folders;
    for(auto itr = folders.begin(); itr != folders.end(); itr++) {
        if(&(*itr) == currentFolder) {
            folders.erase(itr);
            // all folders are stored in playlistConfig, subfolders or not
            WriteToFile(GetConfigPath(), playlistConfig);
            break;
        }
    }
    deselectFolder();
}

void PlaylistFilters::createButtonPressed() {
    setFolderEdit(false);
}

void PlaylistFilters::subfoldersToggled(bool enabled) {
    currentSubfolders = enabled;
    // update ui for subfolders / playlists
    playlistListContainer->SetActive(!enabled);
    if(state == State::editing) {
        currentFolder->HasSubfolders = enabled;
        WriteToFile(GetConfigPath(), playlistConfig);
        ReloadPlaylists();
    }
}

void PlaylistFilters::defaultsToggled(bool enabled) {
    currentDefaults = enabled;
    if(state == State::editing) {
        currentFolder->ShowDefaults = enabled;
        WriteToFile(GetConfigPath(), playlistConfig);
        ReloadPlaylists();
    }
}

void PlaylistFilters::playlistSelected(int cellIdx) {
    LOG_DEBUG("Playlist selected in edit menu");
    if(!currentFolder)
        return;
    // add playlist to current folder
    auto& playlist = loadedPlaylists[cellIdx];
    auto& playlistVector = state == State::editing ? currentFolder->Playlists : currentPlaylists;
    playlistVector.emplace_back(playlist->path);
    // save and update ingame playlists if editing
    if(state == State::editing) {
        WriteToFile(GetConfigPath(), playlistConfig);
        ReloadPlaylists();
    }
}

void PlaylistFilters::playlistDeselected(int cellIdx) {
    LOG_DEBUG("Playlist %i deselected in edit menu", cellIdx);
    if(!currentFolder)
        return;
    // remove playlist from current folder
    auto& playlist = loadedPlaylists[cellIdx];
    auto& playlistVector = state == State::editing ? currentFolder->Playlists : currentPlaylists;
    for(auto itr = playlistVector.begin(); itr != playlistVector.end(); itr++) {
        if(*itr == playlist->path) {
            playlistVector.erase(itr);
            break;
        }
    }
    // save and update ingame playlists if editing
    if(state == State::editing) {
        WriteToFile(GetConfigPath(), playlistConfig);
        ReloadPlaylists();
    }
}

void PlaylistFilters::scrollFolderListLeftButtonPressed() {
    CustomListSource::ScrollListLeft(folderList, 5);
}

void PlaylistFilters::scrollFolderListRightButtonPressed() {
    CustomListSource::ScrollListRight(folderList, 5);
}

void PlaylistFilters::scrollPlaylistListLeftButtonPressed() {
    CustomListSource::ScrollListLeft(playlistList, 5);
}

void PlaylistFilters::scrollPlaylistListRightButtonPressed() {
    CustomListSource::ScrollListRight(playlistList, 5);
}
#pragma endregion

custom_types::Helpers::Coroutine PlaylistFilters::initCoroutine() {
    // make canvas for display
    canvas = BeatSaberUI::CreateCanvas();
    canvas->AddComponent<HMUI::Screen*>();
    auto cvsTrans = canvas->get_transform();
    cvsTrans->set_position({0, 0.02, 2.25});
    cvsTrans->set_eulerAngles({90, 0, 0});

    co_yield nullptr;

    #pragma region filterList
    // set up list
    UnityEngine::Vector2 sizeDelta{60, 15};
    filterList = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(cvsTrans, sizeDelta, [this](int cellIdx){
        filterSelected(cellIdx);
    });
    filterList->setType(csTypeOf(CoverTableCell*));
    filterList->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    filterList->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    // yet another questui bug
    auto layoutElement = filterList->tableView->get_transform()->get_parent()->GetComponent<UnityEngine::UI::LayoutElement*>();
    layoutElement->set_flexibleHeight(sizeDelta.y);
    layoutElement->set_minHeight(sizeDelta.y);
    layoutElement->set_preferredHeight(sizeDelta.y);
    layoutElement->set_flexibleWidth(sizeDelta.x);
    layoutElement->set_minWidth(sizeDelta.x);
    layoutElement->set_preferredWidth(sizeDelta.x);

    // add list data
    filterList->addSprites({AllPacksSprite(), DefaultPacksSprite(), CustomPacksSprite(), PackFoldersSprite()});
    filterList->addTexts({"All level packs", "No custom playlists", "Only custom playlists", "Playlist folders"});
    // starting anchors break size delta for some reason
    auto contentTransform = filterList->tableView->scrollView->contentRectTransform;
    contentTransform->set_anchorMin({0, 0});
    contentTransform->set_anchorMax({0, 1});
    filterList->tableView->ReloadData();
    if(filterSelectionState == 3) {
        filterSelectionState = 0;
        ReloadPlaylists();
    }
    filterList->tableView->SelectCellWithIdx(filterSelectionState, false);

    co_yield nullptr;
    // some update method or something is messing with this
    // and I cannot figure out what or why it's only this list
    contentTransform->set_localPosition({0, 7.5, 0});
    #pragma endregion

    #pragma region folderMenu
    folderMenu = createContainer(cvsTrans);
    folderMenu->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({0, 10});

    auto topLayoutGroup = BeatSaberUI::CreateHorizontalLayoutGroup(folderMenu->get_transform());
    topLayoutGroup->get_gameObject()->GetComponent<UnityEngine::UI::ContentSizeFitter*>()->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    topLayoutGroup->set_childControlWidth(true);
    topLayoutGroup->set_spacing(2);

    static ConstString contentName("Content");

    auto backButton = BeatSaberUI::CreateUIButton(topLayoutGroup->get_transform(), "<", "ActionButton", [this](){
        backButtonPressed();
    });
    UnityEngine::Object::Destroy(backButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    auto sizeFitter = backButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

    folderTitle = BeatSaberUI::CreateText(topLayoutGroup->get_transform(), "All Folders", false);
    folderTitle->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(30);
    folderTitle->set_margin({0, 1});
    folderTitle->set_enableWordWrapping(false);
    folderTitle->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);

    editButton = BeatSaberUI::CreateUIButton(topLayoutGroup->get_transform(), "Edit", [this](){
        editButtonPressed();
    });
    UnityEngine::Object::Destroy(editButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    sizeFitter = editButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

    deleteButton = BeatSaberUI::CreateUIButton(topLayoutGroup->get_transform(), "Delete", [this](){
        deleteButtonPressed();
    });
    UnityEngine::Object::Destroy(deleteButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    sizeFitter = deleteButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

    auto createButton = BeatSaberUI::CreateUIButton(topLayoutGroup->get_transform(), "Create", [this](){
        createButtonPressed();
    });
    UnityEngine::Object::Destroy(createButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    sizeFitter = createButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    
    editButton->set_interactable(false);
    deleteButton->set_interactable(false);

    folderMenu->SetActive(false);
    #pragma endregion

    co_yield nullptr;

    #pragma region folderEditMenu
    folderEditMenu = createContainer(cvsTrans);
    folderEditMenu->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({0, 10});
    
    topLayoutGroup = BeatSaberUI::CreateHorizontalLayoutGroup(folderEditMenu->get_transform());
    topLayoutGroup->get_gameObject()->GetComponent<UnityEngine::UI::ContentSizeFitter*>()->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    topLayoutGroup->set_childControlWidth(true);
    topLayoutGroup->set_spacing(2);
    topLayout = topLayoutGroup->get_rectTransform();
    
    auto editBackButton = BeatSaberUI::CreateUIButton(topLayoutGroup->get_transform(), "<", [this](){
        backButtonPressed();
    });
    UnityEngine::Object::Destroy(editBackButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    sizeFitter = editBackButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    
    editCreateButton = BeatSaberUI::CreateUIButton(topLayoutGroup->get_transform(), "+", [this](){
        editMenuCreateButtonPressed();
    });
    UnityEngine::Object::Destroy(editCreateButton->get_transform()->Find(contentName)->GetComponent<UnityEngine::UI::LayoutElement*>());
    sizeFitter = editCreateButton->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    sizeFitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    sizeFitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);

    titleField = BeatSaberUI::CreateStringSetting(topLayoutGroup->get_transform(), "Folder Name", "", [this](StringW newTitle){
        folderTitleTyped(newTitle);
    });
    titleField->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(41);

    subfoldersToggle = BeatSaberUI::CreateToggle(topLayoutGroup->get_transform(), "Subfolders", [this](bool enabled){
        subfoldersToggled(enabled);
    });
    subfoldersToggle->get_transform()->GetParent()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(35);
    
    folderEditMenu->SetActive(false);
    #pragma endregion

    co_yield nullptr;

    #pragma region folderMenuLists
    auto folderListContainer = createContainer(folderMenu->get_transform());
    folderListContainer->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({0, -20});
    // set up list
    folderList = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(folderListContainer->get_transform(), {75, 30}, [this](int cellIdx){
        folderSelected(cellIdx);
    });
    folderList->setType(csTypeOf(FolderTableCell*));
    folderList->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    folderList->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    RefreshFolders();
    // paging buttons
    auto left = BeatSaberUI::CreateUIButton(folderListContainer->get_transform(), "", "SettingsButton", {-40, 0}, {8, 8}, [this](){
        scrollFolderListLeftButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    auto right = BeatSaberUI::CreateUIButton(folderListContainer->get_transform(), "", "SettingsButton", {40, 0}, {8, 8}, [this](){
        scrollFolderListRightButtonPressed();
    });
    reinterpret_cast<UnityEngine::RectTransform*>(right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());

    co_yield nullptr;

    playlistListContainer = createContainer(folderEditMenu->get_transform());
    playlistListContainer->GetComponent<UnityEngine::RectTransform*>()->set_anchoredPosition({0, -15});
    // list for selecting playlists from folders
    playlistList = BeatSaberUI::CreateCustomSourceList<CustomListSource*>(playlistListContainer->get_transform(), {75, 15}, [this](int cellIdx){
        playlistSelected(cellIdx);
    });
    playlistList->setType(csTypeOf(CoverTableCell*));
    playlistList->tableView->tableType = HMUI::TableView::TableType::Horizontal;
    playlistList->tableView->scrollView->scrollViewDirection = HMUI::ScrollView::ScrollViewDirection::Horizontal;
    playlistList->tableView->set_selectionType(HMUI::TableViewSelectionType::Multiple);
    PlaylistFilters::monitoredTable = playlistList->tableView;
    PlaylistFilters::deselectCallback = [this](int cellIdx){ playlistDeselected(cellIdx); };
    RefreshPlaylists();
    // paging buttons
    left = BeatSaberUI::CreateUIButton(playlistListContainer->get_transform(), "", "SettingsButton", {-40, 0}, {8, 8}, [this](){
        scrollPlaylistListLeftButtonPressed();
    });
    ((UnityEngine::RectTransform*) left->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(left, LeftCaratInactiveSprite(), LeftCaratSprite());

    right = BeatSaberUI::CreateUIButton(playlistListContainer->get_transform(), "", "SettingsButton", {40, 0}, {8, 8}, [this](){
        scrollPlaylistListRightButtonPressed();
    });
    ((UnityEngine::RectTransform*) right->get_transform()->GetChild(0))->set_sizeDelta({8, 8});
    BeatSaberUI::SetButtonSprites(right, RightCaratInactiveSprite(), RightCaratSprite());
    
    defaultsToggle = BeatSaberUI::CreateToggle(playlistListContainer->get_transform(), "Defaults", {0, -65}, [this](bool enabled){
        defaultsToggled(enabled);
    });
    defaultsToggle->get_transform()->GetParent()->GetComponent<UnityEngine::UI::LayoutElement*>()->set_preferredWidth(30);
    defaultsToggle->get_transform()->GetParent()->get_gameObject()->AddComponent<UnityEngine::UI::ContentSizeFitter*>()->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
    #pragma endregion

    setFoldersFilters(true);

    co_return;
}

void PlaylistFilters::refreshFolderPlaylists() {
    if(!currentFolder || !playlistList) return;
    LOG_DEBUG("Reloading playlists of the folder");

    // set table cells selected
    playlistList->tableView->selectedCellIdxs->Clear();
    for(std::string& path : currentFolder->Playlists) {
        int idx = GetPlaylistIndex(path);
        if(idx >= 0 && idx < loadedPlaylists.size())
            playlistList->tableView->selectedCellIdxs->Add(idx);
    }
    // update visuals
    playlistList->tableView->RefreshCells(true, false);
}

void PlaylistFilters::setFoldersFilters(bool filtersVisible) {
    LOG_DEBUG("Setting folder/filter menu state");
    filterList->get_gameObject()->SetActive(filtersVisible);
    folderMenu->SetActive(!filtersVisible);
    folderEditMenu->SetActive(false);
    state = filtersVisible ? State::filters : State::folders;
}

void PlaylistFilters::setFolderEdit(bool editing) {
    LOG_DEBUG("Activating edit/create menu");
    if(editing) {
        if(!currentFolder) {
            setFoldersFilters(false);
            return;
        }
        titleField->set_text(currentFolder->FolderName);
        instantSetToggle(subfoldersToggle, currentFolder->HasSubfolders);
        instantSetToggle(defaultsToggle, currentFolder->ShowDefaults);
        playlistListContainer->SetActive(!currentFolder->HasSubfolders);
        refreshFolderPlaylists();
        editCreateButton->get_gameObject()->SetActive(false);
        topLayout->set_anchoredPosition({0, 0});
    } else {
        static ConstString empty("");
        titleField->set_text(empty);
        currentTitle = "";
        instantSetToggle(subfoldersToggle, false);
        currentSubfolders = false;
        instantSetToggle(defaultsToggle, false);
        currentDefaults = false;
        currentPlaylists.clear();
        playlistListContainer->SetActive(true);
        playlistList->tableView->ClearSelection();
        editCreateButton->get_gameObject()->SetActive(true);
        topLayout->set_anchoredPosition({5, 0});
    }
    folderMenu->SetActive(false);
    folderEditMenu->SetActive(true);
    RefreshPlaylists();
    filterList->get_gameObject()->SetActive(false);
    state = editing ? State::editing : State::creating;
}

void PlaylistFilters::selectFolder(Folder& folder) {
    LOG_DEBUG("Folder selected");
    // update button interactability
    editButton->set_interactable(true);
    deleteButton->set_interactable(true);
    folderTitle->set_text(folder.FolderName);
    // update config state
    currentFolder = &folder;
    // update menu
    if(folder.HasSubfolders) {
        parentFolders.emplace_back(&folder);
        RefreshFolders();
        folderList->tableView->ClearSelection();
    }
    ReloadPlaylists();
}

void PlaylistFilters::deselectFolder() {
    LOG_DEBUG("Folder deselected");
    // update table
    if(folderList)
        folderList->tableView->ClearSelection();
    // update config state
    if(parentFolders.size() > 0) {
        currentFolder = parentFolders.back();
        folderTitle->set_text(parentFolders.back()->FolderName);
    } else {
        currentFolder = nullptr;
        static ConstString defaultTitle("All Folders");
        folderTitle->set_text(defaultTitle);
    }
    // only disable buttons if there is no folder selected
    editButton->set_interactable(currentFolder == nullptr);
    deleteButton->set_interactable(currentFolder == nullptr);
    RefreshFolders();
    ReloadPlaylists();
}

void PlaylistFilters::Init() {
    // start coroutine
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(initCoroutine()));
}

void PlaylistFilters::RefreshFolders() {
    if(!folderList) return;
    LOG_DEBUG("Reloading folders/subfolders");

    std::vector<std::string> folderNames;
    currentFolderList.clear();
    if(parentFolders.size() > 0) {
        LOG_DEBUG("Loading current folder subfolders");
        for(auto& folder : parentFolders.back()->Subfolders) {
            folderNames.push_back(folder.FolderName);
            currentFolderList.push_back(&folder);
        }
    } else {
        LOG_DEBUG("Loading top level folders");
        for(auto& folder : playlistConfig.Folders) {
            folderNames.push_back(folder.FolderName);
            currentFolderList.push_back(&folder);
        }
    }
    folderList->replaceTexts(folderNames);
    folderList->tableView->ReloadData();
}

void PlaylistFilters::RefreshPlaylists() {
    if(!playlistList)
        return;
    loadedPlaylists = GetLoadedPlaylists();
    std::vector<UnityEngine::Sprite*> newCovers;
    std::vector<std::string> newHovers;
    for(auto& playlist : loadedPlaylists) {
        newCovers.emplace_back(GetCoverImage(playlist));
        newHovers.emplace_back(playlist->name);
    }
    playlistList->replaceSprites(newCovers);
    playlistList->replaceTexts(newHovers);
    playlistList->tableView->ReloadData();
}

void PlaylistFilters::Destroy() {
    PlaylistFilters::filtersInstance = nullptr;
    currentFolder = nullptr;
    UnityEngine::Object::Destroy(canvas);
    // assumes it's always allocated with new
    delete this;
}