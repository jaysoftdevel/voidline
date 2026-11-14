#ifdef _WIN32
#include "engine/graphics.hpp"
#include "platform/paths.hpp"
#include "platform/sdl.hpp"
#include <windows.h>
#include <memory>
#include <stdexcept>
#include <vector>

namespace vl {
namespace {
class Environment {
public:
    Environment(const wchar_t* name,const wchar_t* value):key(name) {
        const DWORD size=GetEnvironmentVariableW(key,nullptr,0);
        if (size) {
            previous.resize(size);
            const DWORD count=GetEnvironmentVariableW(key,previous.data(),size);
            previous.resize(count);
            hadValue=true;
        }
        if (!SetEnvironmentVariableW(key,value) && (value || GetLastError()!=ERROR_ENVVAR_NOT_FOUND))
            throw std::runtime_error("Could not configure the Direct3D 12 translation driver.");
    }
    ~Environment() {
        SetEnvironmentVariableW(key,hadValue?previous.c_str():nullptr);
    }
    Environment(const Environment&)=delete;
private:
    const wchar_t* key;
    std::wstring previous;
    bool hadValue=false;
};

std::filesystem::path systemOpenGL() {
    std::wstring path(32768,L'\0');
    const auto n=GetSystemDirectoryW(path.data(),static_cast<UINT>(path.size()));
    if (!n || n>=path.size()) throw std::runtime_error("Could not locate the Windows OpenGL runtime.");
    path.resize(n);
    return std::filesystem::path(path)/"opengl32.dll";
}
}

struct WindowsGraphics::State {
    std::filesystem::path library;
    DLL_DIRECTORY_COOKIE directory=nullptr;
    std::vector<std::unique_ptr<Environment>> vars;
    ~State() {
        if (directory) RemoveDllDirectory(directory);
    }
    void set(const wchar_t* name,const wchar_t* value) {
        vars.push_back(std::make_unique<Environment>(name,value));
    }
};

WindowsGraphics::WindowsGraphics(GraphicsMode mode,const std::filesystem::path& mesa,std::string_view adapter) {
    auto config=std::make_unique<State>();
    if (mode==GraphicsMode::opengl) config->library=systemOpenGL();
    else {
        config->library=mesa/"opengl32.dll";
        if (!std::filesystem::is_regular_file(config->library) || !std::filesystem::is_regular_file(mesa/"dxil.dll"))
            throw std::runtime_error("Direct3D 12 compatibility runtime is missing. Run install-mesa.bat.");
        if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))
            throw std::runtime_error("Could not configure the Windows DLL search path.");
        config->directory=AddDllDirectory(mesa.c_str());
        if (!config->directory) throw std::runtime_error("Could not open the Mesa runtime directory.");

        config->set(L"GALLIUM_DRIVER",L"d3d12");
        config->set(L"LIBGL_ALWAYS_SOFTWARE",mode==GraphicsMode::warp?L"1":L"0");
        if (mode==GraphicsMode::warp) config->set(L"MESA_D3D12_DEFAULT_ADAPTER_NAME",nullptr);
        else if (!adapter.empty()) {
            const auto name=pathFromUtf8(adapter).wstring();
            config->set(L"MESA_D3D12_DEFAULT_ADAPTER_NAME",name.c_str());
        }
    }
    state=config.release();
}

WindowsGraphics::~WindowsGraphics() { delete state; }

void WindowsGraphics::loadLibrary() const {
    if (SDL_GL_LoadLibrary(utf8Path(state->library).c_str())<0)
        throw std::runtime_error(std::string("Graphics runtime could not load: ")+SDL_GetError());
}
}
#endif
