#pragma once

#include "Types/CustomListSource.hpp"

#include "HMUI/ImageView.hpp"
#include "HMUI/HoverHint.hpp"

DECLARE_CLASS_CUSTOM(PlaylistManager, CoverTableCell, PlaylistManager::CustomTableCell,

    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, coverImage);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, selectedImage);
    DECLARE_INSTANCE_FIELD(HMUI::HoverHint*, hoverHint);

    DECLARE_CTOR(ctor);
    void refreshVisuals();
    void init(UnityEngine::Sprite* sprite, std::string text);
    void setSprite(UnityEngine::Sprite* sprite);
    void setText(std::string text);
)
