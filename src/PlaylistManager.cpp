#include "PlaylistManager.hpp"

namespace PlaylistManager {

    std::optional<BPList> ReadFromFile(std::string_view path) {
        if(!fileexists(path))
            return std::nullopt;
        auto json = readfile(path);
        rapidjson::Document document;
        document.Parse(json);
        if(document.HasParseError() || !document.IsObject())
            return std::nullopt;
        BPList list;
        list.Deserialize(document.GetObject());
        return list;
    }
    
}
