#include "render/figures/humanoid.hpp"

namespace vl {
// Draws one human figure back to front. Each part lives in its own file:
// rig.cpp (skeleton), legs.cpp, torso.cpp (scarf, pack, torso), head.cpp and headgear.cpp, arms.cpp.
void Renderer::humanoid(const Humanoid& h) {
    const Rig rig=buildRig(h);
    if (h.grounded) {
        halo(h.ground-Vec{0,1},24,hex(0x090f19).alpha(.50f));
        ellipse(h.ground,{20.f+h.crouch*4,2.8f},hex(0x090f16).alpha(.56f));
    }
    // A flip rotates the body but not the weapon, so the arms are drawn after the rotation ends.
    if (h.spin!=0) beginSpin({h.ground.x,h.ground.y-26+h.crouch*8},h.spin);
    drawLegs(h,rig);
    if (h.scarf) drawScarf(h,rig);
    drawPack(h,rig);
    drawTorso(h,rig);
    drawHead(h,rig);
    Rig arms=rig;
    if (h.spin!=0) {
        arms.shoulder=transform(rig.shoulder);
        arms.farShoulder=transform(rig.farShoulder);
        endSpin();
    }
    drawArms(h,arms);
}
} // namespace vl
