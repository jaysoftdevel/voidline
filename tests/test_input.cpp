// Keyboard chords, remapping and control modes.
#include "support.hpp"

using namespace test;
using namespace vl;

namespace {
void inputChords() {
    using namespace vl;
    Input input;
    // Eight simultaneous keys from different actions, plus a mouse button.
    for (int k : {sdl::key::d, sdl::key::space, sdl::key::shiftLeft, sdl::key::f, sdl::key::q, sdl::key::c, sdl::key::r, sdl::key::two})
        input.inject(k, true);
    input.injectButtons(sdl::buttonLeft);
    input.poll();
    CHECK(input.down(Act::right));
    CHECK(input.down(Act::jump));
    CHECK(input.down(Act::sprint));
    CHECK(input.down(Act::assist));
    CHECK(input.down(Act::slow));
    CHECK(input.down(Act::crouch));
    CHECK(input.down(Act::reload));
    CHECK(input.down(Act::weapon2));
    CHECK(input.down(Act::fire));
    CHECK(!input.down(Act::left));
    CHECK(input.heldKeys().size() >= 8);
    CHECK(input.pressed(Act::jump));
    input.poll();
    CHECK(!input.pressed(Act::jump) && input.down(Act::jump));

    // Releasing one key must not disturb the others.
    input.inject(sdl::key::space, false);
    input.poll();
    CHECK(!input.down(Act::jump) && input.released(Act::jump));
    CHECK(input.down(Act::right) && input.down(Act::sprint) && input.down(Act::slow));

    // Alternate bindings.
    input.reset();
    input.inject(sdl::key::left, true);
    input.inject(sdl::key::up, true);
    input.inject(sdl::key::s, true);
    input.poll();
    CHECK(input.down(Act::left) && input.down(Act::jump) && input.down(Act::crouch));
    CHECK(input.horizontal() < 0);

    // A tap shorter than one frame is still reported as a press.
    input.reset();
    input.event(keyEvent(sdl::keyDown, sdl::key::space));
    input.event(keyEvent(sdl::keyUp, sdl::key::space));
    input.poll();
    CHECK(input.pressed(Act::jump) && !input.down(Act::jump));
    input.poll();
    CHECK(!input.pressed(Act::jump));
}
}

void directionsAndOrder() {
    Input input;
    // Holding both directions follows the one pressed last, in either order.
    input.inject(sdl::key::a, true);
    input.poll();
    input.inject(sdl::key::d, true);
    input.poll();
    CHECK(input.down(Act::right) && !input.down(Act::left) && input.horizontal() > 0);
    input.inject(sdl::key::a, false);
    input.inject(sdl::key::a, true);
    input.poll();
    input.poll();
    CHECK(input.down(Act::right));
    input.reset();
    input.inject(sdl::key::d, true);
    input.poll();
    input.inject(sdl::key::left, true);
    input.poll();
    CHECK(input.down(Act::left) && !input.down(Act::right));
    // The list of held keys keeps press order and counts every key.
    input.reset();
    for (int k : {sdl::key::w, sdl::key::d, sdl::key::shiftLeft, sdl::key::q, sdl::key::three}) { input.inject(k, true); input.poll(); }
    CHECK(input.heldKeys().size() == 5);
    CHECK(input.heldKeys().front() == sdl::key::w && input.heldKeys().back() == sdl::key::three);
    CHECK(input.peakKeys() == 5);
}

void eventStreamAlone() {
    Input input;
    // Keys that arrive only as events (nothing in the keyboard snapshot) still count as held.
    for (int k : {sdl::key::d, sdl::key::space, sdl::key::shiftLeft, sdl::key::f}) input.event(keyEvent(sdl::keyDown, k));
    input.poll();
    CHECK(input.down(Act::right) && input.down(Act::jump) && input.down(Act::sprint) && input.down(Act::assist));
    CHECK(input.pressed(Act::jump));
    input.event(keyEvent(sdl::keyUp, sdl::key::space));
    input.poll();
    CHECK(!input.down(Act::jump) && input.down(Act::right) && input.down(Act::sprint) && input.down(Act::assist));
}

void remapping() {
    Input input;
    // Assigning a key to another action takes it away from the first one.
    CHECK(input.assign(Act::reload, 1, sdl::key::z));
    CHECK(input.binding(Act::reload).keys[1] == sdl::key::z);
    CHECK(input.assign(Act::crouch, 0, sdl::key::z));
    CHECK(input.binding(Act::reload).keys[1] == 0 && input.binding(Act::crouch).keys[0] == sdl::key::z);
    // Reserved keys are refused.
    CHECK(!input.assign(Act::jump, 0, sdl::key::esc) && !input.assign(Act::jump, 0, sdl::key::f11));
    // A remapped action no longer answers to its old key.
    CHECK(input.assign(Act::jump, 0, sdl::key::v));
    input.inject(sdl::key::space, true);
    input.poll();
    CHECK(!input.down(Act::jump));
    input.reset();
    input.inject(sdl::key::w, true);
    input.poll();
    CHECK(input.down(Act::jump));
    input.reset();
    input.inject(sdl::key::v, true);
    input.poll();
    CHECK(input.down(Act::jump));
    // The saved key map restores the same bindings on a fresh input object.
    Settings settings;
    settings.keymap = input.keymap();
    Input fresh;
    fresh.configure(settings);
    CHECK(fresh.binding(Act::jump).keys == input.binding(Act::jump).keys);
    CHECK(fresh.binding(Act::crouch).keys == input.binding(Act::crouch).keys);
    CHECK(fresh.binding(Act::left).keys == defaultBinding(Act::left).keys);
    // An untouched key map leaves the defaults in place.
    Settings plain;
    Input defaults;
    defaults.configure(plain);
    CHECK(defaults.binding(Act::sprint).keys == defaultBinding(Act::sprint).keys);
    CHECK(keyName(sdl::key::space) != "-");
}

void controlModes() {
    Settings settings;
    settings.sprintMode = static_cast<int>(SprintMode::toggle);
    settings.crouchToggle = true;
    Input input;
    input.configure(settings);
    // Toggle sprint: one tap engages it while moving, and stopping releases it.
    input.inject(sdl::key::d, true);
    input.inject(sdl::key::shiftLeft, true);
    input.poll();
    input.inject(sdl::key::shiftLeft, false);
    input.poll();
    CHECK(input.down(Act::sprint));
    input.inject(sdl::key::d, false);
    input.poll();
    CHECK(!input.down(Act::sprint));
    // Toggle crouch holds until tapped again or until the player jumps.
    input.inject(sdl::key::c, true);
    input.poll();
    input.inject(sdl::key::c, false);
    input.poll();
    CHECK(input.down(Act::crouch));
    input.inject(sdl::key::space, true);
    input.poll();
    CHECK(!input.down(Act::crouch));
    // Automatic sprint runs whenever the player moves and walks while the key is held.
    settings.sprintMode = static_cast<int>(SprintMode::automatic);
    settings.crouchToggle = false;
    input.configure(settings);
    input.reset();
    input.inject(sdl::key::d, true);
    input.poll();
    CHECK(input.down(Act::sprint));
    input.inject(sdl::key::shiftLeft, true);
    input.poll();
    CHECK(!input.down(Act::sprint));
}

void runInputTests() {
    inputChords();
    directionsAndOrder();
    eventStreamAlone();
    remapping();
    controlModes();
}
