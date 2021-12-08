#pragma once

#include "UnityEngine/Sprite.hpp"

#define SPRITE(name) UnityEngine::Sprite* name##Sprite();
#define ACTIVE_INACTIVE(name) UnityEngine::Sprite* name##Sprite(); UnityEngine::Sprite* name##InactiveSprite();

SPRITE(White);
SPRITE(AllPacks);
SPRITE(DefaultPacks);
SPRITE(CustomPacks);
SPRITE(PackFolders);
SPRITE(Folder);
ACTIVE_INACTIVE(LeftCarat);
ACTIVE_INACTIVE(RightCarat);
ACTIVE_INACTIVE(SaveCover);
ACTIVE_INACTIVE(AddToPlaylist);
ACTIVE_INACTIVE(RemoveFromPlaylist);

#undef SPRITE
#undef ACTIVE_INACTIVE