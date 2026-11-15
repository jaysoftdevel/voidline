#include "engine/engine.hpp"
#include "platform/paths.hpp"
#include "content/model_io.hpp"
#include "content/runtime_model_catalog.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace vl {
struct Engine::Platform {
#ifdef _WIN32
    std::unique_ptr<WindowsGraphics> graphics;
#endif
    SDL_Window* window=nullptr;
    void* context=nullptr;
    bool initialized=false;

    Platform(const Options& options,GraphicsMode mode,const std::filesystem::path& mesa) {
#ifdef _WIN32
        graphics=std::make_unique<WindowsGraphics>(mode,mesa,options.adapter);
#else
        (void)mode;
        (void)mesa;
#endif
        SDL_SetMainReady();
        if (SDL_Init(sdl::video)<0) {
            const std::string error=SDL_GetError();
            SDL_Quit();
            throw std::runtime_error(error);
        }
        initialized=true;
        try {
#ifdef _WIN32
            graphics->loadLibrary();
#endif
            SDL_GL_SetAttribute(5,1);  // Double buffering.
            SDL_GL_SetAttribute(17,3); // Context major version.
            SDL_GL_SetAttribute(18,3); // Context minor version.
            SDL_GL_SetAttribute(21,1); // Core profile.
            const U32 flags=sdl::openGL|sdl::resizable|sdl::highDPI|(options.smoke?sdl::hidden:0);
            window=SDL_CreateWindow("VOIDLINE",sdl::centered,sdl::centered,options.width,options.width/2,flags);
            if (!window) throw std::runtime_error(SDL_GetError());
            SDL_SetWindowMinimumSize(window,720,360);
            SDL_StopTextInput();
            context=SDL_GL_CreateContext(window);
            if (!context)
                throw std::runtime_error(std::string("OpenGL 3.3 core context unavailable: ")+SDL_GetError());
        } catch (...) {
            shutdown();
            throw;
        }
    }
    ~Platform() { shutdown(); }
    void shutdown() {
        if (context) SDL_GL_DeleteContext(context);
        if (window) SDL_DestroyWindow(window);
        if (initialized) SDL_Quit();
    }
};

Engine::Engine(const Options& options):config(options) {
    const auto root=executableDirectory();
    const auto media=root/"assets";
    loadBakedCharacterModels(media);
    loadBakedEnemyModels(media);
#ifdef _WIN32
    const auto mesa=config.mesa.empty()?root/"mesa":config.mesa;
    const bool automatic=config.graphics==GraphicsMode::automatic;
    const std::vector<GraphicsMode> modes=automatic
        ?std::vector<GraphicsMode>{GraphicsMode::dx12,GraphicsMode::opengl,GraphicsMode::warp}
        :std::vector<GraphicsMode>{config.graphics};
    std::string errors;
    for (const auto mode:modes) {
        try {
            auto candidate=std::make_unique<Platform>(config,mode,mesa);
            auto output=std::make_unique<Renderer>(candidate->window,media,mode==GraphicsMode::warp);
            const auto driver=output->driver();
            if (mode==GraphicsMode::dx12 && (!isDirectXRenderer(driver) || isWarpRenderer(driver)))
                throw std::runtime_error("Hardware Direct3D 12 was requested, but the runtime selected "+driver);
            if (mode==GraphicsMode::warp && (!isDirectXRenderer(driver) || !isWarpRenderer(driver)))
                throw std::runtime_error("Direct3D 12 WARP was requested, but the runtime selected "+driver);
            const std::string label=graphicsName(mode);
            std::cout<<"Graphics: "<<label<<" | "<<driver<<'\n';
            SDL_SetWindowTitle(candidate->window,("VOIDLINE - "+label).c_str());
            platform=std::move(candidate);
            renderer=std::move(output);
            break;
        } catch (const std::runtime_error& error) {
            if (!errors.empty()) errors+="\n";
            errors+=std::string(graphicsName(mode))+": "+error.what();
        }
    }
    if (!renderer) throw std::runtime_error(errors);
    if (!errors.empty()) std::cerr<<errors<<'\n';
#else
    platform=std::make_unique<Platform>(config,GraphicsMode::opengl,std::filesystem::path{});
    renderer=std::make_unique<Renderer>(platform->window,media);
#endif
    if (SDL_Init(sdl::audio)<0) std::cerr<<"SDL audio initialization: "<<SDL_GetError()<<'\n';
    if (SDL_Init(sdl::gameController)<0) std::cerr<<"Gamepad support unavailable: "<<SDL_GetError()<<'\n';
    if (!config.windowed && SDL_SetWindowFullscreen(platform->window,sdl::fullscreen)<0)
        std::cerr<<"Fullscreen unavailable: "<<SDL_GetError()<<'\n';
    vsync=SDL_GL_SetSwapInterval(config.smoke?0:1)==0 && !config.smoke;
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
    double updateMs=0,drawMs=0,presentMs=0,worstMs=0;
    const auto started=Clock::now();
    auto millis=[](Clock::time_point a,Clock::time_point b) { return std::chrono::duration<double,std::milli>(b-a).count(); };
    while (app->active() && (!config.frames || frames<config.frames)) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (!config.smoke) app->event(event);
        }
        if (!app->active()) break;
        const auto now=Clock::now();
        const float elapsed=std::chrono::duration<float>(now-previous).count();
        previous=now;
        app->sampleFrame(config.smoke?1.f/60:elapsed);
        app->update(config.smoke?1.f/60:std::min(elapsed,.10f));
        const auto afterUpdate=Clock::now();
        renderer->begin();
        app->draw();
        const auto afterDraw=Clock::now();
        if (++frames==config.frames && !config.screenshot.empty()) renderer->screenshot(config.screenshot,app->followCrop(config.follow));
        renderer->end();
        const auto done=Clock::now();
        if (!config.smoke) renderer->adapt(float(millis(now,done)+std::max(0.0,millis(done,Clock::now()))));
        updateMs+=millis(now,afterUpdate); drawMs+=millis(afterUpdate,afterDraw); presentMs+=millis(afterDraw,done);
        worstMs=std::max(worstMs,millis(now,done));
        if (!vsync && !config.smoke) std::this_thread::sleep_until(now+std::chrono::milliseconds(8));
    }
    renderer->check();
    if (config.smoke) {
        const bool fullscreen=(SDL_GetWindowFlags(platform->window)&sdl::fullscreen)!=0;
        std::cout<<"Rendered "<<frames<<" frames on "<<renderer->driver()
                 <<" | scene "<<config.scene<<" | "<<(fullscreen?"fullscreen":"windowed")<<'\n';
        const double total=millis(started,Clock::now());
        std::cout<<std::fixed<<std::setprecision(2)<<"Frame time: "<<total/std::max(1,frames)<<" ms average ("
                 <<1000.0*frames/std::max(1.0,total)<<" fps), update "<<updateMs/std::max(1,frames)<<" ms, draw submission "
                 <<drawMs/std::max(1,frames)<<" ms, present and GPU wait "<<presentMs/std::max(1,frames)
                 <<" ms, worst frame "<<worstMs<<" ms\n";
    }
    return 0;
}
} // namespace vl
