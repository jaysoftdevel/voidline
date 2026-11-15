#include "game/game.hpp"
#include "game/detail.hpp"
#include "content/skeleton.hpp"
#include "game/stairs.hpp"

namespace vl {
// Acceleration toward the wanted speed, then the world edges.
void Game::runHorizontal(float dt,int direction,bool grounded) {
    auto& p=player;
    const auto& model=characterSkeletons[static_cast<size_t>(character-characters.data())];
    const float baseSpeed=character->speed*speedScale;
    const bool reversing=direction && p.velocity.x*direction<0;
    const float pace=clamp(std::abs(p.velocity.x)/baseSpeed);
    if (grounded) {
        p.sprinting=!p.crouched && held(Key::sprint) && direction;
        p.sprintDirection=p.sprinting?direction:0;
    }
    if (p.crouched) p.sprinting=false;
    const float speed=baseSpeed*(p.crouched?.40f:p.sprinting?1.28f:1.f);
    const float acceleration=grounded
        ? (reversing?3380.f:p.crouched && std::abs(p.velocity.x)>speed?3200.f:direction?mix(2700.f,1730.f,pace):3000.f)
        : (direction?1320.f:105.f);
    float targetSpeed=direction*speed;
    if (!grounded && p.sprinting && direction!=p.sprintDirection) targetSpeed=direction*baseSpeed;
    p.velocity.x=approach(p.velocity.x,targetSpeed,acceleration*dt*(direction?model.acceleration:model.braking));
    const float edge=bossDead?stage->worldWidth-playerW:stage->exit.x-106;
    const float nextX=p.box.x+p.velocity.x*dt;
    p.box.x=clamp(nextX,12,edge);
    if (p.box.x!=nextX) p.velocity.x=0;
}

// Stops the operative at the sides of solid platforms. Ledges stay one-way during jumps, and a grounded
// operative needs clearance to walk beneath them.
void Game::collideWalls(float oldX,bool grounded) {
    auto& p=player;
    const auto collide=[&](const Rect& s) {
        if (p.box.bottom()<=s.y+.5f || p.box.y>=s.bottom()) return;
        if (oldX+playerW<=s.x && p.box.right()>s.x) {
            p.box.x=s.x-playerW; p.velocity.x=0;
        } else if (oldX>=s.right() && p.box.x<s.right()) {
            p.box.x=s.right(); p.velocity.x=0;
        }
    };
    if (puzzleMode) for (size_t i=0;i<stage->relays.size();++i) {
        const auto& relay=stage->relays[i];
        const auto& shutter=relayStates[i];
        const bool touching=p.box.overlaps(relay.barrier);
        const bool passable=shutter.openness>=.76f && (relayOpen(i) || touching);
        if (!passable && relay.operatorPlayer!=(controllingPartner?1:0)) {
            // Wait for the physical shutter to raise before crossing. If the
            // lever expires while a runner is inside, allow them to exit safely.
            const Rect& wall=relay.barrier;
            if (oldX+playerW<=wall.x && p.box.right()>wall.x) {
                p.box.x=wall.x-playerW; p.velocity.x=0;
            } else if (oldX>=wall.right() && p.box.x<wall.right()) {
                p.box.x=wall.right(); p.velocity.x=0;
            }
        }
    }
    for (const auto& surface:platforms) {
        const auto& s=surface.box;
        const bool lowCeiling=grounded && p.box.bottom()>=s.bottom()-.5f;
        if ((!surface.ground && !surface.cover && !lowCeiling) || p.box.bottom()<=s.y+.5f || p.box.y>=s.bottom()) continue;
        // A walkable staircase meets a solid deck at its surface. At the
        // junction, the character's *leading edge* reaches the deck before
        // their center/feet reach its height. Let the stair transition take
        // precedence over the deck's vertical side collision; the one-way
        // surface landing still prevents falling through the deck.
        bool stairEntry=false;
        if (grounded && p.jumpAge==0 && p.velocity.y>=-15.f && p.velocity.y<130.f &&
            p.box.bottom()>s.y && p.box.bottom()-s.y<=45.f) {
            const float center=p.box.center().x;
            for (const Stair& stair:stage->stairs) {
                if (!validStair(stair) || center<stair.from.x-8.f || center>stair.to.x+8.f) continue;
                const Vec joining=(std::abs(stair.from.y-s.y)<.5f)?stair.from:stair.to;
                if (std::abs(joining.y-s.y)>.5f || joining.x<s.x-2.f || joining.x>s.right()+2.f) continue;
                if (std::abs(stairY(stair,clamp(center,stair.from.x,stair.to.x))-p.box.bottom())<=24.f &&
                    std::abs(stairY(stair,clamp(center,stair.from.x,stair.to.x))-s.y)<=48.f) {
                    stairEntry=true;
                    break;
                }
            }
        }
        if (!stairEntry) collide(s);
    }
}
} // namespace vl
