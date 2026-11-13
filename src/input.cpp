#include "input.hpp"
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
    using namespace sdl;
    auto bind = [&](Act a, std::initializer_list<int> keys, unsigned mouse = 0, int button = -1, int trigger = -1) {
        Binding b;
        size_t i = 0;
        for (int k : keys) b.keys[i++] = k;
        b.mouse = mouse; b.button = button; b.trigger = trigger;
        bindings[index(a)] = b;
    };
    bind(Act::left, {key::a, key::left}, 0, pad::left);
    bind(Act::right, {key::d, key::right}, 0, pad::right);
    bind(Act::jump, {key::space, key::w, key::up, key::k}, 0, pad::a);
    bind(Act::fire, {}, buttonLeft, pad::rightShoulder, pad::rightTrigger);
    bind(Act::assist, {key::f, key::j});
    bind(Act::reload, {key::r}, 0, pad::x);
    bind(Act::sprint, {key::shiftLeft, key::shiftRight}, 0, pad::stick);
    bind(Act::crouch, {key::c, key::s, key::down, key::controlLeft}, 0, pad::b);
    bind(Act::slow, {key::q, key::x}, buttonRight, pad::leftShoulder, pad::leftTrigger);
    bind(Act::next, {key::e}, 0, pad::y);
    bind(Act::previous, {}, 0, -1);
    bind(Act::weapon1, {key::one});
    bind(Act::weapon2, {key::two});
    bind(Act::weapon3, {key::three});
    bind(Act::weapon4, {key::four});
    bind(Act::pause, {key::esc, key::p}, 0, pad::start);
    openPad();
}

bool Input::usesKey(const Binding& b, int scancode) const {
    for (int k : b.keys) if (k && k == scancode) return true;
    return false;
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
    auto latch = [&](auto predicate, bool down) {
        for (size_t i = 0; i < actCount; ++i) {
            if (!predicate(bindings[i])) continue;
            (down ? state[i].latchedDown : state[i].latchedUp) = true;
        }
    };
    switch (e.type) {
    case sdl::keyDown:
        if (!e.key.repeat) latch([&](const Binding& b) { return usesKey(b, e.key.keysym.scancode); }, true);
        break;
    case sdl::keyUp:
        latch([&](const Binding& b) { return usesKey(b, e.key.keysym.scancode); }, false);
        break;
    case sdl::mouseDown:
        latch([&](const Binding& b) { return b.mouse & (1u << (e.button.button - 1)); }, true);
        break;
    case sdl::mouseUp:
        latch([&](const Binding& b) { return b.mouse & (1u << (e.button.button - 1)); }, false);
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
    synthetic.fill(0);
    syntheticMouse = 0;
}

void Input::inject(int scancode, bool down) {
    if (scancode >= 0 && scancode < sdl::scancodes) synthetic[scancode] = down;
}
void Input::injectButtons(unsigned mask) { syntheticMouse = mask; }

void Input::poll() {
    int count = 0;
    const U8* keyboard = SDL_GetKeyboardState(&count);
    const unsigned mouse = SDL_GetMouseState(nullptr, nullptr) | syntheticMouse;
    const auto keyHeld = [&](int scancode) {
        return (keyboard && scancode < count && keyboard[scancode]) || synthetic[scancode];
    };
    keysHeld = 0;
    for (int k = 0; k < std::min(count, sdl::scancodes); ++k) keysHeld += keyHeld(k) ? 1 : 0;

    stickX = 0; aim = {}; aimActive = false;
    const bool attached = pad && SDL_GameControllerGetAttached(pad);
    if (attached) {
        stickX = axisValue(SDL_GameControllerGetAxis(pad, sdl::pad::leftX));
        const float ax = axisValue(SDL_GameControllerGetAxis(pad, sdl::pad::rightX));
        const float ay = axisValue(SDL_GameControllerGetAxis(pad, sdl::pad::rightY));
        if (std::hypot(ax, ay) > .15f) { aim = normal({ax, ay}); aimActive = true; }
    } else if (pad) closePad();

    for (size_t i = 0; i < actCount; ++i) {
        const Binding& b = bindings[i];
        bool now = (mouse & b.mouse) != 0;
        for (int k : b.keys) now = now || (k && keyHeld(k));
        if (attached) {
            if (b.button >= 0 && SDL_GameControllerGetButton(pad, b.button)) now = true;
            if (b.trigger >= 0 && SDL_GameControllerGetAxis(pad, b.trigger) > 12000) now = true;
        }
        const Act a = static_cast<Act>(i);
        if (attached && a == Act::left && stickX < -.35f) now = true;
        if (attached && a == Act::right && stickX > .35f) now = true;
        State& s = state[i];
        s.pressed = (now && !s.held) || s.latchedDown;
        s.released = (!now && s.held) || (s.latchedUp && !now);
        s.held = now;
        s.latchedDown = s.latchedUp = false;
    }
}

float Input::horizontal() const {
    float v = float(down(Act::right)) - float(down(Act::left));
    if (v == 0 && std::abs(stickX) > 0) v = stickX;
    return v;
}
}
