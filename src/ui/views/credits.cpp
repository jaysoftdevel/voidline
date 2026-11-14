#include "ui/app.hpp"
#include "ui/common.hpp"
#include "core/version.hpp"

using namespace vl::ui;

namespace vl {
void App::credits() {
    shell("orbital"); heading("VEGA-9","Credits");
    r.gradient({55,172,718,323},hex(0x1b2e39),hex(0x101e29));
    r.outline({55,172,718,323},hex(0x485f6b));
    r.text({74,215},"VOIDLINE  /  Native edition",31,white,true);
    copy({76,251},"Original VOIDLINE campaign, sector art, opening sequence and metal soundtrack.",620,18);
    copy({76,311},"Custom OpenGL renderer with techniques studied in LearnOpenGL, three.js and Godot.",620,17);
    copy({76,370},"Linux platform services by SDL2. Media decoding by libpng, libwebp and libsndfile. Roboto typography under the included license.",620,17);
    label({76,457},std::string("VERSION ")+version,teal);
    button({55,526,142,41},"MAIN MENU",[this]{show(View::menu);},true,true);
}
} // namespace vl
