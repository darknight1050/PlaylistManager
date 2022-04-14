#include "Main.hpp"
#include "Types/Scroller.hpp"
#include "Types/Config.hpp"
#include "ResettableStaticPtr.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsGridViewAnimator.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "HMUI/EventSystemListener.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Time.hpp"

#include "System/Action_1.hpp"

// a scroller in multiple ways specialized for the playlist grid
// however, it could likely be made more versatile with a few changes
// of course, it would be nice if I could just use a regular ScrollView without it crashing...

DEFINE_TYPE(PlaylistManager, Scroller);

#define ACTION_1(type, methodname) il2cpp_utils::MakeDelegate<System::Action_1<type>*>((std::function<void(type)>) [this](type arg){if(this->cachedPtr == this) methodname(arg);})

using namespace PlaylistManager;

float fixedCellHeight = 15;

void UpdateScrollSpeed() {
    Scroller::scrollSpeed() = 45 * playlistConfig.ScrollSpeed;
}

void Scroller::Awake() {
    platformHelper = FindComponent<GlobalNamespace::VRController*>()->vrPlatformHelper;

    auto eventListener = GetComponent<HMUI::EventSystemListener*>();
    if(!eventListener)
        eventListener = get_gameObject()->AddComponent<HMUI::EventSystemListener*>();
    eventListener->add_pointerDidEnterEvent(ACTION_1(UnityEngine::EventSystems::PointerEventData*, HandlePointerDidEnter));
    eventListener->add_pointerDidExitEvent(ACTION_1(UnityEngine::EventSystems::PointerEventData*, HandlePointerDidExit));
    
    set_enabled(false);
}

void Scroller::Update() {
    if(!contentTransform)
        return;
    auto pos = contentTransform->get_anchoredPosition().y;
    float newPos = std::lerp(pos, destinationPos, UnityEngine::Time::get_deltaTime() * 8);
    if (std::abs(newPos - destinationPos) < 0.01)
    {
        newPos = destinationPos;
        set_enabled(false);
    }
    contentTransform->set_anchoredPosition({0, newPos});
}

void Scroller::OnDestroy() {
    // would be better to remove the delegate, I'll try that out when codegen updates to the bshook with it and better virtuals
    cachedPtr = nullptr;
}

void Scroller::Init(UnityEngine::RectTransform* content) {
    content->set_anchoredPosition({0, content->get_anchoredPosition().y});
    contentTransform = content;
    UpdateScrollSpeed();
    cachedPtr = this;
}

void Scroller::HandlePointerDidEnter(UnityEngine::EventSystems::PointerEventData* pointerEventData) {
    if(!addedDelegate) {
        platformHelper->add_joystickWasNotCenteredThisFrameEvent(ACTION_1(UnityEngine::Vector2, HandleJoystickWasNotCenteredThisFrame));
        addedDelegate = true;
    }
    pointerHovered = true;
    float pos = FindComponent<GlobalNamespace::AnnotatedBeatmapLevelCollectionsGridViewAnimator*>()->GetContentYOffset();
    SetDestinationPos(pos);
}

void Scroller::HandlePointerDidExit(UnityEngine::EventSystems::PointerEventData* pointerEventData) {
    pointerHovered = false;
    destinationPos = FindComponent<GlobalNamespace::AnnotatedBeatmapLevelCollectionsGridViewAnimator*>()->GetContentYOffset();
    set_enabled(true);
}

void Scroller::HandleJoystickWasNotCenteredThisFrame(UnityEngine::Vector2 deltaPos) {
    if(!pointerHovered)
        return;
    float num = destinationPos;
    num -= deltaPos.y * UnityEngine::Time::get_deltaTime() * Scroller::scrollSpeed();
    SetDestinationPos(num);
}

void Scroller::SetDestinationPos(float value) {
    if(!contentTransform)
        return;
    float contentSize = contentTransform->get_rect().get_height();
    float afterEndPageSize = fixedCellHeight * 4;
    float zeroPos = -(contentSize - fixedCellHeight) / 2;
    float endPos = -zeroPos - afterEndPageSize;
    float difference = contentSize - afterEndPageSize;
    if(difference <= 0)
        destinationPos = zeroPos;
    else
        destinationPos = std::clamp(value, zeroPos, endPos);
    set_enabled(true);
}