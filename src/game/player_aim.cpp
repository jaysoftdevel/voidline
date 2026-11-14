#include "game/game.hpp"
#include "game/detail.hpp"
#include "content/skeleton.hpp"

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
        const Vec d=mouse-origin;
        if (std::abs(d.x)>4) p.facing=sign(d.x);
        p.aim=std::atan2(d.y,d.x); return;
    }
    if (direction) p.facing=direction;
    const bool peeking=!p.crouched && nearCover();
    float best=900;
    p.aim=p.facing>0?-.025f:pi+.025f;
    for (const auto& enemy:enemies) {
        if (!enemy.alive) continue;
        Vec target{enemy.box.center().x,enemy.box.y+enemy.box.h*.48f};
        if (blocked(origin,target)) {
            if (!peeking) continue;
            target.y=enemy.box.y+5;
            if (blocked(origin,target)) continue;
        }
        const Vec d=target-origin;
        const float distance=std::hypot(d.x,d.y*1.35f)*(sign(d.x)==p.facing?1.f:1.45f);
        if (distance>=best) continue;
        best=distance; p.aim=std::atan2(d.y,d.x);
    }
    if (best==900 && held(Key::automatic) && !p.crouched) {
        float nearest=470;
        for (const auto& crate:crates) {
            const Vec d=crate.box.center()-origin;
            const float distance=length(d);
            if (crate.spent || sign(d.x)!=p.facing || distance>=nearest
                || distance<=crateSpec(crate.kind).radius+18 || blocked(origin,crate.box.center())) continue;
            p.aim=std::atan2(d.y,d.x);
            nearest=distance;
        }
    }
    p.facing=std::cos(p.aim)<0?-1:1;
}

bool Game::nearCover(const Player& body) const {
    const auto& p=body.box;
    if (body.support<0) return false;
    return std::any_of(platforms.begin(),platforms.end(),[&](const Platform& surface) {
        const auto& b=surface.box;
        return surface.cover && std::abs(p.bottom()-b.bottom())<2
            && p.right()+40>b.x && p.x-40<b.right();
    });
}

Vec Game::weaponPivot(const Player& p) const {
    const float reach=nearCover(p)?(p.crouched?17.f:47.f):mix(29.f,17.f,p.gait.crouch);
    return {p.box.center().x,p.box.bottom()-reach};
}
Vec Game::weaponPivot() const { return weaponPivot(player); }
Vec Game::muzzle(const Operative& op) const {
    const auto operativeIndex=static_cast<size_t>(op.character-characters.data());
    const float seat=operativeIndex<characterSkeletons.size()?characterSkeletons[operativeIndex].weaponSeat:0.f;
    return weaponPivot(op.player)+ray(op.player.aim)*muzzleReach[op.gun]+Vec{seat*float(op.player.facing),0};
}
Vec Game::muzzle() const { return muzzle(operative); }
} // namespace vl
