#include "render/renderer.hpp"

namespace vl {
void Renderer::operativePose(const Player& player,const Player& previous,float alpha) {
    const Vec position{player.box.x,player.box.y};
    const Vec prior{previous.box.x,previous.box.y};
    const float blend=length(prior-position)>30.f?1.f:alpha;
    playerShift=(prior-position)*(1-blend);
    for (int i=0;i<2;++i) {
        drawHeels[i]=mix(previous.gait.feet[i].heel,player.gait.feet[i].heel,blend);
        drawFootAngles[i]=mix(previous.gait.feet[i].angle,player.gait.feet[i].angle,blend);
    }
    pose=player.gait;
    const auto lowReady=[](const Player& p) { return p.sprinting && !p.aiming ? p.gait.run : 0.f; };
    carry=mix(lowReady(previous),lowReady(player),blend);
    pose.lean=mix(previous.gait.lean,pose.lean,blend);
    pose.crouch=mix(previous.gait.crouch,pose.crouch,blend);
    pose.pelvis=mix(previous.gait.pelvis,pose.pelvis,blend);
    pose.compression=mix(previous.gait.compression,pose.compression,blend);
    pose.tuck=mix(previous.gait.tuck,pose.tuck,blend);
    pose.reach=mix(previous.gait.reach,pose.reach,blend);
}

void Renderer::drawWorld(const Game& g,bool high,bool motion,float alpha,const Player& previous,float previousCamera,const Player* previousPartner) {
    const float shake=motion?g.shake:0.f;
    offset={-mix(previousCamera,g.camera,alpha)+std::cos(g.time*119)*shake*.55f,std::sin(g.time*97)*shake*.38f};
    operativePose(g.player,previous,alpha);
    priorPartner=previousPartner; partnerBlend=alpha;
    const int targetW=software?std::clamp(int(viewW*resolution),640,960)/16*16:
        high?std::clamp(int(viewW*resolution),960,1920)/16*16:1200;
    const int targetH=targetW/2;
    if (raw.texture.w!=targetW) {
        destroy(raw); destroy(lit); destroy(mirror); destroy(ldr);
        raw=target(targetW,targetH); lit=target(targetW,targetH); ldr=target(targetW,targetH,GL_RGBA8);
        mirror=target(targetW/2,targetH/2);
    }
    // The reflection samples its own target, avoiding feedback from the water surface.
    bind(mirror,true); gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    backdrop(g); depthScenery(g); scenery(g); terrain(g); objects(g); actors(g);
    bind(raw,true);
    backdrop(g); ambient(g); depthScenery(g); scenery(g); shafts(g);
    water(g,high);
    terrain(g); objects(g); drawParticles(g); foreground(g);
    flush();
    collectLights(g);
    drawLights(high);
    post(g,high,motion);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& m:g.messages) {
        const Vec p=m.p+offset;
        const float age=m.maxLife-m.life;
        const float size=(m.text.size()>11?15.f:20.f)*m.scale*(1+.55f*std::exp(-age*14));
        outlinedText({p.x-measure(m.text,size,true)*.5f,p.y},m.text,size,m.color.alpha(clamp(m.life*4)));
    }
}
} // namespace vl
