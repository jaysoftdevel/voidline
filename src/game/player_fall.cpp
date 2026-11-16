#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// Gravity, landing on platforms and the moving lifts. Returns how hard the landing was (zero when airborne)
// and which platform carried the operative before the step.
Game::Landing Game::moveVertical(float dt,float oldX) {
    auto& p=player;
    if (p.jumpAge>0) p.jumpAge+=dt;
    const bool rising=p.velocity.y<0;
    const bool releasedEarly=p.jumpAge>.13f && !held(Key::jump) && p.velocity.y<-80;
    const float weight=rising?(releasedEarly?2.65f:1.f):1.13f;
    const float feet=p.box.bottom();
    const int attached=p.support;
    p.velocity.y=std::min(1075.f,p.velocity.y+gravity*weight*dt);
    p.box.y+=p.velocity.y*dt;
    p.support=-1;
    Landing result{0,attached};
    if (p.velocity.y<0) return result;
    for (size_t i=0;i<platforms.size();++i) {
        const auto& platform=platforms[i];
        if (p.dropThrough>0 && static_cast<int>(i)==p.dropPlatform) continue;
        const auto& s=platform.box;
        const Vec surfaceMotion=static_cast<int>(i)==attached?Vec{}:platform.velocity*dt;
        const float previousSurface=s.y-surfaceMotion.y;
        if (feet>previousSurface+2 || p.box.bottom()<s.y) continue;
        const float t=clamp((previousSurface-feet)/std::max(.001f,p.box.bottom()-feet-surfaceMotion.y));
        const float x=mix(oldX,p.box.x,t)+surfaceMotion.x*(1-t);
        if (x+playerW-5<=s.x || x+5>=s.right()) continue;
        // Once attached, the final footprint must still overlap that surface.
        if (p.box.right()-5<=s.x || p.box.x+5>=s.right()) continue;
        if (p.support<0 || s.y<platforms[p.support].box.y) p.support=static_cast<int>(i);
    }
    if (p.support<0) return result;
    const auto& surface=platforms[p.support];
    const auto& s=surface.box;
    const float impact=p.velocity.y-surface.velocity.y;
    if (attached<0 && impact>185) {
        result.strength=clamp((impact-105)/525,.16f,1);
        p.landed=result.strength;
        const char* surfaceSound=surface.ground?footSounds[static_cast<int>(stage->biome)]:"boot-grate";
        audio.event("land",.30f+result.strength*.66f,0,1.05f-result.strength*.15f+random()*.06f);
        audio.event(surfaceSound,.26f+result.strength*.37f,0,.75f+random()*.11f);
        burst({p.box.center().x,s.y},hex(stage->cold),5+int(result.strength*9),62+result.strength*54,ParticleKind::smoke);
    }
    p.velocity.y=0; p.box.y=s.y-p.box.h; p.jumpAge=0; p.airJumpUsed=false; p.flip=0;
    if (surface.ground && p.box.x>s.x+40 && p.box.x<s.right()-68) p.safe={p.box.x,s.y};
    return result;
}

// Falling out of the world costs a little suit integrity and returns the operative to the last safe footing.
void Game::recoverFromFall() {
    auto& p=player;
    hurt(26,true);
    if (mode!=Mode::active) return;
    const int hp=p.hp;
    spawn(p.safe);
    p.hp=hp; p.flash=.69f;
    std::erase_if(hostile,[&](const Shot& s){return std::abs(s.p.x-p.box.x)<160;});
    message({p.box.center().x,p.box.y-12},"FALL RECOVERY",hex(0xffad6a));
}
} // namespace vl
