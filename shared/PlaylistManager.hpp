#pragma once
#include "Types/BPList.hpp"
#include <string>

namespace PlaylistManager {

    std::optional<BPList> ReadFromFile(std::string_view path);

}
