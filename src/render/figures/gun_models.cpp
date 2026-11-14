#include "render/renderer.hpp"
#include "render/figures/gun_art.hpp"

namespace vl {
// All armory previews now use the same hand-painted sprite pipeline as gameplay.
void Renderer::showcaseGun(int gun,Rect box) {
    if (gun<0 || gun>=static_cast<int>(figures::gunArt.size())) return;
    const auto& texture=asset(figures::gunArt[gun].name);
    const float fit=std::min(box.w/texture.w,box.h/texture.h);
    image(figures::gunArt[gun].name,{box.x+(box.w-texture.w*fit)*.5f,
          box.y+(box.h-texture.h*fit)*.5f,texture.w*fit,texture.h*fit});
}
} // namespace vl
