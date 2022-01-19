#pragma once

#include "HMUI/ViewController.hpp"

namespace PlaylistManager {
    void ModSettingsDidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

    void DestroyUI();

    void CreateUI();
}