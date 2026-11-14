#include "platform/gl.hpp"
#include "platform/sdl.hpp"
#include <stdexcept>

namespace vl {
void GL::load() {
#define VL_LOAD(result,name,args) name=reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress("gl" #name)); \
    if (!name) throw std::runtime_error("Missing OpenGL entry point gl" #name);
    VL_GL_FUNCTIONS(VL_LOAD)
#undef VL_LOAD
}
} // namespace vl
