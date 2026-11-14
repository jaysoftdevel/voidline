#pragma once
#include <array>
#include <cstddef>

namespace vl {
// Every gameplay command the player can issue. Each one can be bound to several keys,
// a mouse button and a gamepad control, and all of them are evaluated every frame.
enum class Act {
    left, right, jump, fire, assist, reload, sprint, crouch, slow,
    next, previous, weapon1, weapon2, weapon3, weapon4, weapon5, weapon6, weapon7, pause, count
};
constexpr size_t actCount = static_cast<size_t>(Act::count);

struct ActionInfo {
    const char* label;
    bool rebindable;
};
const ActionInfo& actionInfo(Act act);
}
