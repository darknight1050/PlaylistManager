#include "Main.hpp"
#include "Types/BPList.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"

#include <filesystem>
#include <fstream>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "songloader/shared/API.hpp"

#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Convert.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"

using namespace RuntimeSongLoader;

namespace PlaylistManager {

    // map from playlist names to json objects
    std::unordered_map<std::string, BPList*> playlists_json;
    // map from file paths to songloader playlists
    SafePtr<System::Collections::Generic::Dictionary_2<Il2CppString*, SongLoaderCustomBeatmapLevelPack*>> path_playlists;
    // cache default cover to avoid tons of lookups
    UnityEngine::Sprite* defaultCover;
    // keep track of images to avoid loading duplicates
    static std::hash<std::string> hasher;
    std::unordered_map<std::size_t, int> imageHashes;
    
    // array of all loaded images
    std::vector<UnityEngine::Sprite*> loadedImages;
    // all unusable playlists
    const std::unordered_set<std::string> staticPacks = {
        "Original Soundtrack Vol. 1",
        "Original Soundtrack Vol. 2",
        "Original Soundtrack Vol. 3",
        "Original Soundtrack Vol. 4",
        "Extras",
        "Camellia Music Pack",
        "Skrillex Music Pack",
        "Interscope Mixtape",
        "BTS Music Pack",
        "Linkin Park Music Pack",
        "Timbaland Music Pack",
        "Green Day Music Pack",
        "Rocket League x Monstercat Music Pack",
        "Panic! At The Disco Music Pack",
        "Imagine Dragons Music Pack",
        "Monstercat Music Pack Vol. 1",
        "Custom Levels",
        "WIP Levels"
    };
    
    std::string SanitizeFileName(std::string fileName) {
        std::string newName;
        // yes I know not all of these are disallowed, and also that they are unlikely to end up in a name
        static const std::unordered_set<unsigned char> replacedChars = {
            '/', '\n', '\\', '\'', '\"', ' ', '?', '<', '>', ':', '*'
        };
        std::transform(fileName.begin(), fileName.end(), std::back_inserter(newName), [](unsigned char c){
            if(replacedChars.contains(c))
                return (unsigned char)('_');
            return c;
        });
        if(newName == "")
            return "_";
        return newName;
    }

    bool UniqueFileName(std::string fileName, std::string compareDirectory) {
        if(!std::filesystem::is_directory(compareDirectory))
            return true;
        for(auto& entry : std::filesystem::directory_iterator(compareDirectory)) {
            if(entry.is_directory())
                continue;
            if(entry.path().filename().string() == fileName)
                return false;
        }
        return true;
    }

    std::string GetBase64ImageType(std::string& base64) {
        if(base64.length() < 3)
            return "";
        std::string sub = base64.substr(0, 3);
        if(sub == "iVB")
            return ".png";
        if(sub == "/9j")
            return ".jpg";
        if(sub == "R0l")
            return ".gif";
        if(sub == "Qk1")
            return ".bmp";
        return "";
    }

    // returns base 64 string of png as well
    std::string WriteImageToFile(std::string_view pathToPng, UnityEngine::Sprite* image) {
        auto bytes = UnityEngine::ImageConversion::EncodeToPNG(image->get_texture());
        writefile(pathToPng, std::string(reinterpret_cast<char*>(bytes.begin()), bytes.Length()));
        return STR(System::Convert::ToBase64String(bytes));
    }

    bool ReadFromFile(std::string_view path, JSONClass& toDeserialize) {
        if(!fileexists(path))
            return false;
        auto json = readfile(path);
        rapidjson::Document document;
        document.Parse(json);
        if(document.HasParseError() || !document.IsObject())
            return false;
        try {
            toDeserialize.Deserialize(document.GetObject());
            return true;
        } catch (const char* msg) {
            LOG_ERROR("Error loading playlist %s: %s", path.data(), msg);
        }
        return false;
    }

    bool WriteToFile(std::string_view path, JSONClass& toSerialize) {
        rapidjson::Document document;
        document.SetObject();
        toSerialize.Serialize(document.GetAllocator()).Swap(document);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
        std::string s = buffer.GetString();

        return writefile(path, s);
    }
    
    BPList* GetPlaylistJSON(std::string title) {
        auto foundItr = playlists_json.find(title);
        if(foundItr == playlists_json.end())
            return nullptr;
        return foundItr->second;
    }

