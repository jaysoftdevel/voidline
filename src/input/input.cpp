#include "input/input.hpp"
#include <algorithm>
#include <cmath>

namespace vl {
namespace {
constexpr float deadZone = .24f;
float axisValue(short raw) {
    const float v = std::clamp(raw / 32767.f, -1.f, 1.f);
    const float m = std::abs(v);
    if (m < deadZone) return 0;
    return (v < 0 ? -1.f : 1.f) * (m - deadZone) / (1 - deadZone);
}
}

Input::Input() {
    resetBindings();
    openPad();
}

void Input::resetBindings() {
    for (size_t i = 0; i < actCount; ++i) bindings[i] = defaultBinding(static_cast<Act>(i));
}

void Input::configure(const Settings& settings) {
    resetBindings();
    for (size_t a = 0; a < actCount; ++a) {
        if (!actionInfo(static_cast<Act>(a)).rebindable) continue;
        // An action with any stored slot is taken as the player's choice. -1 everywhere leaves the defaults.
        bool custom = false;
        for (size_t s = 0; s < keysPerAction; ++s) custom = custom || settings.keymap[a * keysPerAction + s] != -1;
        if (!custom) continue;
        for (size_t s = 0; s < keysPerAction; ++s) {
            const int value = settings.keymap[a * keysPerAction + s];
            bindings[a].keys[s] = value > 0 && value < sdl::scancodes ? value : 0;
        }
    }
    sprintMode = std::clamp(settings.sprintMode, 0, 2);
    crouchToggle = settings.crouchToggle;
    sprintLatched = crouchLatched = false;
}

std::array<int,keymapSize> Input::keymap() const {
    std::array<int,keymapSize> out{};
    out.fill(-1);
    for (size_t a = 0; a < actCount; ++a) {
        const Binding factory = defaultBinding(static_cast<Act>(a));
        if (bindings[a].keys == factory.keys) continue;
        for (size_t s = 0; s < keysPerAction; ++s) out[a * keysPerAction + s] = bindings[a].keys[s];
    }
    return out;
}

bool Input::assign(Act act, size_t slot, int scancode) {
    if (reservedKey(scancode) || slot >= keysPerAction || !actionInfo(act).rebindable) return false;
    // A key belongs to one action at a time. Taking it frees it elsewhere.
    for (auto& b : bindings)
        for (int& k : b.keys) if (k == scancode) k = 0;
    bindings[index(act)].keys[slot] = scancode;
    return true;
}

void Input::clear(Act act, size_t slot) {
    if (slot < keysPerAction) bindings[index(act)].keys[slot] = 0;
}

void Input::openPad() {
    if (pad) return;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (!SDL_IsGameController(i)) continue;
        pad = SDL_GameControllerOpen(i);
        if (pad) return;
    }
}
void Input::closePad() {
    if (pad) SDL_GameControllerClose(pad);
    pad = nullptr;
}

void Input::event(const SDL_Event& e) {
    auto latch = [&](auto predicate, bool isDown) {
        for (size_t i = 0; i < actCount; ++i) {
            if (!predicate(bindings[i])) continue;
            (isDown ? state[i].latchedDown : state[i].latchedUp) = true;
        }
    };
    switch (e.type) {
    case sdl::keyDown:
        if (e.key.keysym.scancode > 0 && e.key.keysym.scancode < sdl::scancodes) fromEvents[e.key.keysym.scancode] = 1;
        if (!e.key.repeat) latch([&](const Binding& b) { return b.usesKey(e.key.keysym.scancode); }, true);
        break;
    case sdl::keyUp:
        if (e.key.keysym.scancode > 0 && e.key.keysym.scancode < sdl::scancodes) fromEvents[e.key.keysym.scancode] = 0;
        latch([&](const Binding& b) { return b.usesKey(e.key.keysym.scancode); }, false);
        break;
    case sdl::mouseDown:
        latch([&](const Binding& b) { return (b.mouse & (1u << (e.button.button - 1))) != 0; }, true);
        break;
    case sdl::mouseUp:
        latch([&](const Binding& b) { return (b.mouse & (1u << (e.button.button - 1))) != 0; }, false);
        break;
    case sdl::mouseWheel:
        if (e.wheel.y > 0) state[index(Act::previous)].latchedDown = true;
        else if (e.wheel.y < 0) state[index(Act::next)].latchedDown = true;
        break;
    case sdl::controllerAdded: openPad(); break;
    case sdl::controllerRemoved:
        if (pad && !SDL_GameControllerGetAttached(pad)) closePad();
        openPad();
        break;
    case sdl::window:
        if (e.window.event == sdl::focusLost) reset();
        break;
    default: break;
    }
}

void Input::reset() {
    for (auto& s : state) s = {};
    suppressed.fill(false);
    synthetic.fill(0);
    fromEvents.fill(0);
    held.clear();
    syntheticMouse = 0;
    sprintLatched = crouchLatched = false;
}

