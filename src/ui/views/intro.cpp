#include "ui/app.hpp"
#include "ui/common.hpp"

using namespace vl::ui;

namespace vl {
void App::cinematic() {
    drawOpening(false);
    button({1010,528,132,35},"SKIP PROLOGUE",[this]{launch(0);});
}
}