    int GetPackIndex(std::string title) {
        // find index of playlist title in config
        int size = playlistConfig.Order.size();
        for(int i = 0; i < size; i++) {
            if(playlistConfig.Order[i] == title)
                return i;
        }
        // add to end of config if not found
        playlistConfig.Order.push_back(title);
        return -1;
    }

    SongLoaderCustomBeatmapLevelPack* GetSongloaderPack(BPList* playlist) {
        auto pathCS = CSTR(playlist->path);
        if(path_playlists->ContainsKey(pathCS))
            return path_playlists->get_Item(pathCS);
        return nullptr;
    }

    std::vector<GlobalNamespace::CustomBeatmapLevelPack*> GetLoadedPlaylists() {
        // create return vector with base size
        std::vector<GlobalNamespace::CustomBeatmapLevelPack*> ret(playlistConfig.Order.size());
        // auto cs_itr = path_playlists->get_Values()->GetEnumerator();
        // while(cs_itr.MoveNext()) {
        for(int i = 0; i < path_playlists->get_Count(); i++) {
            auto pack = (SongLoaderCustomBeatmapLevelPack*) path_playlists->entries[i].value;
            if(pack) {
                std::string name = STR(pack->CustomLevelsPack->get_packName());
                int idx = GetPackIndex(name);
                if(idx >= 0)
                    ret[idx] = pack->CustomLevelsPack;
                else
                    ret.push_back(pack->CustomLevelsPack);
            }
        }
        // remove empty slots
        int i = 0;
        for(auto itr = ret.begin(); itr != ret.end(); itr++) {
            if(*itr == nullptr) {
                ret.erase(itr);
                itr--;
            }
            i++;
        }
        return ret;
    }

    UnityEngine::Sprite* GetDefaultCoverImage() {
        if(!defaultCover) {
            auto arr = UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::CustomLevelLoader*>();
            if(arr.Length() < 1) {
                LOG_ERROR("Unable to find custom level loader");
                return nullptr;
            }
            defaultCover = arr[0]->defaultPackCover;
        }
        return defaultCover;
    }

    UnityEngine::Sprite* GetCoverImage(BPList* playlist) {
        if(playlist->ImageString.has_value()) {
            std::string imageBase64 = playlist->ImageString.value();
            // trim "data:image/png;base64,"-like metadata
            static std::string searchString = "base64,";
            auto searchIndex = imageBase64.find(searchString);
            if(searchIndex != std::string::npos)
                imageBase64 = imageBase64.substr(searchIndex + searchString.length());
            // return loaded image if existing
            std::size_t imgHash = hasher(imageBase64);
            auto foundItr = imageHashes.find(imgHash);
            if (foundItr != imageHashes.end()) {
                LOG_INFO("Returning loaded image with hash %lu", imgHash);
                playlist->imageIndex = foundItr->second;
                return loadedImages[foundItr->second];
            }
            // check image type
            std::string imgType = GetBase64ImageType(imageBase64);
            if(imgType != ".png" && imgType != ".jpg") {
                LOG_ERROR("Unsupported image type %s", imgType.c_str());
                return GetDefaultCoverImage();
            }
            // get and write sprite
            auto sprite = QuestUI::BeatSaberUI::Base64ToSprite(imageBase64);
            // compare sanitized hash
            // can sometimes need two passes?? idk but I don't want to add to the lag and do it on every playlist, so just restart I guess
            auto bytes = UnityEngine::ImageConversion::EncodeToPNG(sprite->get_texture());
            std::size_t oldHash = imgHash;
            imageBase64 = STR(System::Convert::ToBase64String(bytes));
            imgHash = hasher(imageBase64);
            // write to playlist if changed
            if(imgHash != oldHash) {
                playlist->ImageString = imageBase64;
                WriteToFile(playlist->path, *playlist);
            }
            foundItr = imageHashes.find(imgHash);
            if (foundItr != imageHashes.end()) {
                LOG_INFO("Returning loaded image with hash %lu", imgHash);
                playlist->imageIndex = foundItr->second;
                return loadedImages[foundItr->second];
            }
            // save image as file and return
            LOG_INFO("Writing image with hash %lu", imgHash);
            std::string imgPath = GetCoversPath() + "/" + playlist->PlaylistTitle + "_" + std::to_string(imgHash) + ".png";
            writefile(imgPath, std::string(reinterpret_cast<char*>(bytes.begin()), bytes.Length()));
            imageHashes.insert({imgHash, loadedImages.size()});
            playlist->imageIndex = loadedImages.size();
            loadedImages.emplace_back(sprite);
            return sprite;
        }
        return GetDefaultCoverImage();
    }

