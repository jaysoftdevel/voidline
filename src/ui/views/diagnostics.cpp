#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
// Overlay toggled with F3. It lists the keys the program currently receives, in the order they were pressed.
void App::diagnostics() {
    const auto& keys=input.heldKeys();
    std::string held;
    for (int code:keys) held+=(held.empty()?"":"  +  ")+keyName(code);
    if (held.empty()) held="none";
    const float w=std::max(300.f,std::min(760.f,r.measure(held,13,true)+34));
    const Rect box{19,118,w,66};
    r.gradient(box,hex(0x0c1723).alpha(.94f),hex(0x101c26).alpha(.9f));
    r.outline(box,hex(0x526978).alpha(.8f));
    label({box.x+12,box.y+20},"INPUT  /  KEYS HELD "+std::to_string(keys.size())+"   PEAK "+std::to_string(input.peakKeys())+
          (input.gamepadConnected()?"   GAMEPAD":""),teal);
    r.text({box.x+12,box.y+40},held,13,white,true);
    std::string actions;
    for (size_t i=0;i<actCount;++i) {
        const Act act=static_cast<Act>(i);
        if (input.down(act)) actions+=(actions.empty()?"":", ")+std::string(actionInfo(act).label);
    }
    r.text({box.x+12,box.y+57},actions.empty()?"no actions":actions,11.5f,pale);
}
} // namespace vl
