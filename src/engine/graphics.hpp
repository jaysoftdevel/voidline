#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace vl {
enum class GraphicsMode {automatic,opengl,dx12,warp};
const char* graphicsName(GraphicsMode mode);
bool isDirectXRenderer(std::string_view renderer);
bool isWarpRenderer(std::string_view renderer);

#ifdef _WIN32
// The DLL search entry and Mesa environment are scoped to the lifetime of the video context.
class WindowsGraphics {
public:
    WindowsGraphics(GraphicsMode mode,const std::filesystem::path& mesa,std::string_view adapter);
    ~WindowsGraphics();
    WindowsGraphics(const WindowsGraphics&)=delete;
    WindowsGraphics& operator=(const WindowsGraphics&)=delete;
    void loadLibrary() const;
private:
    struct State;
    State* state;
};
void printDirectXReport();
#endif
}