    void GetCoverImages() {
        // ensure path exists
        auto path = GetCoversPath();
        if(!std::filesystem::is_directory(path))
            return;
        // iterate through all image files
        for(const auto& file : std::filesystem::directory_iterator(path)) {
            if(!file.is_directory()) {
                auto path = file.path();
                // check file extension
                if(path.extension().string() == ".jpg") {
                    auto newPath = path.parent_path() / (path.stem().string() + ".png");
                    std::filesystem::rename(path, newPath);
                    path = newPath;
                } else if(path.extension().string() != ".png") {
                    LOG_ERROR("Incompatible file extension: %s", path.extension().string().c_str());
                    continue;
                }
                // check hash of base image before converting to sprite and to png
                std::ifstream instream(path, std::ios::in | std::ios::binary | std::ios::ate);
                auto size = instream.tellg();
                instream.seekg(0, instream.beg);
                auto bytes = Array<uint8_t>::NewLength(size);
                instream.read(reinterpret_cast<char*>(bytes->values), size);
                std::size_t imgHash = hasher(STR(System::Convert::ToBase64String(bytes)));
                if(imageHashes.contains(imgHash)) {
                    LOG_INFO("Skipping loading image with hash %lu", imgHash);
                    continue;
                }
                // sanatize hash by converting to png
                auto sprite = QuestUI::BeatSaberUI::ArrayToSprite(bytes);
                imgHash = hasher(WriteImageToFile(path.string(), sprite));
                // check hash with loaded images
                if(imageHashes.contains(imgHash)) {
                    LOG_INFO("Skipping loading image with hash %lu", imgHash);
                    continue;
                }
                LOG_INFO("Loading image with hash %lu", imgHash);
                // rename file with hash for identification (coolImage -> coolImage_1987238271398)
                auto searchIndex = path.stem().string().rfind("_") + 1;
                if(path.stem().string().substr(searchIndex) != std::to_string(imgHash)) {
                    LOG_INFO("Renaming image file with hash %lu", imgHash);
                    std::filesystem::rename(path, path.parent_path() / (path.stem().string() + "_" + std::to_string(imgHash) + ".png"));
                }
                imageHashes.insert({imgHash, loadedImages.size()});
                loadedImages.emplace_back(sprite);
            }
        }
    }

    bool AvailablePlaylistName(std::string title) {
        // check in constant playlists
        if(staticPacks.contains(title))
            return false;
        // check in custom playlists
        for(auto& pair : playlists_json) {
            if(pair.second->PlaylistTitle == title)
                return false;
        }
        return true;
    }

    bool ShouldAddPack(std::string name) {
        if(currentFolder && folderSelectionState == 3) {
            for(std::string testName : currentFolder->PlaylistNames) {
                if(name == testName)
                    return true;
            }
        }
        return folderSelectionState == 0 || folderSelectionState == 2;
    }

