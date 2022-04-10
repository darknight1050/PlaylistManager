#pragma once

#include "HMUI/ViewController.hpp"

#include "custom-types/shared/macros.hpp"

DECLARE_CLASS_CODEGEN(PlaylistManager, SettingsViewController, HMUI::ViewController,

    bool needsRestart = false;

    public:
    static void DestroyUI();

    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::ViewController::DidActivate>::get(), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD(void, DidDeactivate, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::ViewController::DidDeactivate>::get(), bool removedFromHierarchy, bool screenSystemDisabling);
)