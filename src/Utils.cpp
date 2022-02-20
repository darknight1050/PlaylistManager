#include "Main.hpp"
#include "Utils.hpp"

#include "UnityEngine/ImageConversion.hpp"
#include "System/Convert.hpp"

#include <filesystem>

// desired image size
const int imageSize = 512;

std::string GetLevelHash(GlobalNamespace::CustomPreviewBeatmapLevel* level) {
    std::string id = level->levelID;
    // should be in all songloader levels
    auto prefixIndex = id.find("custom_level_");
    if(prefixIndex == std::string::npos)
        return "";
    // remove prefix
    id = id.substr(prefixIndex + 13);
    auto wipIndex = id.find(" WIP");
    if(wipIndex != std::string::npos)
        id = id.substr(0, wipIndex);
    LOWER(id);
    return id;
}

std::string SanitizeFileName(std::string const& fileName) {
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

bool UniqueFileName(std::string const& fileName, std::string const& compareDirectory) {
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

std::string GetNewPlaylistPath(std::string const& title) {
    std::string fileTitle = SanitizeFileName(title);
    while(!UniqueFileName(fileTitle + ".bplist_BMBF.json", GetPlaylistsPath()))
        fileTitle += "_";
    return GetPlaylistsPath() + "/" + fileTitle + ".bplist_BMBF.json";
}

std::string GetBase64ImageType(std::string const& base64) {
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

std::string ProcessImage(UnityEngine::Texture2D* texture, bool returnPngString) {
    // check texture size and resize if necessary
    int width = texture->get_width();
    int height = texture->get_height();
    if(width > imageSize && height > imageSize) {
        // resize (https://gist.github.com/gszauer/7799899 modified for only downscaling)
        auto texColors = texture->GetPixels();
        ArrayW<UnityEngine::Color> newColors(imageSize * imageSize);
        float ratio_x = ((float) width - 1) / imageSize;
        float ratio_y = ((float) height - 1) / imageSize;

        for(int y = 0; y < imageSize; y++) {
            int offset_from_y = y * imageSize;

            int old_texture_y = floor(y * ratio_y);
            int old_texture_offset_from_y = old_texture_y * width;
            
            for(int x = 0; x < imageSize; x++) {
                int old_texture_x = floor(x * ratio_x);

                newColors[offset_from_y + x] = texColors[old_texture_offset_from_y + old_texture_x];
            }
        }
        texture->Resize(imageSize, imageSize);
        texture->SetPixels(newColors);
        texture->Apply();
    }
    if(!returnPngString)
        return "";
    // convert to png if necessary
    // can sometimes need two passes to reach a fixed result
    // but I don't want to to it twice for every cover, so it will just normalize itself after another restart
    // and it shouldn't be a problem through ingame cover management
    auto bytes = UnityEngine::ImageConversion::EncodeToPNG(texture);
    return System::Convert::ToBase64String(bytes);
}

void WriteImageToFile(std::string const& pathToPng, UnityEngine::Texture2D* texture) {
    auto bytes = UnityEngine::ImageConversion::EncodeToPNG(texture);
    writefile(pathToPng, std::string((char*) bytes.begin(), bytes.Length()));
}