#pragma once
#include "data.hpp"
#include "platform.hpp"
#include <array>
#include <vector>

namespace vl {
// Every gameplay command the player can issue. Each one may be bound to several keys,
// a mouse button and a gamepad control, and all of them are evaluated every frame.
enum class Act {
    left, right, jump, fire, assist, reload, sprint, crouch, slow,
    next, previous, weapon1, weapon2, weapon3, weapon4, pause, count
};
constexpr size_t actCount = static_cast<size_t>(Act::count);

struct Binding {
    std::array<int,5> keys{};   // Scancodes. Zero means unused.
    unsigned mouse = 0;         // Mouse button mask.
    int button = -1;            // Gamepad button.
    int trigger = -1;           // Gamepad analog trigger axis.
};

// Polled keyboard, mouse and gamepad state.
//
// The keyboard snapshot comes from SDL_GetKeyboardState, which tracks every key at once, so any
// number of chords can be held together. Press and release edges are latched from the event stream
// so a tap that starts and ends within one frame is never lost. The only unavoidable limit is
// electrical ghosting inside some keyboards. For that, every action has alternate keys so a
// player can choose a combination their hardware reports correctly.
class Input {
public:
    Input();
    void event(const SDL_Event& event);
    void poll();
    void reset();
    // Holds a scancode in software. Used for scripted scenes and tests, and combined with real input.
    void inject(int scancode, bool down);
    void injectButtons(unsigned mouseMask);

    bool down(Act a) const { return state[index(a)].held; }
    bool pressed(Act a) const { return state[index(a)].pressed; }
    bool released(Act a) const { return state[index(a)].released; }
    // Left minus right in the range -1 to 1, including analog gamepad movement.
    float horizontal() const;
    Vec aimStick() const { return aim; }
    bool aimStickActive() const { return aimActive; }
    bool gamepadConnected() const { return pad != nullptr; }
    int heldKeys() const { return keysHeld; }
    const Binding& binding(Act a) const { return bindings[index(a)]; }

private:
    struct State { bool held = false, pressed = false, released = false, latchedDown = false, latchedUp = false; };
    static size_t index(Act a) { return static_cast<size_t>(a); }
    std::array<Binding, actCount> bindings{};
    std::array<State, actCount> state{};
    std::array<U8, sdl::scancodes> synthetic{};
    unsigned syntheticMouse = 0;
    SDL_GameController* pad = nullptr;
    float stickX = 0;
    Vec aim{};
    bool aimActive = false;
    int keysHeld = 0;
    bool usesKey(const Binding& b, int scancode) const;
    void openPad();
    void closePad();
};
}
