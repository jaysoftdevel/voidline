#pragma once
#include "content/types.hpp"
#include "input/actions.hpp"
#include "input/binding.hpp"
#include "platform/sdl.hpp"
#include <array>
#include <vector>

namespace vl {
// Polled keyboard, mouse and gamepad state.
//
// A key counts as held when either SDL's keyboard snapshot or the event stream says so, so any number of
// keys can be held together. Press and release edges are latched from the event stream, which means a tap
// that starts and ends inside one frame is never lost. Opposite directions resolve to the one pressed last.
// The only limit that remains is electrical ghosting inside some keyboards, which no program can see. The
// held-key list below makes it visible: a key that never shows up there never reached the computer.
class Input {
public:
    Input();
    void event(const SDL_Event& event);
    void poll();
    void reset();
    // Ignore an action inherited from a menu until its physical button/key is released.
    // This stops the Start Campaign / Skip Scene click from firing the first shot.
    void suppressUntilReleased(Act action);
    // Holds a scancode in software. Used for scripted scenes and tests, and combined with real input.
    void inject(int scancode, bool down);
    void injectButtons(unsigned mouseMask);

    // Applies the saved key map and the sprint and crouch modes.
    void configure(const Settings& settings);
    // Gives a key to an action slot, removing it from any other action. Returns false for reserved keys.
    bool assign(Act act, size_t slot, int scancode);
    void clear(Act act, size_t slot);
    void resetBindings();
    // Key map in the form stored in Settings: 0 unassigned, -1 default.
    std::array<int,keymapSize> keymap() const;

    bool down(Act a) const { return state[index(a)].held; }
    bool pressed(Act a) const { return state[index(a)].pressed; }
    bool released(Act a) const { return state[index(a)].released; }
    // Left minus right in the range -1 to 1, including analog gamepad movement.
    float horizontal() const;
    Vec aimStick() const { return aim; }
    bool aimStickActive() const { return aimActive; }
    bool gamepadConnected() const { return pad != nullptr; }
    // Scancodes held at this moment, in the order they were pressed.
    const std::vector<int>& heldKeys() const { return held; }
    // Most keys that were held together since the last reset of the counter.
    int peakKeys() const { return peak; }
    void resetPeak() { peak = 0; }
    const Binding& binding(Act a) const { return bindings[index(a)]; }

private:
    struct State { bool held = false, pressed = false, released = false, latchedDown = false, latchedUp = false; };
    static size_t index(Act a) { return static_cast<size_t>(a); }
    std::array<Binding, actCount> bindings{};
    std::array<State, actCount> state{};
    std::array<bool, actCount> suppressed{};
    std::array<U8, sdl::scancodes> synthetic{}, fromEvents{};
    std::vector<int> held;
    unsigned syntheticMouse = 0;
    SDL_GameController* pad = nullptr;
    float stickX = 0;
    Vec aim{};
    bool aimActive = false;
    int peak = 0, sprintMode = 0;
    bool crouchToggle = false, sprintLatched = false, crouchLatched = false;
    unsigned long long order = 0;
    std::array<unsigned long long, 2> lastPressed{};
    void openPad();
    void closePad();
    void applyModes();
};
}
