#include "engine.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace fs=std::filesystem;
namespace vl {
namespace {
constexpr const char* version="2.0.0";
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
        else if (arg=="--frames") result.frames=std::stoi(value());
        else if (arg=="--scene") result.scene=value();
        else if (arg=="--screenshot") result.screenshot=value();
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
    const bool named=result.scene=="menu" || result.scene=="create" || result.scene=="armory" || result.scene=="intro" ||
        result.scene=="flip" || result.scene=="slow" || result.scene.rfind("stand",0)==0 || result.scene.rfind("gallery",0)==0 || result.scene=="walk" || result.scene=="boss";
    if (result.smoke && !named && !(result.scene.size()==1 && result.scene[0]>='1' && result.scene[0]<='5'))
        throw std::runtime_error("Unknown smoke-test scene.");
    return result;
}

struct Engine::Platform {
    SDL_Window* window=nullptr;
    void* context=nullptr;
    Platform(bool smoke,int width) {
        if (SDL_Init(sdl::video)<0) throw std::runtime_error(SDL_GetError());
        if (SDL_Init(sdl::audio)<0) std::cerr<<"SDL audio initialization: "<<SDL_GetError()<<'\n';
        if (SDL_Init(sdl::gameController)<0) std::cerr<<"Gamepad support unavailable: "<<SDL_GetError()<<'\n';
        SDL_GL_SetAttribute(5,1);  // Double buffering.
        SDL_GL_SetAttribute(17,3); // Context major version.
        SDL_GL_SetAttribute(18,3); // Context minor version.
        SDL_GL_SetAttribute(21,1); // Core profile.
        const U32 flags=sdl::openGL|sdl::resizable|sdl::highDPI|(smoke?sdl::hidden:0);
        window=SDL_CreateWindow("VOIDLINE",sdl::centered,sdl::centered,width,width/2,flags);
        if (!window) { const std::string error=SDL_GetError(); SDL_Quit(); throw std::runtime_error(error); }
        SDL_SetWindowMinimumSize(window,720,360);
        context=SDL_GL_CreateContext(window);
        if (!context) {
            const std::string error=std::string("OpenGL 3.3 core context unavailable: ")+SDL_GetError();
            SDL_DestroyWindow(window); SDL_Quit();
            throw std::runtime_error(error);
        }
    }
    ~Platform() {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

Engine::Engine(const Options& options):config(options) {
    const auto root=fs::read_symlink("/proc/self/exe").parent_path();
    platform=std::make_unique<Platform>(config.smoke,config.width);
    if (!config.windowed && SDL_SetWindowFullscreen(platform->window,sdl::fullscreen)<0)
        std::cerr<<"Fullscreen unavailable: "<<SDL_GetError()<<'\n';
    vsync=SDL_GL_SetSwapInterval(config.smoke?0:1)==0 && !config.smoke;
    const auto media=root/"assets";
    renderer=std::make_unique<Renderer>(platform->window,media);
    audio=std::make_unique<Audio>(media);
    const auto path=config.save.empty()?(config.smoke?root/".smoke-campaign.json":storagePath()):config.save;
    storage=std::make_unique<Storage>(path);
    app=std::make_unique<App>(platform->window,*renderer,*audio,*storage);
    if (config.smoke) app->sceneForTest(config.scene);
}
Engine::~Engine()=default;

int Engine::run() {
    using Clock=std::chrono::steady_clock;
    auto previous=Clock::now();
    int frames=0;
    while (app->active() && (!config.frames || frames<config.frames)) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (!config.smoke) app->event(event);
        }
        if (!app->active()) break;
        const auto now=Clock::now();
        const float elapsed=std::chrono::duration<float>(now-previous).count();
        previous=now;
        app->update(config.smoke?1.f/60:std::min(elapsed,.10f));
        renderer->begin();
        app->draw();
        if (++frames==config.frames && !config.screenshot.empty()) renderer->screenshot(config.screenshot);
        renderer->end();
        if (!vsync && !config.smoke) std::this_thread::sleep_until(now+std::chrono::milliseconds(8));
    }
    renderer->check();
    if (config.smoke) {
        const bool fullscreen=(SDL_GetWindowFlags(platform->window)&sdl::fullscreen)!=0;
        std::cout<<"Rendered "<<frames<<" frames on "<<renderer->driver()
                 <<" | scene "<<config.scene<<" | "<<(fullscreen?"fullscreen":"windowed")<<'\n';
    }
    return 0;
}
}