    void LoadPlaylists(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh) {
        // load images if none are loaded
        if(loadedImages.size() < 1)
            GetCoverImages();
        // get playlists if not already initialized
        if(!path_playlists)
            path_playlists = System::Collections::Generic::Dictionary_2<Il2CppString*, SongLoaderCustomBeatmapLevelPack*>::New_ctor();
        if(fullRefresh)
            path_playlists->Clear();
        // ensure path exists
        auto path = GetPlaylistsPath();
        if(!std::filesystem::is_directory(path))
            return;
        // create array for playlists
        std::vector<SongLoaderCustomBeatmapLevelPack*> sortedPlaylists(playlistConfig.Order.size());
        // iterate through all playlist files
        for(const auto& entry : std::filesystem::directory_iterator(path)) {
            if(!entry.is_directory()) {
                // check if playlist has been loaded already
                auto path = entry.path().string();
                auto pathCS = CSTR(path);
                if(path_playlists->ContainsKey(pathCS)) {
                    LOG_INFO("Loading playlist file %s from cache", path.c_str());
                    // check if playlist should be added
                    auto levelPack = path_playlists->get_Item(pathCS);
                    if(ShouldAddPack(STR(levelPack->CustomLevelsPack->packName))) {
                        int packPosition = GetPackIndex(STR(levelPack->CustomLevelsPack->packName));
                        // add if new (idk how)
                        if(packPosition < 0)
                            sortedPlaylists.emplace_back(levelPack);
                        else
                            sortedPlaylists[packPosition] = levelPack;
                    }
                } else {
                    LOG_INFO("Loading playlist file %s", path.c_str());
                    // get BPList object from file
                    BPList* list = new BPList();
                    if(ReadFromFile(path, *list)) {
                        // check for duplicate name
                        while(playlists_json.find(list->PlaylistTitle) != playlists_json.end()) {
                            LOG_INFO("Duplicate playlist name!");
                            continue;
                        }
                        playlists_json.insert({list->PlaylistTitle, list});
                        // create playlist object and add it to playlist dictionary
                        list->path = path;
                        SongLoaderCustomBeatmapLevelPack* customBeatmapLevelPack = SongLoaderCustomBeatmapLevelPack::New_ctor(list->PlaylistTitle, list->PlaylistTitle, GetCoverImage(list));
                        path_playlists->Add(pathCS, customBeatmapLevelPack);
                        // add all songs to the playlist object
                        auto foundSongs = List<GlobalNamespace::CustomPreviewBeatmapLevel*>::New_ctor();
                        for(auto& song : list->Songs) {
                            auto search = RuntimeSongLoader::API::GetLevelByHash(song.Hash);
                            if(search.has_value())
                                foundSongs->Add(search.value());
                        }
                        customBeatmapLevelPack->SetCustomPreviewBeatmapLevels(foundSongs->ToArray());
                        // add the playlist to the sorted array
                        if(ShouldAddPack(list->PlaylistTitle)) {
                            int packPosition = GetPackIndex(list->PlaylistTitle);
                            // add if new
                            if(packPosition < 0)
                                sortedPlaylists.emplace_back(customBeatmapLevelPack);
                            else
                                sortedPlaylists[packPosition] = customBeatmapLevelPack;
                        }
                    }
                }
            }
        }
        // add playlists to game in sorted order
        for(auto customBeatmapLevelPack : sortedPlaylists) {
            if(customBeatmapLevelPack)
                customBeatmapLevelPack->AddTo(customBeatmapLevelPackCollectionSO, true);
        }
    }

    void AddPlaylist(std::string title, std::string author, UnityEngine::Sprite* coverImage) {
        // create playlist with info
        auto newPlaylist = BPList();
        newPlaylist.PlaylistTitle = title;
        if(author != "")
            newPlaylist.PlaylistAuthor = author;
        auto bytes = UnityEngine::ImageConversion::EncodeToPNG(coverImage->get_texture());
        newPlaylist.ImageString = STR(System::Convert::ToBase64String(bytes));
        // save playlist
        std::string fileTitle = SanitizeFileName(title);
        while(!UniqueFileName(fileTitle + ".bplist_BMBF.json", GetPlaylistsPath()))
            fileTitle += "_";
        WriteToFile(GetPlaylistsPath() + "/" + fileTitle + ".bplist_BMBF.json", newPlaylist);
        RefreshPlaylists(); // load it, with the others because I'm lazy
    }

    void MovePlaylist(BPList* playlist, int index) {
        int originalIndex = GetPackIndex(playlist->PlaylistTitle);
        if(originalIndex < 0) {
            LOG_ERROR("Attempting to move unloaded playlist");
            return;
        }
        playlistConfig.Order.erase(playlistConfig.Order.begin() + originalIndex);
        playlistConfig.Order.insert(playlistConfig.Order.begin() + index, playlist->PlaylistTitle);
        WriteToFile(GetConfigPath(), playlistConfig);
    }

