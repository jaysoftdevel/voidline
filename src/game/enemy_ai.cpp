#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::moveEnemies(float dt) {
    const auto& p=player.box;
    const float pace=difficulty->pace;
    for (auto& e:enemies) {
        if (!e.alive) continue;
        e.hit=std::max(0.f,e.hit-dt);
        updateEnemyFeedback(e,dt);
        e.flinch=std::max(0.f,e.flinch-dt*4.5f);
        e.kick=approach(e.kick,0,dt*5.f);
        e.recoil=std::max(0.f,e.recoil-dt*6.f);
        e.landed=std::max(0.f,e.landed-dt*5.f);
        e.turn=approach(e.turn,float(e.facing),dt*(e.kind==EnemyKind::stalker?14.f:8.f));
        const float oldX=e.box.x;
        const Vec origin{e.box.center().x,e.box.y+e.box.h*.43f};
        const Vec target{p.center().x,p.y+p.h*.43f}, d=target-origin;
        if (std::abs(d.x)>690) continue;
        const bool inRange=std::abs(d.x)<e.reach && std::abs(d.y)<240;
        const bool boss=e.kind==EnemyKind::boss;
        // The weapon comes up when the target is in range and drops back after a moment.
        e.raise=approach(e.raise,inRange||e.warning>0?1.f:0.f,dt*(inRange?5.5f:1.6f));
        if (boss && std::abs(d.x)<555 && !bossActive) {
            bossActive=true; message({origin.x,e.box.y-18},stage->boss,hex(stage->accent),1.7f);
            shake=8; audio.event("launch",.76f);
        }

        if (e.kind==EnemyKind::drone) {
            e.box.x=e.origin.x+std::sin(time*1.42f+e.phase)*72;
            e.box.y=e.origin.y+std::sin(time*2.37f+e.phase)*19;
            e.facing=sign(d.x);
        } else if (e.kind==EnemyKind::turret) {
            e.facing=sign(d.x);
        } else {
            const bool stalker=e.kind==EnemyKind::stalker;
            if (inRange) e.facing=sign(d.x);
            float targetSpeed=0;
            if (stalker || (inRange && e.warning<=0 && std::abs(d.x)>(boss?188:171)))
                targetSpeed=e.facing*e.speed*pace*(inRange?1.f:.35f);
            else if (!inRange && !boss) targetSpeed=e.facing*e.speed*.60f*pace;
            if (e.flinch>0) targetSpeed*=.3f;
            e.vx=approach(e.vx,targetSpeed,dt*(stalker?670.f:420.f));
            e.box.x=clamp(e.box.x+e.vx*dt,e.origin.x-e.patrol,e.origin.x+e.patrol);
            if (e.box.x==e.origin.x-e.patrol || e.box.x==e.origin.x+e.patrol) {
                if (!inRange && !boss && e.patrol>0) e.facing=e.box.x<e.origin.x?1:-1;
                e.vx=0;
            }
            e.distance+=std::abs(e.box.x-oldX);
            if (stalker) {
                e.jump=std::max(0.f,e.jump-dt);
                if (inRange && e.jump==0 && std::abs(d.x)<210 && e.box.bottom()>=e.base-.5f) {
                    e.vertical=-454; e.jump=1.72f/pace;
                }
                e.vertical+=1260*dt;
                e.box.y=std::min(e.base-e.box.h,e.box.y+e.vertical*dt);
                if (e.box.bottom()>=e.base-.01f) { if (e.vertical>120) e.landed=1; e.vertical=0; }
            }
            if (boss) e.box.y=e.origin.y-(index==4?33+std::sin(time*2.4f)*24:std::max(0.f,std::sin(time*2.7f))*4);
        }
        if (p.overlaps(e.box)) hurt(e.damage*(boss?1.4f:1.f));
        if (mode!=Mode::active) return;
        if (e.kind==EnemyKind::stalker) continue;
        const bool visible=inRange && entry<.95f && !blocked(origin,target);
        if (e.warning>0) {
            e.warning=std::max(0.f,e.warning-dt);
            if (e.warning==0) {
                if (visible || boss) enemyFire(e);
                e.timer=(boss?patterns[index].interval:spec(e.kind).delay)/pace;
            }
        } else if (visible) {
            e.timer-=dt;
            if (e.timer<=0) {
                e.warning=(boss?.38f:.26f)/pace;
                e.aim=std::atan2(d.y,d.x);
            }
        }
    }
}

void Game::enemyFire(Enemy& e) {
    const bool boss=e.kind==EnemyKind::boss, phase=e.hp<e.maxHp*.48f;
    const auto& pattern=patterns[index];
    const int volley=boss?pattern.fans+(phase?2:0):(e.kind==EnemyKind::turret && index>=2?3:1);
    const float spread=boss?pattern.spread+(phase?.22f:0):.25f;
    const Color color=hex(boss?pattern.color:e.kind==EnemyKind::turret?0xff8f65:0xfd6471);
    const float speed=(boss?pattern.velocity:e.kind==EnemyKind::drone?310:355)*std::sqrt(difficulty->pace);
    const Vec direction=ray(e.aim);
    const Vec origin{e.box.center().x+direction.x*e.box.w*.24f,e.box.y+e.box.h*(.43f+direction.y*.22f)};
    for (int i=0;i<volley;++i) {
        const float offset=volley==1?0:(float(i)/(volley-1)-.5f)*spread;
        hostile.push_back({origin,ray(e.aim+offset)*speed,2.7f,e.damage,color,0,0,boss,false});
    }
    if (boss && ++e.volley%3==0 && index>=1)
        hostile.push_back({{origin.x,e.base-15},{sign(player.box.x-e.box.x)*speed*1.07f,0},2.4f,e.damage*1.3f,color,0,0,true,true});
    e.recoil=1;
    burst(origin,color,boss?10:4,98);
    audio.event("enemy",boss?.78f:.49f,(origin.x-player.box.x)/width,boss?.73f:.91f+random()*.22f);
}
} // namespace vl
