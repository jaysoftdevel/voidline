#include "platform/paths.hpp"
#include <cstdlib>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <vector>
#endif

namespace vl {
namespace fs=std::filesystem;
fs::path pathFromUtf8(std::string_view path) {
    const std::u8string utf8(path.begin(),path.end());
    return fs::path(utf8);
}

std::string utf8Path(const fs::path& path) {
    const auto utf8=path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()),utf8.size()};
}

fs::path executableDirectory() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(512);
    for (;;) {
        const DWORD n=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));
        if (!n) throw std::runtime_error("Could not locate the game executable.");
        if (n<buffer.size()) return fs::path(std::wstring(buffer.data(),n)).parent_path();
        if (buffer.size()>=32768) throw std::runtime_error("The game executable path is too long.");
        buffer.resize(buffer.size()*2);
    }
#else
    return fs::read_symlink("/proc/self/exe").parent_path();
#endif
}

fs::path userDataDirectory() {
#ifdef _WIN32
    PWSTR buffer=nullptr;
    const HRESULT result=SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&buffer);
    fs::path path;
    if (SUCCEEDED(result) && buffer) path=buffer;
    CoTaskMemFree(buffer);
    if (!path.empty()) return path/"VOIDLINE";
    throw std::runtime_error("Could not locate Windows Local AppData. Use --save-file PATH.");
#else
    if (const auto* p=std::getenv("XDG_DATA_HOME"); p && p[0]=='/') return fs::path(p)/"voidline";
    if (const auto* p=std::getenv("HOME"); p && *p) return fs::path(p)/".local/share/voidline";
    throw std::runtime_error("Set XDG_DATA_HOME or HOME to select a save location.");
#endif
}
}
