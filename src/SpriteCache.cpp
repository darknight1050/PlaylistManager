#include "SpriteCache.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Image.hpp"

using namespace UnityEngine;

std::unordered_map<Sprite*, GameObject*> caches;

void CacheSprite(Sprite* sprite) {
    if(!caches.contains(sprite)) {
        static ConstString name("PlaylistManagerCachedSprite");
        auto object = GameObject::New_ctor(name);
        object->AddComponent<UI::Image*>()->set_sprite(sprite);
        Object::DontDestroyOnLoad(object);
        caches.insert({sprite, object});
    }
}

void RemoveCachedSprite(Sprite* sprite) {
    if(caches.contains(sprite)) {
        Object::Destroy(caches.find(sprite)->second);
        caches.erase(sprite);
    }
}

void ClearCachedSprites() {
    for(auto& pair : caches)
        Object::Destroy(pair.second);
    caches.clear();
}
