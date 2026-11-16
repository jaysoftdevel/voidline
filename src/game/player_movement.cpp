#include "game/game.hpp"
#include "game/detail.hpp"
#include "game/stairs.hpp"

namespace vl {
void Game::moveLifts(float dt) {
    for (size_t i=0;i<platforms.size();++i) {
        auto& p=platforms[i];
        if (!p.lift) continue;
        const auto& l=*p.lift;
        const float phase=time*2*pi/l.period+l.phase;
        const Vec offset{l.x+std::sin(phase)*l.dx-p.box.x,l.y+std::sin(phase)*l.dy-p.box.y};
        p.velocity={std::cos(phase)*l.dx*2*pi/l.period,std::cos(phase)*l.dy*2*pi/l.period};
        p.box.x+=offset.x; p.box.y+=offset.y;
        if (dt<=0) continue;
        auto carry=[&](Player& body) {
            if (body.support!=static_cast<int>(i)) return;
            body.box.x+=offset.x; body.box.y+=offset.y;
        };
        carry(player);
        if (coop) carry(partner.player);
    }
}

bool Game::headroom(float x,float feet) const {
    const Rect upper{x+3,feet-playerH,playerW-6,playerH-crouchHeight-.5f};
    for (const auto& platform:platforms) {
        if (upper.overlaps(platform.box)) return false;
    }
    return true;
}

// One fixed step of the operative's movement. Each phase lives in its own function:
// stance (player_stance.cpp), horizontal motion and walls (player_run.cpp), jumping (player_jump.cpp)
// and vertical motion with landing (player_fall.cpp).
void Game::movePlayer(float dt) {
    auto& p=player;
    const int direction=int(held(Key::right))-int(held(Key::left));
    p.dropThrough=std::max(0.f,p.dropThrough-dt);
    if (p.ladder>=0) p.stairDropRequested=false;
    if (p.dropThrough==0) p.dropPlatform=-1;
    const auto nearLadder=[&]() -> int {
        for (size_t i=0;i<stage->ladders.size();++i) {
            const Ladder& l=stage->ladders[i];
            if (std::abs(p.box.center().x-l.x)<26.f &&
                p.box.bottom()>=l.top-28.f && p.box.y<=l.bottom+8.f)
                return static_cast<int>(i);
        }
        return -1;
    };
    const int close=nearLadder();
    // Down at the upper landing engages the vertical ladder for descent.
    // The deck is one-way here: do not collide with it as the feet pass below.
    if (p.ladder<0 && close>=0 && held(Key::crouch) && p.support>=0 &&
        std::abs(platforms[p.support].box.y-stage->ladders[close].top)<8.f &&
        p.dropThrough==0) {
        p.dropPlatform=p.support; p.dropThrough=.65f; p.support=-1;
        p.ladder=close; p.velocity={}; p.box.y+=3.f;
    }
    if (p.ladder<0 && close>=0 && p.dropThrough==0 &&
        ((held(Key::jump) && p.box.bottom()>stage->ladders[close].top+8.f) ||
         (p.velocity.y>65.f && p.support<0 && p.box.bottom()>=stage->ladders[close].top-5.f))) {
        // Ladder climbing uses the standing rig, but a crouched player must
        // keep the same feet position when the collision box grows.
        if (p.crouched) {
            const float feet=p.box.bottom();
            p.crouched=false; p.box.h=playerH; p.box.y=feet-playerH;
        }
        p.ladder=close; p.velocity={}; p.support=-1; p.windup=0;
        p.jumpBuffer=0; p.airJumpUsed=false;
    }
    if (p.ladder>=0) {
        const Ladder& l=stage->ladders[static_cast<size_t>(p.ladder)];
        if (direction!=0 && !held(Key::jump)) {
            // Move sideways to step away; gravity resumes normally.
            p.ladder=-1; p.velocity.x=direction*character->speed*.36f;
        } else {
            p.box.x=approach(p.box.x,l.x-p.box.w*.5f,dt*300.f);
            const float before=p.box.bottom();
            const float climb=(held(Key::jump)?1.f:0.f)-(held(Key::crouch)?1.f:0.f);
            const float speed=136.f*climb;
            p.box.y=clamp(p.box.y-speed*dt,l.top-p.box.h,l.bottom-p.box.h);
            p.velocity={}; p.support=-1; p.crouched=false;
            p.gait.cycle=std::fmod(p.gait.cycle+std::abs(before-p.box.bottom())/82.f,1.f);
            p.gait.weight=approach(p.gait.weight,climb!=0?1.f:0.f,dt*8.f);
            p.gait.run=0; p.gait.crouch=0;
            tickPlayerTimers(dt,false);
            if ((p.box.bottom()<=l.top+.25f && climb>0) ||
                (p.box.bottom()>=l.bottom-.25f && climb<0)) {
                const float deck=climb>0?l.top:l.bottom;
                p.box.y=deck-p.box.h; p.ladder=-1;
                for (size_t i=0;i<platforms.size();++i)
                    if (std::abs(platforms[i].box.y-deck)<8.f &&
                        platforms[i].box.x<=l.x && platforms[i].box.right()>=l.x) {
                        p.support=static_cast<int>(i);break;
                    }
            }
            // Climbing is a lower-body task; the torso can still swivel to
            // aim and fire. Aim direction does not change the ladder grip.
            p.aiming=held(Key::fire) || held(Key::automatic) ||
                p.fireBuffer>0 || p.muzzle>0;
            aimPlayer(0);
            return;
        }
    }
    const int priorSupport=p.support;
    const bool onStairs=p.dropThrough==0 &&
        stairFooting(*stage,p.box.center().x,p.box.bottom(),13.f).has_value() &&
        p.jumpAge==0 && p.velocity.y>=-15.f && p.velocity.y<130.f;
    // Staircases are walkable slopes, not vertical ladder rungs. Pressing
    // Down drops through the one-way staircase; hold Up to climb ladders.
    // Only a *new* Down press drops through stairs. Someone already crouched
    // before stepping onto a slope must still be able to crouch-jump.
    const bool stairDrop=p.stairDropRequested;
    p.stairDropRequested=false;
    if (onStairs && stairDrop && held(Key::crouch)) {
        p.dropThrough=.52f; p.support=-1; p.velocity.y=std::max(125.f,p.velocity.y);
        p.box.y+=4.f;
    }
    const bool grounded=p.support>=0 || (onStairs && p.dropThrough==0);
    updateStance(grounded);
    const float oldX=p.box.x, oldV=p.velocity.x;
    runHorizontal(dt,direction,grounded);
    collideWalls(oldX,grounded);
    tickPlayerTimers(dt,grounded);
    handleJump(grounded,priorSupport,direction,dt);
    const Landing fall=moveVertical(dt,oldX);
    // Sloped, one-way stair surface: only capture a walking operative, never
    // one who has deliberately jumped or is falling quickly from above.
    if (p.dropThrough==0 && p.jumpAge==0 && p.velocity.y>=-15.f && p.velocity.y<130.f && grounded) {
        if (const auto y=stairFooting(*stage,p.box.center().x,p.box.bottom(),18.f)) {
            p.box.y=*y-p.box.h;
            p.velocity.y=0;
            p.airJumpUsed=false;
            p.jumpAge=0;
            // Keep a ground-level support index for stable idle/aim pose; the
            // actual elevation is always decided by stairFooting each tick.
            if (p.support<0) p.support=priorSupport;
        }
    }
    updateGait(dt,p.box.x-oldX,(p.velocity.x-oldV)/dt,fall.attached,fall.strength);
    if (p.box.y>height+95) {
        recoverFromFall();
        if (mode!=Mode::active) return;
    }
    p.aiming=held(Key::fire) || held(Key::automatic) || p.fireBuffer>0 || p.muzzle>0;
    aimPlayer(direction);
}
} // namespace vl
