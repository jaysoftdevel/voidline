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
    const int priorSupport=p.support;
    const bool onStairs=stairFooting(*stage,p.box.center().x,p.box.bottom(),13.f).has_value() &&
        p.jumpAge==0 && p.velocity.y>=-15.f && p.velocity.y<130.f;
    const bool grounded=priorSupport>=0 || onStairs;
    updateStance(grounded);
    const float oldX=p.box.x, oldV=p.velocity.x;
    runHorizontal(dt,direction,grounded);
    collideWalls(oldX,grounded);
    tickPlayerTimers(dt,grounded);
    handleJump(grounded,priorSupport,direction,dt);
    const Landing fall=moveVertical(dt,oldX);
    // Sloped, one-way stair surface: only capture a walking operative, never
    // one who has deliberately jumped or is falling quickly from above.
    if (p.jumpAge==0 && p.velocity.y>=-15.f && p.velocity.y<130.f && grounded) {
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
