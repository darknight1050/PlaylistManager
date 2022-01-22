#include "Main.hpp"
#include "Types/BPList.hpp"
#include "Types/Config.hpp"
#include "PlaylistManager.hpp"
#include "ResettableStaticPtr.hpp"
#include "Settings.hpp"
#include "Utils.hpp"

#include <filesystem>
#include <fstream>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "songloader/shared/API.hpp"

#include "System/Convert.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

using namespace RuntimeSongLoader;

bool ShouldAddPack(std::string name) {
    if(currentFolder && folderSelectionState == 3) {
        for(std::string testName : currentFolder->PlaylistNames) {
            if(name == testName)
                return true;
        }
    }
    return folderSelectionState == 0 || folderSelectionState == 2;
}

namespace PlaylistManager {
    
    std::unordered_map<std::string, Playlist*> name_playlists;
    std::unordered_map<std::string, Playlist*> path_playlists;
    std::unordered_map<UnityEngine::Sprite*, std::string> image_paths;

    std::hash<std::string> hasher;
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
    
    int GetPackIndex(std::string title) {
        // find index of playlist title in config
        int size = playlistConfig.Order.size();
        for(int i = 0; i < size; i++) {
            if(playlistConfig.Order[i] == title)
                return i;
        }
        // add to end of config if not found
        playlistConfig.Order.push_back(title);
        WriteToFile(GetConfigPath(), playlistConfig);
        return -1;
    }
    
    UnityEngine::Sprite* GetDefaultCoverImage() {
        STATIC_AUTO(defaultCover, UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::CustomLevelLoader*>()[0]->defaultPackCover);
        return defaultCover;
    }

    UnityEngine::Sprite* GetCoverImage(Playlist* playlist) {
        auto& json = playlist->playlistJSON;
        if(json.ImageString.has_value()) {
            std::string imageBase64 = json.ImageString.value();
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
            // get and write texture
            auto texture = UnityEngine::Texture2D::New_ctor(0, 0, UnityEngine::TextureFormat::RGBA32, false, false);
            UnityEngine::ImageConversion::LoadImage(texture, System::Convert::FromBase64String(CSTR(imageBase64)));
            // process texture size and png string and check hash for changes
            std::size_t oldHash = imgHash;
            imageBase64 = ProcessImage(texture, true);
            imgHash = hasher(imageBase64);
            // write to playlist if changed
            if(imgHash != oldHash) {
                json.ImageString = imageBase64;
                WriteToFile(playlist->path, json);
            }
            foundItr = imageHashes.find(imgHash);
            if (foundItr != imageHashes.end()) {
                LOG_INFO("Returning loaded image with hash %lu", imgHash);
                playlist->imageIndex = foundItr->second;
                return loadedImages[foundItr->second];
            }
            // save image as file and return
            LOG_INFO("Writing image with hash %lu", imgHash);
            // reuse playlist file name
            std::string playlistPathName = std::filesystem::path(playlist->path).stem();
            std::string imgPath = GetCoversPath() + "/" + playlistPathName + ".png";
            WriteImageToFile(imgPath, texture);
            imageHashes.insert({imgHash, loadedImages.size()});
            auto sprite = UnityEngine::Sprite::Create(texture, UnityEngine::Rect(0, 0, texture->get_width(), texture->get_height()), {0.5, 0.5}, 1024, 1, UnityEngine::SpriteMeshType::FullRect, {0, 0, 0, 0}, false);
            image_paths.insert({sprite, imgPath});
            playlist->imageIndex = loadedImages.size();
            loadedImages.emplace_back(sprite);
            return sprite;
        }
        return GetDefaultCoverImage();
    }

