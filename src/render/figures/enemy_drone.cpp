#include "render/figures/enemy_view.hpp"

namespace vl {
// Hovering scout. It banks into its movement, spins four-blade rotors and watches the player.
// When it dies the rotors shear off, it tumbles trailing fire, and its eye goes dark.
void Renderer::drawDrone(const EnemyView& v) {
    const Enemy& e=v.e;
    const Color black=v.black,hull=v.hull,rim=v.rim,plate=v.plate,alert=v.alert,accent=v.accent;
    const float time=v.time;
    const bool sheared=v.dead>.18f;
    const float bank=std::cos(time*1.42f+e.phase)*.2f*(e.alive?1.f:0.f);
    const Vec at{v.x,v.y+(e.alive?std::sin(time*7.f+e.phase)*.8f:0.f)};
    const float ca=std::cos(bank),sa=std::sin(bank);
    auto local=[&](float lx,float ly) { return at+Vec{lx*ca-ly*sa,lx*sa+ly*ca}; };
    const float live=e.alive?1.f:1-v.burnt;
    for (float side:{-1.f,1.f}) {
        const Vec mount=local(side*19.f,-2.f), rotor=local(side*22.f,-7.f);
        line(local(side*8.f,0),mount,3.4f,black);
        line(local(side*8.f,0),mount,2.1f,hull);
        ellipse(rotor,{4.f,3.f},black);
        if (!sheared) {
            const float flutter=std::sin(time*70.f+side)*2.f;
            ellipse(rotor,{13.f+flutter,1.9f},rim.alpha(.40f));
            line(rotor-Vec{14.f+flutter,0},rotor+Vec{14.f+flutter,0},1.1f,rim.alpha(.85f));
        } else {
            // Bent stub where the blades broke off.
            line(rotor,rotor+Vec{side*3.f,-3.5f},1.6f,black);
        }
        halo(local(side*22.f,5.f),20,accent.alpha(.38f*live));
        line(local(side*22.f,-1.f),local(side*22.f,7.f),2.4f,accent.energy(1.9f*live));
    }
    for (float side:{-1.f,1.f}) {
        line(local(side*7.f,8.f),local(side*11.f,14.f),1.6f,black);
        line(local(side*11.f,14.f),local(side*13.5f,14.f),1.6f,black);
    }
    ellipse(at,{18.f,12.5f},black);
    ellipse(at+Vec{0,-.6f},{16.f,10.7f},hull);
    polygon(local(-14.f,-3.f),local(14.f,-3.f),local(12.f,-7.5f),local(-12.f,-7.5f),plate);
    line(local(-12.f,3.f),local(12.f,3.f),.9f,black.alpha(.8f));
    for (int k=-2;k<=2;++k) line(local(k*4.f,4.f),local(k*4.f+1.f,9.f),.9f,black.alpha(.6f));
    line(local(-13.f,-4.5f),local(13.f,-4.5f),1.f,rim.alpha(.8f));
    line(local(0,-10.f),local(2.f,-17.f),1.1f,black);
    ellipse(local(2.f,-17.5f),{1.3f,1.3f},alert.energy(2.f+std::sin(time*5.f)));
    const Vec look=normal(v.target-at);
    const Vec lens=at+Vec{v.facing*6.f*std::abs(v.turn),0}+(e.alive?look*1.6f:Vec{});
    ellipse(lens,{5.4f,5.1f},black);
    ellipse(lens,{4.2f,4.f},hex(0x2a1215));
    emissive(lens+look*.8f,2.5f,alert,e.warning>0?3.2f:2.f);
    ellipse(lens+look*1.4f,{1.f,1.f},hex(0xfff0e0).energy(2.4f*(e.alive?1.f:alert.r)));
    halo(local(0,9.f),12,accent.alpha(.22f*live));
    // A damaged drone trails smoke from its wounds and shows a cracked lens.
    if (e.alive && e.hp<e.maxHp*.5f) {
        line(lens+Vec{-3.f,-2.5f},lens+Vec{2.f,3.f},.8f,hex(0xe8f0f4).alpha(.8f));
        line(lens+Vec{1.f,-3.f},lens+Vec{-1.f,2.5f},.6f,hex(0xe8f0f4).alpha(.6f));
    }
}
} // namespace vl
