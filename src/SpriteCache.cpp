#include "SpriteCache.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Image.hpp"

#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"

using namespace UnityEngine;
using namespace QuestUI;

std::unordered_map<Sprite*, WeakPtrGO<GameObject>> caches;

void CacheSprite(Sprite* sprite) {
    if(!caches.contains(sprite)) {
        static ConstString name("PlaylistManagerCachedSprite");
        auto object = WeakPtrGO(GameObject::New_ctor(name));
        object->AddComponent<UI::Image*>()->set_sprite(sprite);
        Object::DontDestroyOnLoad(object);
        caches.insert({sprite, object});
    }
}

void RemoveCachedSprite(Sprite* sprite) {
    if(caches.contains(sprite)) {
        auto object = caches.find(sprite)->second;
        if(object.isValid())
            Object::Destroy(caches.find(sprite)->second);
        caches.erase(sprite);
    }
}

void ClearCachedSprites() {
    for(auto& pair : caches) {
        auto object = pair.second;
        if(object.isValid())
            Object::Destroy(object);
    }
    caches.clear();
}
