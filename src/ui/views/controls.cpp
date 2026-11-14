#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
namespace {
const char* sprintModeName(int mode) {
    return mode==static_cast<int>(SprintMode::toggle)?"TOGGLE":mode==static_cast<int>(SprintMode::automatic)?"AUTOMATIC":"HOLD";
}
}

// Key capture for the controls screen. Escape cancels, Backspace clears the slot.
void App::captureKey(int scancode) {
    const Act act=static_cast<Act>(rebinding/static_cast<int>(keysPerAction));
    const size_t slotIndex=static_cast<size_t>(rebinding)%keysPerAction;
    if (scancode==sdl::key::esc) { rebinding=-1; return; }
    if (scancode==sdl::key::backspace) input.clear(act,slotIndex);
    else if (!input.assign(act,slotIndex,scancode)) { inform("That key is reserved for menus."); return; }
    settings.keymap=input.keymap();
    rebinding=-1;
    commitSettings();
}

void App::keyBindings() {
    shell("orbital");
    heading("SYSTEM","Controls","Select a slot, then press the key. Backspace clears it. Esc cancels.");
    button({640,86,224,36},std::string("SPRINT  ")+sprintModeName(settings.sprintMode),[this]{
        settings.sprintMode=(settings.sprintMode+1)%3; commitSettings();
    });
    button({876,86,269,36},std::string("CROUCH  ")+(settings.crouchToggle?"TOGGLE":"HOLD"),[this]{
        settings.crouchToggle=!settings.crouchToggle; commitSettings();
    });
    // Mouse wheel and the left and right mouse buttons are fixed, so only the keyboard rows are listed.
    constexpr Act rows[]{Act::left,Act::right,Act::jump,Act::crouch,Act::sprint,Act::fire,Act::assist,Act::slow,Act::reload,
                         Act::next,Act::weapon1,Act::weapon2,Act::weapon3,Act::weapon4,Act::weapon5,Act::weapon6,Act::weapon7,Act::pause,Act::interact};
    constexpr int perColumn=10;
    for (int i=0;i<static_cast<int>(std::size(rows));++i) {
        const Act act=rows[i];
        const float x=55.f+(i/perColumn)*560, y=172.f+(i%perColumn)*35;
        r.rect({x,y,530,34},hex(0x152531)); r.outline({x,y,530,34},hex(0x344858));
        r.text({x+12,y+23},actionInfo(act).label,15,white,true);
        for (size_t s=0;s<keysPerAction;++s) {
            const int id=static_cast<int>(static_cast<size_t>(act)*keysPerAction+s);
            const bool capturing=rebinding==id;
            const int code=input.binding(act).keys[s];
            const std::string text=capturing?"press a key":code>0?keyName(code):"-";
            button({x+195+s*113.f,y+3,106,28},text,[this,id]{ rebinding=id; },true,capturing);
        }
    }
    r.text({55,547},"Mouse: left fire, right bullet time, wheel switches weapons.",12.5f,muted);
    r.text({55,563},"Gamepad buttons are fixed.",12.5f,muted);
    button({860,526,129,41},"DEFAULTS",[this]{
        settings.keymap.fill(-1); settings.sprintMode=0; settings.crouchToggle=false; rebinding=-1; commitSettings();
        inform("Controls reset to their defaults.");
    });
    button({1002,526,143,41},"BACK",[this]{ rebinding=-1; show(View::settings); },true,true);
    // Live view of what the computer receives. If a key never shows up here, the keyboard did not report it.
    r.gradient({400,524,440,44},hex(0x0c1723).alpha(.95f),hex(0x101c26).alpha(.9f));
    r.outline({400,524,440,44},hex(0x526978));
    std::string held;
    for (int code:input.heldKeys()) held+=(held.empty()?"":"  +  ")+keyName(code);
    label({412,541},"KEYS HELD  "+std::to_string(input.heldKeys().size())+"   PEAK "+std::to_string(input.peakKeys()),teal);
    r.text({412,560},held.empty()?"hold several keys to test your keyboard":held,13,white,true);
}
} // namespace vl