    void DeleteLoadedImage(UnityEngine::Sprite* image) {
        // get path
        auto foundItr = image_paths.find(image);
        if (foundItr == image_paths.end())
            return;
        // find image index
        for(int i = 0; i < loadedImages.size(); i++) {
            if(loadedImages[i] == image) {
                // update image indices of playlists
                for(auto& playlist : GetLoadedPlaylists()) {
                    if(playlist->imageIndex == i)
                        playlist->imageIndex = -1;
                    if(playlist->imageIndex > i)
                        playlist->imageIndex--;
                }
                // remove from loaded images
                loadedImages.erase(loadedImages.begin() + i);
                // remove from image hashes
                std::unordered_map<std::size_t, int>::iterator removeItr;
                for(auto itr = imageHashes.begin(); itr != imageHashes.end(); itr++) {
                    if(itr->second == i) {
                        removeItr = itr;
                    }
                    // decrement all indices later than i
                    if(itr->second > i) {
                        itr->second--;
                    }
                }
                imageHashes.erase(removeItr);
                break;
            }
        }
        std::filesystem::remove(foundItr->second);
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
                auto texture = UnityEngine::Texture2D::New_ctor(0, 0, UnityEngine::TextureFormat::RGBA32, false, false);
                UnityEngine::ImageConversion::LoadImage(texture, bytes);
                std::size_t oldHash = imgHash;
                imgHash = hasher(ProcessImage(texture, true));
                if(imgHash != oldHash)
                    WriteImageToFile(path.string(), texture);
                // check hash with loaded images
                if(imageHashes.contains(imgHash)) {
                    LOG_INFO("Skipping loading image with hash %lu", imgHash);
                    continue;
                }
                LOG_INFO("Loading image with hash %lu", imgHash);
                // rename file with hash for identification (coolImage -> coolImage_1987238271398)
                // could be useful if images from playlist files are loaded first, but atm they are not
                // auto searchIndex = path.stem().string().rfind("_") + 1;
                // if(path.stem().string().substr(searchIndex) != std::to_string(imgHash)) {
                //     LOG_INFO("Renaming image file with hash %lu", imgHash);
                //     std::filesystem::rename(path, path.parent_path() / (path.stem().string() + "_" + std::to_string(imgHash) + ".png"));
                // }
                imageHashes.insert({imgHash, loadedImages.size()});
                auto sprite = UnityEngine::Sprite::Create(texture, UnityEngine::Rect(0, 0, texture->get_width(), texture->get_height()), {0.5, 0.5}, 1024, 1, UnityEngine::SpriteMeshType::FullRect, {0, 0, 0, 0}, false);
                image_paths.insert({sprite, path});
                loadedImages.emplace_back(sprite);
            }
        }
    }

    std::vector<UnityEngine::Sprite*>& GetLoadedImages() {
        return loadedImages;
    }

    void ClearLoadedImages() {
        loadedImages.clear();
        image_paths.clear();
        imageHashes.clear();
    }

    bool AvailablePlaylistName(std::string title) {
        // check in constant playlists
        if(staticPacks.contains(title))
            return false;
        // check in custom playlists
        for(auto& pair : name_playlists) {
            if(pair.second->name == title)
                return false;
        }
        return true;
    }

    std::string GetPath(std::string title) {
        std::string fileTitle = SanitizeFileName(title);
        while(!UniqueFileName(fileTitle + ".bplist_BMBF.json", GetPlaylistsPath()))
            fileTitle += "_";
        return GetPlaylistsPath() + "/" + fileTitle + ".bplist_BMBF.json";
    }

    void LoadPlaylists(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool fullRefresh) {
        // load images if none are loaded
        if(loadedImages.size() < 1)
            GetCoverImages();
        // clear playlists if requested
        if(fullRefresh) {
            for(auto& pair : name_playlists)
                delete pair.second;
            name_playlists.clear();
            path_playlists.clear();
        }
        // ensure path exists
        auto path = GetPlaylistsPath();
        if(!std::filesystem::is_directory(path))
            return;
        // create array for playlists
        std::vector<GlobalNamespace::CustomBeatmapLevelPack*> sortedPlaylists(playlistConfig.Order.size());
        // iterate through all playlist files
        for(const auto& entry : std::filesystem::directory_iterator(path)) {
            if(!entry.is_directory()) {
                // check if playlist has been loaded already
                auto path = entry.path().string();
                auto path_iter = path_playlists.find(path);
                if(path_iter != path_playlists.end()) {
                    LOG_INFO("Loading playlist file %s from cache", path.c_str());
                    // check if playlist should be added
                    auto playlist = path_iter->second;
                    if(ShouldAddPack(playlist->name)) {
                        int packPosition = GetPackIndex(playlist->name);
                        // add if new (idk how)
                        if(packPosition < 0)
                            sortedPlaylists.emplace_back(playlist->playlistCS);
                        else
                            sortedPlaylists[packPosition] = (GlobalNamespace::CustomBeatmapLevelPack*) playlist->playlistCS;
                    }
                } else {
                    LOG_INFO("Loading playlist file %s", path.c_str());
                    // get playlist object from file
                    Playlist* playlist = new Playlist();
                    if(ReadFromFile(path, playlist->playlistJSON)) {
                        playlist->name = playlist->playlistJSON.PlaylistTitle;
                        playlist->path = path;
                        // check for duplicate name
                        while(name_playlists.find(playlist->name) != name_playlists.end()) {
                            LOG_INFO("Duplicate playlist name!");
                            playlist->name = playlist->name + ".";
                            playlist->playlistJSON.PlaylistTitle = playlist->name;
                            WriteToFile(path, playlist->playlistJSON);
                        }
                        name_playlists.insert({playlist->name, playlist});
                        path_playlists.insert({playlist->path, playlist});
                        // create playlist object
                        // SongLoaderCustomBeatmapLevelPack* songloaderBeatmapLevelPack = SongLoaderCustomBeatmapLevelPack::New_ctor(playlist->name, playlist->name, GetCoverImage(playlist));
                        auto songloaderBeatmapLevelPack = *il2cpp_utils::New<SongLoaderCustomBeatmapLevelPack*>(CSTR("custom_levelPack_" + playlist->name), CSTR(playlist->name), GetCoverImage(playlist));
                        playlist->playlistCS = songloaderBeatmapLevelPack->CustomLevelsPack;
                        // add all songs to the playlist object
                        auto foundSongs = List<GlobalNamespace::CustomPreviewBeatmapLevel*>::New_ctor();
                        for(auto& song : playlist->playlistJSON.Songs) {
                            auto search = RuntimeSongLoader::API::GetLevelByHash(song.Hash);
                            if(search.has_value())
                                foundSongs->Add(search.value());
                        }
                        songloaderBeatmapLevelPack->SetCustomPreviewBeatmapLevels(foundSongs->ToArray());
                        // add the playlist to the sorted array
                        if(ShouldAddPack(playlist->name)) {
                            int packPosition = GetPackIndex(playlist->name);
                            // add if new
                            if(packPosition < 0)
                                sortedPlaylists.emplace_back(songloaderBeatmapLevelPack->CustomLevelsPack);
                            else
                                sortedPlaylists[packPosition] = songloaderBeatmapLevelPack->CustomLevelsPack;
                        }
                    } else
                        delete playlist;
                }
            }
        }
        LOG_INFO("Adding playlists");
        // add playlists to game in sorted order
        for(auto customBeatmapLevelPack : sortedPlaylists) {
            if(customBeatmapLevelPack)
                customBeatmapLevelPackCollectionSO->AddLevelPack(customBeatmapLevelPack);
        }
        LOG_INFO("Playlists loaded");
    }

    std::vector<Playlist*> GetLoadedPlaylists() {
        // create return vector with base size
        std::vector<Playlist*> ret(playlistConfig.Order.size());
        for(auto& pair : name_playlists) {
            auto& playlist = pair.second;
            int idx = GetPackIndex(playlist->name);
            if(idx >= 0)
                ret[idx] = playlist;
            else
                ret.push_back(playlist);
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

    Playlist* GetPlaylist(std::string title) {
        auto iter = name_playlists.find(title);
        if(iter == name_playlists.end())
            return nullptr;
        return iter->second;
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
        std::string path = GetPath(title);
        WriteToFile(path, newPlaylist);
    }

    void MovePlaylist(Playlist* playlist, int index) {
        int originalIndex = GetPackIndex(playlist->name);
        if(originalIndex < 0) {
            LOG_ERROR("Attempting to move unloaded playlist");
            return;
        }
        playlistConfig.Order.erase(playlistConfig.Order.begin() + originalIndex);
        playlistConfig.Order.insert(playlistConfig.Order.begin() + index, playlist->name);
        WriteToFile(GetConfigPath(), playlistConfig);
    }

    void RenamePlaylist(Playlist* playlist, std::string title) {
        std::string oldName = playlist->name;
        std::string oldPath = playlist->path;
        int orderIndex = GetPackIndex(playlist->name);
        if(orderIndex < 0) {
            LOG_ERROR("Attempting to rename unloaded playlist");
            return;
        }
        // update name in map
        auto name_iter = name_playlists.find(playlist->name);
        if(name_iter == name_playlists.end()) {
            LOG_ERROR("Could not find playlist by name");
            return;
        }
        // also update path
        auto path_iter = path_playlists.find(playlist->name);
        if(path_iter == path_playlists.end()) {
            LOG_ERROR("Could not find playlist by path");
            return;
        }
        std::string newPath = GetPath(title);
        // edit variables
        playlistConfig.Order[orderIndex] = title;
        playlist->name = title;
        playlist->playlistJSON.PlaylistTitle = title;
        name_playlists.erase(name_iter);
        name_playlists.insert({title, playlist});
        path_playlists.erase(path_iter);
        path_playlists.insert({newPath, playlist});
        // change name in all folders
        for(auto& folder : playlistConfig.Folders) {
            for(int i = 0; i < folder.PlaylistNames.size(); i++) {
                if(folder.PlaylistNames[i] == oldName)
                    folder.PlaylistNames[i] = title;
            }
        }
        // rename playlist ingame
        auto& levelPack = playlist->playlistCS;
        if(levelPack) {
            auto nameCS = CSTR(playlist->name);
            levelPack->packName = nameCS;
            levelPack->shortPackName = nameCS;
        }
        // save changes
        WriteToFile(GetConfigPath(), playlistConfig);
        WriteToFile(oldPath, playlist->playlistJSON);
        // rename file
        std::filesystem::rename(oldPath, newPath);
        playlist->path = newPath;
    }

    void ChangePlaylistCover(Playlist* playlist, UnityEngine::Sprite* newCover, int coverIndex) {
        // don't save string for default cover
        bool isDefault = false;
        if(!newCover) {
            LOG_INFO("Changing playlist cover to default");
            newCover = GetDefaultCoverImage();
            isDefault = true;
        }
        playlist->imageIndex = coverIndex;
        auto& json = playlist->playlistJSON;
        if(isDefault)
            json.ImageString = std::nullopt;
        else {
            LOG_INFO("Changing playlist cover");
            // save image base 64
            auto bytes = UnityEngine::ImageConversion::EncodeToPNG(newCover->get_texture());
            json.ImageString = STR(System::Convert::ToBase64String(bytes));
        }
        // change cover ingame
        auto levelPack = playlist->playlistCS;
        if(levelPack) {
            levelPack->coverImage = newCover;
            levelPack->smallCoverImage = newCover;
        }
        WriteToFile(playlist->path, json);
    }

    void DeletePlaylist(Playlist* playlist) {
        // remove from maps
        auto name_iter = name_playlists.find(playlist->name);
        if(name_iter == name_playlists.end()) {
            LOG_ERROR("Could not find playlist by name");
            return;
        }
        auto path_iter = path_playlists.find(playlist->path);
        if(path_iter == path_playlists.end()) {
            LOG_ERROR("Could not find playlist by path");
            return;
        }
        name_playlists.erase(name_iter);
        path_playlists.erase(path_iter);
        // delete file
        std::filesystem::remove(playlist->path);
        // remove name from order config
        int orderIndex = GetPackIndex(playlist->name);
        if(orderIndex >= 0)
            playlistConfig.Order.erase(playlistConfig.Order.begin() + orderIndex);
        else
            playlistConfig.Order.erase(playlistConfig.Order.end() - 1);
        // delete playlist object
        delete playlist;
    }

    void RefreshPlaylists(bool fullRefresh) {
        bool showDefaults = folderSelectionState == 0 || folderSelectionState == 1;
        if(folderSelectionState == 3 && currentFolder)
            showDefaults = currentFolder->ShowDefaults;
        if(fullRefresh) {
            for(auto& pair : name_playlists)
                delete pair.second;
            name_playlists.clear();
            path_playlists.clear();
        }
        API::RefreshPacks(showDefaults);
    }

    void AddSongToPlaylist(Playlist* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level) {
        // add song to cs object
        auto pack = playlist->playlistCS;
        auto levelArr = pack->customBeatmapLevelCollection->customPreviewBeatmapLevels;
        // can use an array since we know its length
        ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*> newLevels(levelArr.Length() + 1);
        for(int i = 0; i < levelArr.Length(); i++) {
            newLevels[i] = levelArr[i];
        }
        newLevels[newLevels.Length()] = (GlobalNamespace::CustomPreviewBeatmapLevel*) level;
        pack->customBeatmapLevelCollection->customPreviewBeatmapLevels = newLevels;
        // update json object
        auto& json = playlist->playlistJSON;
        // add a blank song
        json.Songs.emplace_back(BPSong());
        // set info
        auto& songJson = *(json.Songs.end() - 1);
        songJson.Hash = GetLevelHash(level);
        songJson.SongName = STR(level->songName);
        // write to file
        WriteToFile(playlist->path, json);
    }

    void RemoveSongFromPlaylist(Playlist* playlist, GlobalNamespace::CustomPreviewBeatmapLevel* level) {
        // remove song from cs object
        auto pack = playlist->playlistCS;
        if(!pack)
            return;
        auto levelArr = pack->customBeatmapLevelCollection->customPreviewBeatmapLevels;
        // can use an array since we know its length
        ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*> newLevels(levelArr.Length() - 1);
        // remove only one level if duplicates
        bool removed = false;
        for(int i = 0; i < levelArr.Length(); i++) {
            // comparison should work
            auto currentLevel = levelArr[i];
            if(removed)
                newLevels[i - 1] = currentLevel;
            else if(currentLevel != level)
                newLevels[i] = currentLevel;
            else
                removed = true;
        }
        pack->customBeatmapLevelCollection->customPreviewBeatmapLevels = newLevels;
        // update json object
        auto& json = playlist->playlistJSON;
        // find song by hash (since the field is required) and remove
        auto levelHash = GetLevelHash(level);
        for(auto itr = json.Songs.begin(); itr != json.Songs.end(); ++itr) {
            auto& song = *itr;
            LOWER(song.Hash);
            if(song.Hash == levelHash) {
                json.Songs.erase(itr);
                // only erase
                break;
            }
        }
        // write to file
        WriteToFile(playlist->path, json);
    }
}