    void RenamePlaylist(BPList* playlist, std::string title) {
        std::string oldName = playlist->PlaylistTitle;
        std::string oldPath = playlist->path;
        int orderIndex = GetPackIndex(playlist->PlaylistTitle);
        if(orderIndex < 0) {
            LOG_ERROR("Attempting to rename unloaded playlist");
            return;
        }
        // update name in json map
        auto map_pos = playlists_json.find(playlist->PlaylistTitle);
        if(map_pos == playlists_json.end()) {
            LOG_ERROR("Could not find playlist name");
            return;
        }
        // edit variables
        playlistConfig.Order[orderIndex] = title;
        playlist->PlaylistTitle = title;
        playlists_json.erase(map_pos);
        playlists_json.insert({title, playlist});
        // change name in all folders
        for(auto& folder : playlistConfig.Folders) {
            for(int i = 0; i < folder.PlaylistNames.size(); i++) {
                if(folder.PlaylistNames[i] == oldName)
                    folder.PlaylistNames[i] = title;
            }
        }
        // rename playlist ingame
        auto levelPack = GetSongloaderPack(playlist);
        if(levelPack) {
            auto nameCS = CSTR(playlist->PlaylistTitle);
            levelPack->CustomLevelsPack->packName = nameCS;
            levelPack->CustomLevelsPack->shortPackName = nameCS;
        }
        // save changes
        WriteToFile(GetConfigPath(), playlistConfig);
        WriteToFile(oldPath, *playlist);
        // rename file
        std::string fileTitle = SanitizeFileName(title);
        while(!UniqueFileName(fileTitle + ".bplist_BMBF.json", GetPlaylistsPath()))
            fileTitle += "_";
        std::string newPath = GetPlaylistsPath() + "/" + fileTitle + ".bplist_BMBF.json";
        std::filesystem::rename(oldPath, newPath);
        playlist->path = newPath;
        // update path dictionary
        if(levelPack) {
            path_playlists->Remove(CSTR(oldPath));
            path_playlists->Add(CSTR(newPath), levelPack);
        }
    }

    void ChangePlaylistCover(BPList* playlist, UnityEngine::Sprite* newCover, int coverIndex) {
        // don't save string for default cover
        bool isDefault = false;
        if(!newCover) {
            LOG_INFO("Changing playlist cover to default");
            newCover = GetDefaultCoverImage();
            isDefault = true;
        }
        playlist->imageIndex = coverIndex;
        if(isDefault)
            playlist->ImageString = std::nullopt;
        else {
            LOG_INFO("Changing playlist cover");
            // save image base 64
            auto bytes = UnityEngine::ImageConversion::EncodeToPNG(newCover->get_texture());
            playlist->ImageString = STR(System::Convert::ToBase64String(bytes));
        }
        // change cover ingame
        auto levelPack = GetSongloaderPack(playlist);
        if(levelPack)
            levelPack->CustomLevelsPack->coverImage = newCover;
        WriteToFile(playlist->path, *playlist);
    }

    void DeletePlaylist(std::string title) {
        std::string path = GetPlaylistJSON(title)->path;
        // remove from loaded playlists
        path_playlists->Remove(CSTR(path));
        // delete file
        std::filesystem::remove(path);
        // remove name from order config
        int orderIndex = GetPackIndex(title);
        playlistConfig.Order.erase(playlistConfig.Order.begin() + orderIndex);
        // reload
        RefreshPlaylists();
    }

    void RefreshPlaylists() {
        bool showDefaults = folderSelectionState == 0 || folderSelectionState == 1;
        if(folderSelectionState == 3 && currentFolder)
            showDefaults = currentFolder->ShowDefaults;
        API::RefreshPacks(showDefaults);
    }

    std::string GetLevelHash(GlobalNamespace::CustomPreviewBeatmapLevel* level) {
        std::string id = STR(level->levelID);
        // should be in all songloader levels
        auto prefixIndex = id.find("custom_level_");
        if(prefixIndex == std::string::npos)
            return "";
        // remove prefix
        id = id.substr(prefixIndex + 14);
        auto wipIndex = id.find(" WIP");
        if(wipIndex != std::string::npos)
            id = id.substr(0, wipIndex);
        std::transform(id.begin(), id.end(), id.begin(), toupper);
        return id;
    }

    void AddSongToPlaylist(BPList* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level) {
        // add a blank song
        playlist->Songs.emplace_back(BPSong());
        // set info
        auto& songJson = playlist->Songs[playlist->Songs.size()  - 1];
        songJson.Hash = GetLevelHash(level);
        songJson.SongName = STR(level->songName);
        // write to file
        WriteToFile(playlist->path, *playlist);
    }

    void RemoveSongFromPlaylist(BPList* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level) {
        // find song by hash (since the field is required) and remove
        auto levelHash = GetLevelHash(level);
        for(auto itr = playlist->Songs.begin(); itr != playlist->Songs.end(); ++itr) {
            auto& song = *itr;
            std::transform(song.Hash.begin(), song.Hash.end(), song.Hash.begin(), toupper);
            std::transform(levelHash.begin(), levelHash.end(), levelHash.begin(), toupper);
            if(song.Hash == levelHash) {
                playlist->Songs.erase(itr);
                break;
            }
        }
        // write to file
        WriteToFile(playlist->path, *playlist);
    }
}
