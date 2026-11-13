#pragma once
#include "app.hpp"
#include <filesystem>
#include <memory>
#include <string>

namespace vl {
struct Options {
    bool windowed=false,smoke=false;
    int frames=0;
    std::string scene="1";
    std::filesystem::path screenshot,save;
};
Options parseOptions(int argc,char** argv);

// Owns the platform window, the OpenGL context and every subsystem, and runs the frame loop.
class Engine {
public:
    explicit Engine(const Options& options);
    ~Engine();
    Engine(const Engine&)=delete;
    Engine& operator=(const Engine&)=delete;
    int run();
private:
    struct Platform;
    Options config;
    std::unique_ptr<Platform> platform;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<Storage> storage;
    std::unique_ptr<App> app;
    bool vsync=false;
};
}
