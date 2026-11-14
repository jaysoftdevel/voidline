#include "engine/engine.hpp"
#include "core/version.hpp"
#include "platform/paths.hpp"
#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace fs=std::filesystem;

namespace vl {
namespace {
fs::path absolutePath(const std::string& path) { return fs::absolute(pathFromUtf8(path)); }
}
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
        else if (arg=="--renderer") {
            const auto mode=value();
            if (mode=="auto") result.graphics=GraphicsMode::automatic;
            else if (mode=="opengl") result.graphics=GraphicsMode::opengl;
#ifdef _WIN32
            else if (mode=="dx12") result.graphics=GraphicsMode::dx12;
            else if (mode=="warp") result.graphics=GraphicsMode::warp;
#endif
            else throw std::runtime_error("Unsupported graphics mode: "+mode);
        }
#ifdef _WIN32
        else if (arg=="--mesa-path") result.mesa=absolutePath(value());
        else if (arg=="--adapter") result.adapter=value();
        else if (arg=="--dx-report") result.report=true;
#endif
        else if (arg=="--frames") result.frames=std::stoi(value());
        else if (arg=="--scene") result.scene=value();
        else if (arg=="--screenshot") result.screenshot=absolutePath(value());
        else if (arg=="--follow") result.follow=true;
        else if (arg=="--save-file") result.save=absolutePath(value());
        else if (arg=="--width") result.width=std::clamp(std::stoi(value()),720,3840);
        else if (arg=="--help") {
            std::cout<<"VOIDLINE "<<version<<"\nUsage: voidline [--windowed] [--save-file PATH]\n"
#ifdef _WIN32
                "Graphics: --renderer auto|dx12|warp|opengl [--mesa-path PATH] [--adapter NAME]\n"
                "Hardware report: --dx-report\n"
#else
                "Graphics: --renderer auto|opengl\n"
#endif
                "Move: A/D or arrows. Jump (twice to flip): Space, W, Up or K. Sprint: Shift. Crouch: C, S, Down or Ctrl.\n"
                "Fire: mouse, F or J. Bullet time: Q, X or right mouse. Reload: R. Weapons: 1-7, E or mouse wheel.\n"
                "Pause: Esc or P. Fullscreen: F11. A gamepad is supported when connected.\n"
                "Diagnostics: --smoke-test [--frames N] [--width PIXELS] [--scene 1..20|map|menu|create|armory|intro|cinematic-arrival|cinematic-close|cinematic-detail|cinematic-recover|stand[0-2]|gallery[0-2]|walk|flip|slow] [--screenshot PATH.ppm]\n";
            std::exit(0);
        } else if (arg=="--version") {
#ifdef _WIN32
            std::cout<<"VOIDLINE "<<version<<" (Windows)\n";
#else
            std::cout<<"VOIDLINE "<<version<<" (native Linux)\n";
#endif
            std::exit(0);
        } else throw std::runtime_error("Unknown option "+arg+" (see --help).");
    }
    if (result.frames<0 || (result.smoke && result.frames==0)) throw std::runtime_error("Frame count must be positive.");
    const bool named=result.scene=="map" || result.scene=="menu" || result.scene=="create" || result.scene.rfind("armory",0)==0 || result.scene=="intro" || result.scene.rfind("cinematic-",0)==0 ||
        result.scene=="flip" || result.scene=="slow" || result.scene.rfind("stand",0)==0 || result.scene.rfind("gallery",0)==0 || result.scene=="controls" || result.scene.rfind("hit",0)==0 || result.scene.rfind("arms",0)==0 || result.scene.rfind("event",0)==0 || result.scene=="walk" || result.scene=="boss";
    if (result.smoke && !named) {
        int sector=0;
        const char* end=result.scene.data()+result.scene.size();
        const auto parsed=std::from_chars(result.scene.data(),end,sector);
        if (parsed.ec!=std::errc{} || parsed.ptr!=end || sector<1 || sector>sectorCount)
            throw std::runtime_error("Unknown smoke-test scene.");
    }
    return result;
}
} // namespace vl
