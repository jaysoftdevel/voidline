#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
bool Game::blocked(Vec a,Vec b) const {
    for (const auto& surface:platforms)
        if (segment(a,b-a,surface.box)<.985f) return true;
    return false;
}

void Game::aimPlayer(int direction) {
    auto& p=player;
    const Vec origin=weaponPivot();
    if (p.sprinting && direction && !p.aiming) {
        p.facing=p.gait.direction;
        p.aim=p.facing>0?.14f:pi-.14f;
        return;
    }
    if (stickActive) {
        if (std::abs(stickAim.x)>.2f) p.facing=sign(stickAim.x);
        p.aim=std::atan2(stickAim.y,stickAim.x); return;
    }
    if (pointerActive && !held(Key::automatic)) {
        const Vec d=mouse+Vec{camera,0}-origin;
        if (std::abs(d.x)>4) p.facing=sign(d.x);
        p.aim=std::atan2(d.y,d.x); return;
    }
    if (direction) p.facing=direction;
    float best=900;
    p.aim=p.facing>0?-.025f:pi+.025f;
    for (const auto& enemy:enemies) {
        if (!enemy.alive) continue;
        const Vec target{enemy.box.center().x,enemy.box.y+enemy.box.h*.48f}, d=target-origin;
        const float distance=std::hypot(d.x,d.y*1.35f)*(sign(d.x)==p.facing?1.f:1.45f);
        if (distance>=best || blocked(origin,target)) continue;
        best=distance; p.aim=std::atan2(d.y,d.x);
    }
    p.facing=std::cos(p.aim)<0?-1:1;
}

Vec Game::weaponPivot() const { return {player.box.center().x,player.box.bottom()-mix(29.f,17.f,player.gait.crouch)}; }

Vec Game::muzzle() const { return weaponPivot()+ray(player.aim)*muzzleReach[gun]; }
} // namespace vl
