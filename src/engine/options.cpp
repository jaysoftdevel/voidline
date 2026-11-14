#include "engine/engine.hpp"
#include "core/version.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace fs=std::filesystem;

namespace vl {
Options parseOptions(int argc,char** argv) {
    Options result;
    for (int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        auto value=[&]() {
            if (i+1==argc) throw std::runtime_error("Missing value for "+arg);
            return std::string(argv[++i]);
        };
        if (arg=="--windowed") result.windowed=true;
        else if (arg=="--smoke-test") { result.smoke=true; result.frames=90; }
        else if (arg=="--frames") result.frames=std::stoi(value());
        else if (arg=="--scene") result.scene=value();
        else if (arg=="--screenshot") result.screenshot=value();
        else if (arg=="--follow") result.follow=true;
        else if (arg=="--save-file") result.save=fs::absolute(value());
        else if (arg=="--width") result.width=std::clamp(std::stoi(value()),720,3840);
        else if (arg=="--help") {
            std::cout<<"VOIDLINE "<<version<<"\nUsage: voidline [--windowed] [--save-file PATH]\n"
                "Move: A/D or arrows. Jump (twice to flip): Space, W, Up or K. Sprint: Shift. Crouch: C, S, Down or Ctrl.\n"
                "Fire: mouse, F or J. Bullet time: Q, X or right mouse. Reload: R. Weapons: 1-4, E or mouse wheel.\n"
                "Pause: Esc or P. Fullscreen: F11. A gamepad is supported when connected.\n"
                "Diagnostics: --smoke-test [--frames N] [--width PIXELS] [--scene 1..5|menu|create|armory|intro|stand[0-2]|gallery[0-2]|walk|flip|slow] [--screenshot PATH.ppm]\n";
            std::exit(0);
        } else if (arg=="--version") {
            std::cout<<"VOIDLINE "<<version<<" (native Linux)\n"; std::exit(0);
        } else throw std::runtime_error("Unknown option "+arg+" (see --help).");
    }
    if (result.frames<0 || (result.smoke && result.frames==0)) throw std::runtime_error("Frame count must be positive.");
    const bool named=result.scene=="menu" || result.scene=="create" || result.scene.rfind("armory",0)==0 || result.scene=="intro" ||
        result.scene=="flip" || result.scene=="slow" || result.scene.rfind("stand",0)==0 || result.scene.rfind("gallery",0)==0 || result.scene=="controls" || result.scene.rfind("hit",0)==0 || result.scene.rfind("arms",0)==0 || result.scene.rfind("event",0)==0 || result.scene=="walk" || result.scene=="boss";
    if (result.smoke && !named && !(result.scene.size()==1 && result.scene[0]>='1' && result.scene[0]<='5'))
        throw std::runtime_error("Unknown smoke-test scene.");
    return result;
}
} // namespace vl
