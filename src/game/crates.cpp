#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
int Game::crateOnSegment(Vec start,Vec change,float& first) const {
    int hit=-1;
    for (int i=0;i<static_cast<int>(crates.size());++i) {
        const auto& crate=crates[i];
        if (crate.spent) continue;
        const float t=segment(start,change,crate.box);
        if (t<first) { first=t; hit=i; }
    }
    return hit;
}

void Game::strikeCrate(Explosive& crate,Vec hit) {
    crate.hits=std::max(0,crate.hits-1);
    const Color color=hex(crateSpec(crate.kind).color);
    burst(hit,color,5,108);
    audio.event("hit-metal",.4f,(hit.x-player.box.x)/width,.9f+random()*.18f);
}

void Game::detonateCrates() {
    // Queue the chain after projectile updates. A guardian death may clear hostile shots.
    bool pending;
    do {
        pending=false;
        for (auto& crate:crates) {
            if (crate.spent || crate.hits>0) continue;
            crate.spent=true;
            pending=true;
            const Vec at=crate.box.center();
            const auto& spec=crateSpec(crate.kind);
            blastEffects(at,spec.radius,hex(spec.color));
            auto damageAt=[&](Vec target,float bodyRadius) {
                const float distance=std::max(0.f,length(target-at)-bodyRadius);
                if (distance>=spec.radius) return 0.f;
                return spec.damage*(1-.78f*distance/spec.radius)*(blocked(at,target)?.3f:1.f);
            };
            for (auto& e:enemies) {
                if (!e.alive) continue;
                const Vec target=e.box.center();
                const float damage=damageAt(target,e.box.w*.32f);
                if (damage>0) damageEnemy(e,damage,target,normal(target-at),false);
            }
            const auto hurtBody=[&] {
                const float damage=damageAt(player.box.center(),player.box.w*.30f);
                if (damage>0) hurt(damage,true);
            };
            hurtBody();
            if (coop) { swapOperatives(); hurtBody(); swapOperatives(); }
            for (auto& other:crates) {
                if (!other.spent && length(other.box.center()-at)<spec.radius) other.hits=0;
            }
        }
    } while (pending);
}
}
