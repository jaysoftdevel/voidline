#include "input/binding.hpp"
#include "platform/sdl.hpp"
#include <algorithm>
#include <cctype>

namespace vl {
namespace {
constexpr ActionInfo actions[] = {
    {"Move left", true}, {"Move right", true}, {"Jump / flip", true}, {"Fire", true}, {"Auto-aim fire", true},
    {"Reload", true}, {"Sprint", true}, {"Crouch", true}, {"Bullet time", true}, {"Next weapon", true},
    {"Previous weapon", true}, {"Weapon 1", true}, {"Weapon 2", true}, {"Weapon 3", true}, {"Weapon 4", true},
    {"Weapon 5", true}, {"Weapon 6", true}, {"Weapon 7", true}, {"Pause", true}
};
static_assert(sizeof(actions) / sizeof(actions[0]) == actCount);
}

const ActionInfo& actionInfo(Act act) { return actions[static_cast<size_t>(act)]; }

bool Binding::usesKey(int scancode) const {
    return scancode > 0 && std::find(keys.begin(), keys.end(), scancode) != keys.end();
}

Binding defaultBinding(Act act) {
    using namespace sdl;
    Binding b;
    auto keys = [&](std::initializer_list<int> list) {
        size_t i = 0;
        for (int k : list) if (i < b.keys.size()) b.keys[i++] = k;
    };
    switch (act) {
    case Act::left: keys({key::a, key::left}); b.button = pad::left; break;
    case Act::right: keys({key::d, key::right}); b.button = pad::right; break;
    case Act::jump: keys({key::space, key::w, key::up}); b.button = pad::a; break;
    case Act::fire: b.mouse = buttonLeft; b.button = pad::rightShoulder; b.trigger = pad::rightTrigger; break;
    case Act::assist: keys({key::f, key::j}); break;
    case Act::reload: keys({key::r}); b.button = pad::x; break;
    case Act::sprint: keys({key::shiftLeft, key::shiftRight}); b.button = pad::stick; break;
    case Act::crouch: keys({key::c, key::s, key::down}); b.button = pad::b; break;
    case Act::slow: keys({key::q, key::x}); b.mouse = buttonRight; b.button = pad::leftShoulder; b.trigger = pad::leftTrigger; break;
    case Act::next: keys({key::e}); b.button = pad::y; break;
    case Act::weapon1: keys({key::one}); break;
    case Act::weapon2: keys({key::two}); break;
    case Act::weapon3: keys({key::three}); break;
    case Act::weapon4: keys({key::four}); break;
    case Act::weapon5: keys({key::five}); break;
    case Act::weapon6: keys({key::six}); break;
    case Act::weapon7: keys({key::seven}); break;
    case Act::pause: keys({key::p}); b.button = pad::start; break;
    case Act::previous: case Act::count: break;
    }
    return b;
}

bool reservedKey(int scancode) {
    return scancode <= 0 || scancode >= sdl::scancodes || scancode == sdl::key::esc || scancode == sdl::key::f11 ||
           scancode == sdl::key::f3 || scancode == sdl::key::tab || scancode == sdl::key::enter;
}

std::string keyName(int scancode) {
    if (scancode <= 0) return "-";
    const char* name = SDL_GetScancodeName(scancode);
    if (!name || !*name) return "Key " + std::to_string(scancode);
    std::string text = name;
    if (text.size() == 1) text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
    return text;
}
}
