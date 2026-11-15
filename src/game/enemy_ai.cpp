#include "game/game.hpp"
#include "game/detail.hpp"
#include "game/enemy_ground.hpp"
#include "content/enemy_motion.hpp"

namespace vl {
void Game::moveEnemies(float dt) {
    const auto aimAt=[&](const Player& body) {
        const auto& p=body.box;
        return Vec{p.center().x,p.y+(nearCover(body) && !body.crouched?10.f:p.h*.43f)};
    };
    const Vec first=aimAt(player);
    const Vec second=coop?aimAt(partner.player):first;
    const int later=std::max(0,index-4);
    const auto& pattern=patterns[static_cast<int>(stage->biome)];
    for (auto& e:enemies) {
        if (!e.alive) continue;
        const auto& authored=enemySkeletons[static_cast<size_t>(e.kind)];
        const float pace=difficulty->pace*(1+later*.023f)*(e.elite?1.15f:1.f);
        e.hit=std::max(0.f,e.hit-dt);
        updateEnemyFeedback(e,dt);
        e.flinch=std::max(0.f,e.flinch-dt*4.5f);
        e.kick=approach(e.kick,0,dt*5.f);
        e.recoil=std::max(0.f,e.recoil-dt*6.f);
        e.landed=std::max(0.f,e.landed-dt*5.f);
        e.turn=approach(e.turn,float(e.facing),dt*((e.kind==EnemyKind::stalker ||
            e.kind==EnemyKind::scavenger)?14.f:8.f));
        const float oldX=e.box.x;
        const Vec origin{e.box.center().x,e.box.y+e.box.h*.43f};
        Vec target=first;
        if (coop) {
            const auto priority=[&](Vec at) {
                const Vec delta=at-origin;
                const bool available=std::abs(delta.x)<e.reach && std::abs(delta.y)<240 && !blocked(origin,at);
                return std::abs(delta.x)+std::abs(delta.y)*.4f+(available?0.f:900.f);
            };
            if (priority(second)<priority(first)) target=second;
        }
        const Vec d=target-origin;
        if (std::abs(d.x)>690) continue;
        const bool inRange=std::abs(d.x)<e.reach && std::abs(d.y)<240;
        const bool boss=e.kind==EnemyKind::boss;
        // The weapon comes up when the target is in range and drops back after a moment.
        const bool canAim=(inRange && !blocked(origin,target)) || e.warning>0;
        e.raise=approach(e.raise,canAim?1.f:0.f,dt*(canAim?7.2f*authored.aimRaise:3.0f*authored.braking));
        if (boss && std::abs(d.x)<555 && !bossActive) {
            bossActive=true; message({origin.x,e.box.y-18},stage->boss,hex(stage->accent),1.7f);
            shake=8; audio.event("launch",.76f);
        }
        if (boss && bossActive) {
            const float integrity=float(e.hp)/std::max(1,e.maxHp);
            const int phase=integrity<.35f?2:integrity<.70f?1:0;
            if (phase>bossPhase) {
                bossPhase=phase;
                ring(e.box.center(),phase==2?hex(0xff7453):hex(stage->accent),phase==2?160.f:110.f,.55f);
                burst(e.box.center(),hex(0xffb568),phase==2?35:20,280);
                message(e.box.center()-Vec{0,120},phase==2?"OVERDRIVE  /  PHASE III":"GUARDIAN SHIELDS BROKEN",
                        hex(0xffd48a),2.1f);
                shake=std::max(shake,phase==2?13.f:8.f);
                audio.event("guardian",.75f,0,phase==2?1.23f:.89f);
                e.warning=.4f;
            }
        }

        if (e.kind==EnemyKind::drone || e.kind==EnemyKind::interceptor) {
            const bool interceptor=e.kind==EnemyKind::interceptor;
            const float sway=std::sin(time*(interceptor?4.1f:2.9f)+e.phase*.7f);
            e.box.x=e.origin.x+std::sin(time*(interceptor?2.45f:1.42f)+e.phase)*(interceptor?118.f:76.f)+sway*(interceptor?22.f:10.f);
            e.box.y=e.origin.y+std::sin(time*(interceptor?3.3f:2.37f)+e.phase)*(interceptor?36.f:22.f)+std::cos(time*(interceptor?2.1f:1.4f)+e.phase)*6.f;
            e.facing=sign(d.x);
        } else if (e.kind==EnemyKind::turret) {
            e.facing=sign(d.x);
        } else {
            const bool stalker=e.kind==EnemyKind::stalker;
            const bool scavenger=e.kind==EnemyKind::scavenger;
            const bool sniper=e.kind==EnemyKind::sniper;
            const bool heavy=e.kind==EnemyKind::bulwark || e.kind==EnemyKind::juggernaut;
            if (inRange || (scavenger && std::abs(d.x)<390)) e.facing=sign(d.x);
            float targetSpeed=0;
            if (scavenger && std::abs(d.x)<390) targetSpeed=e.facing*e.speed*pace*(std::abs(d.x)<170?-.55f:1.f);
            else if (sniper && std::abs(d.x)<210) targetSpeed=-e.facing*e.speed*pace;
            else if (sniper && std::abs(d.x)>390) targetSpeed=e.facing*e.speed*.35f*pace;
            else if (stalker || (inRange && e.warning<=0 && std::abs(d.x)>(boss?188:171)))
                targetSpeed=e.facing*e.speed*pace*(inRange?1.f:.35f);
            else if (!inRange && !boss) targetSpeed=e.facing*e.speed*.60f*pace;
            if (heavy) targetSpeed*=.65f;
            if (e.flinch>0) targetSpeed*=.3f;
            e.vx=approach(e.vx,targetSpeed,dt*(stalker || scavenger?670.f:420.f)*
                            (targetSpeed==0?authored.braking:authored.acceleration));
            const float desired=clamp(e.box.x+e.vx*dt,e.origin.x-e.patrol,e.origin.x+e.patrol);
            const float footing=groundedEnemyX(*stage,e,oldX,desired);
            e.box.x=footing;
            // Feet follow the actual stair elevation. Patrol range still caps
            // horizontal travel, and a raised stair never bridges empty air
            // unless the level author provided a continuous staircase.
            if (e.kind!=EnemyKind::stalker || e.vertical==0) {
                if (const auto y=stairFooting(*stage,e.box.center().x,e.base,19.f)) {
                    e.base=*y;
                    e.box.y=e.base-e.box.h;
                }
            }
            if (std::abs(footing-desired)>.02f) {
                // Turn at a ledge, rather than walking across a non-existent floor.
                e.facing=desired>oldX?-1:1;
                e.vx=0;
            }
            if (e.box.x==e.origin.x-e.patrol || e.box.x==e.origin.x+e.patrol) {
                if (!inRange && !boss && e.patrol>0) e.facing=e.box.x<e.origin.x?1:-1;
                e.vx=0;
            }
            if (stalker) {
                e.jump=std::max(0.f,e.jump-dt);
                if (inRange && e.jump==0 && std::abs(d.x)<(e.elite?265.f:225.f) && e.box.bottom()>=e.base-.5f) {
                    e.vertical=e.elite?-535.f:-472.f; e.jump=1.56f/pace;
                    e.vx=e.facing*e.speed*pace*(e.elite?1.35f:1.1f);
                }
                e.vertical+=1260*dt;
                e.box.y=std::min(e.base-e.box.h,e.box.y+e.vertical*dt);
                if (e.box.bottom()>=e.base-.01f) { if (e.vertical>120) e.landed=1; e.vertical=0; }
            }
            if (boss) e.box.y=e.origin.y-(stage->biome==Biome::orbital?33+std::sin(time*2.4f)*24:std::max(0.f,std::sin(time*2.7f))*4);
            for (const auto& wall:stage->cover) {
                if (!e.box.overlaps(wall)) continue;
                if (oldX+e.box.w<=wall.x) e.box.x=wall.x-e.box.w;
                else if (oldX>=wall.right()) e.box.x=wall.right();
                else continue;
                if (!inRange && !boss) e.facing=-e.facing;
                e.vx=0;
            }
            e.distance+=std::abs(e.box.x-oldX);
        }
        const float contact=e.damage*(boss?1.4f:1.f);
        if (player.box.overlaps(e.box)) hurt(contact);
        if (coop && partner.player.box.overlaps(e.box)) {
            swapOperatives(); hurt(contact); swapOperatives();
        }
        if (mode!=Mode::active) return;
        if (e.kind==EnemyKind::stalker || e.kind==EnemyKind::scavenger) continue;
        const bool visible=inRange && entry<.95f && !blocked(origin,target);
        if (e.warning>0) {
            e.warning=std::max(0.f,e.warning-dt);
            if (e.warning==0) {
                if (visible || boss) enemyFire(e,target.x);
                e.timer=(boss?pattern.interval:spec(e.kind).delay)/pace;
            }
        } else if (visible) {
            e.timer-=dt;
            if (e.timer<=0) {
                const float telegraph=e.kind==EnemyKind::sniper?.68f:
                    e.kind==EnemyKind::juggernaut?.52f:
                    e.kind==EnemyKind::bulwark?.42f:boss?.38f:e.elite?.31f:.26f;
                e.warning=std::max(boss?.27f:.19f,telegraph/pace);
                e.aim=std::atan2(d.y,d.x);
            }
        }
    }
}

void Game::enemyFire(Enemy& e,float targetX) {
    const bool boss=e.kind==EnemyKind::boss, phase=e.hp<e.maxHp*.48f;
    const int later=std::max(0,index-4);
    const auto& pattern=patterns[static_cast<int>(stage->biome)];
    const int volley=boss?pattern.fans+bossPhase*2+(phase?2:0)+(later>=8?2:0)
        :e.kind==EnemyKind::juggernaut?7:e.kind==EnemyKind::bulwark?3:
        e.kind==EnemyKind::interceptor?2:e.kind==EnemyKind::sniper?1:
        e.kind==EnemyKind::turret && index>=2?(e.elite?5:3):e.elite?2:1;
    const float spread=boss?pattern.spread+bossPhase*.12f+(phase?.22f:0)+(later>=8?.12f:0):
        e.kind==EnemyKind::sniper?0.f:e.kind==EnemyKind::juggernaut?.83f:
        e.kind==EnemyKind::bulwark?.48f:e.elite?.38f:.25f;
    const Color color=hex(boss?pattern.color:e.kind==EnemyKind::sniper?0x8feaff:
        e.kind==EnemyKind::interceptor?0xce91ff:e.kind==EnemyKind::juggernaut?0xff704a:
        e.kind==EnemyKind::bulwark?0xffcb68:e.elite?0xffcb68:
        e.kind==EnemyKind::turret?0xff8f65:0xfd6471);
    const float speed=(boss?pattern.velocity:e.kind==EnemyKind::sniper?680:
        e.kind==EnemyKind::interceptor?465:e.kind==EnemyKind::juggernaut?390:
        e.kind==EnemyKind::drone?310:355)
        *std::sqrt(difficulty->pace)*(1+later*.012f)*(e.elite?1.12f:1.f);
    const Vec direction=ray(e.aim);
    const Vec origin{e.box.center().x+direction.x*e.box.w*.24f,e.box.y+e.box.h*(.43f+direction.y*.22f)};
    for (int i=0;i<volley;++i) {
        const float offset=volley==1?0:(float(i)/(volley-1)-.5f)*spread;
        hostile.push_back({origin,ray(e.aim+offset)*speed,2.7f,e.damage,color,0,{},boss,false});
        hostile.back().id=++nextShot;
    }
    if (boss && ++e.volley%(bossPhase>=2?2:phase && later>=7?2:3)==0 && index>=1) {
        hostile.push_back({{origin.x,e.base-15},{sign(targetX-e.box.center().x)*speed*1.07f,0},2.4f,e.damage*1.3f,color,0,{},true,true});
        hostile.back().id=++nextShot;
    }
    e.recoil=1;
    burst(origin,color,boss?10:4,98);
    audio.event("enemy",boss?.78f:.49f,(origin.x-player.box.x)/width,boss?.73f:.91f+random()*.22f);
}
} // namespace vl
