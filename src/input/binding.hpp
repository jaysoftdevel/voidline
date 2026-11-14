#pragma once
#include "content/types.hpp"
#include "input/actions.hpp"
#include <array>
#include <string>

namespace vl {
struct Binding {
    std::array<int,keysPerAction> keys{}; // Scancodes. Zero means unused.
    unsigned mouse = 0;                   // Mouse button mask.
    int button = -1;                      // Gamepad button.
    int trigger = -1;                     // Gamepad analog trigger axis.
    bool usesKey(int scancode) const;
};

// The factory bindings, before any remapping.
Binding defaultBinding(Act act);
// Scancodes that can never be assigned, because they drive the window or the menus.
bool reservedKey(int scancode);
// Short label for a scancode, for example "Space" or "Left Shift".
std::string keyName(int scancode);
}