void Input::suppressUntilReleased(Act action) {
    const size_t i=index(action);
    suppressed[i]=true;
    state[i]={};
}

void Input::inject(int scancode, bool isDown) {
    if (scancode >= 0 && scancode < sdl::scancodes) synthetic[scancode] = isDown;
}
void Input::injectButtons(unsigned mask) { syntheticMouse = mask; }

void Input::poll() {
    int count = 0;
    const U8* keyboard = SDL_GetKeyboardState(&count);
    const unsigned mouse = SDL_GetMouseState(nullptr, nullptr) | syntheticMouse;
    const auto keyHeld = [&](int scancode) {
        return (keyboard && scancode < count && keyboard[scancode]) || synthetic[scancode] || fromEvents[scancode];
    };

    // Held keys keep their press order, so the list reads like the sequence the player pressed them in.
    std::erase_if(held, [&](int k) { return !keyHeld(k); });
    for (int k = 1; k < sdl::scancodes; ++k)
        if (keyHeld(k) && std::find(held.begin(), held.end(), k) == held.end()) held.push_back(k);
    peak = std::max(peak, static_cast<int>(held.size()));

    stickX = 0; aim = {}; aimActive = false;
    const bool attached = pad && SDL_GameControllerGetAttached(pad);
    if (attached) {
        stickX = axisValue(SDL_GameControllerGetAxis(pad, sdl::pad::leftX));
        const float ax = axisValue(SDL_GameControllerGetAxis(pad, sdl::pad::rightX));
        const float ay = axisValue(SDL_GameControllerGetAxis(pad, sdl::pad::rightY));
        if (std::hypot(ax, ay) > .15f) { aim = normal({ax, ay}); aimActive = true; }
    } else if (pad) closePad();

    std::array<bool, actCount> now{};
    for (size_t i = 0; i < actCount; ++i) {
        const Binding& b = bindings[i];
        bool value = (mouse & b.mouse) != 0;
        for (int k : b.keys) value = value || (k && keyHeld(k));
        if (attached) {
            if (b.button >= 0 && SDL_GameControllerGetButton(pad, b.button)) value = true;
            if (b.trigger >= 0 && SDL_GameControllerGetAxis(pad, b.trigger) > 12000) value = true;
        }
        now[i] = value;
    }
    if (attached && stickX < -.35f) now[index(Act::left)] = true;
    if (attached && stickX > .35f) now[index(Act::right)] = true;

    // Both directions at once is a common result of quick reversals. The most recent press wins instead of cancelling.
    auto& left = state[index(Act::left)];
    auto& right = state[index(Act::right)];
    const bool leftNew = now[index(Act::left)] && (!left.held || left.latchedDown);
    const bool rightNew = now[index(Act::right)] && (!right.held || right.latchedDown);
    if (leftNew) lastPressed[0] = ++order;
    if (rightNew) lastPressed[1] = ++order;
    if (now[index(Act::left)] && now[index(Act::right)]) {
        if (lastPressed[0] > lastPressed[1]) now[index(Act::right)] = false;
        else now[index(Act::left)] = false;
    }

    for (size_t i = 0; i < actCount; ++i) {
        State& s = state[i];
        // A UI button may still be physically down on the first gameplay frame.
        // Discard its press edge and held state until we observe a full release.
        if (suppressed[i]) {
            if (!now[i]) suppressed[i]=false;
            now[i]=false;
            s.latchedDown=s.latchedUp=false;
        }
        s.pressed = (now[i] && !s.held) || s.latchedDown;
        s.released = (!now[i] && s.held) || (s.latchedUp && !now[i]);
        s.held = now[i];
        s.latchedDown = s.latchedUp = false;
    }
    applyModes();
}

// Sprint and crouch can be held, toggled, or for sprint automatic. Toggles save a finger for players
// whose keyboards cannot report every key combination at once.
void Input::applyModes() {
    auto& sprint = state[index(Act::sprint)];
    auto& crouch = state[index(Act::crouch)];
    const bool moving = horizontal() != 0;
    if (sprintMode == static_cast<int>(SprintMode::toggle)) {
        if (sprint.pressed) sprintLatched = !sprintLatched;
        if (!moving) sprintLatched = false;
        sprint.held = sprintLatched;
    } else if (sprintMode == static_cast<int>(SprintMode::automatic)) {
        // Always sprint while moving. Holding the key walks instead.
        sprint.held = moving && !sprint.held;
    }
    if (crouchToggle) {
        if (crouch.pressed) crouchLatched = !crouchLatched;
        if (state[index(Act::jump)].pressed) crouchLatched = false;
        crouch.held = crouchLatched;
    }
}

float Input::horizontal() const {
    float v = float(down(Act::right)) - float(down(Act::left));
    if (v == 0 && std::abs(stickX) > 0) v = stickX;
    return v;
}
}
