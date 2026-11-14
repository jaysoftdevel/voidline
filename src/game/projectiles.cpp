#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::moveShots(float dt) {
    for (auto& s:shots) {
        s.life-=dt;
        if (s.life<=0) {
            if (s.blast>0) explode(s.p,s.blast,s.damage,s.color);
            continue;
        }
        s.v.y+=s.gravity*dt;
        Vec change=s.v*dt;
        // Resolve multiple hits in travel order, including a wall after a piercing hit.
        while (s.life>0 && length(change)>.0001f) {
            float first=infinity;
            int hit=-1;
            for (const auto& surface:platforms) first=std::min(first,segment(s.p,change,surface.box));
            for (size_t i=0;i<enemies.size();++i) {
                if (!enemies[i].alive || (s.passed&(1u<<i))) continue;
                const float t=segment(s.p,change,enemies[i].box);
                if (t<first) { first=t; hit=static_cast<int>(i); }
            }
            if (first>1) { s.p+=change; break; }
            s.p+=change*first;
            change=change*(1-first);
            burst(s.p,hex(hit>=0?0xffe5ae:0x9ba8ba),hit>=0?6:3,108);
            if (hit>=0) ring(s.p,s.color,16,.2f);
            if (hit<0) {
                if (s.blast>0) explode(s.p,s.blast,s.damage,s.color);
                s.life=0; break;
            }
            auto& e=enemies[hit];
            if (s.blast>0) { explode(s.p,s.blast,s.damage,s.color); s.life=0; break; }
            if (e.hit<=0) audio.event("hit-metal",.3f,(s.p.x-player.box.x)/590,.92f+random()*.23f);
            s.passed|=1u<<hit;
            damageEnemy(e,s.damage,s.p,normal(s.v),s.crit);
            if (s.pierce>0) --s.pierce;
            else s.life=0;
        }
    }
    for (auto& s:hostile) {
        s.life-=dt;
        if (s.life<=0) continue;
        const Vec change=s.v*dt;
        float cover=infinity;
        for (const auto& surface:platforms) cover=std::min(cover,segment(s.p,change,surface.box));
        const auto& p=player.box;
        const float hit=segment(s.p,change,{p.x+4,p.y+3,p.w-8,p.h-7});
        if (hit<cover) { hurt(s.damage); s.life=0; }
        else if (cover<=1) { burst(s.p+change*cover,s.color,3,77); s.life=0; }
        s.p+=change;
        if (mode!=Mode::active) break;
    }
    std::erase_if(shots,[](const Shot& s){return s.life<=0;});
    std::erase_if(hostile,[](const Shot& s){return s.life<=0;});
}

void Game::explode(Vec at,float radius,float damage,Color color) {
    burst(at,color,26,330);
    burst(at,hex(0xfff0c2),12,420);
    burst(at,hex(0xc9ced4),6,120,ParticleKind::puff);
    burst(at,hex(0x6d747c),4,70,ParticleKind::puff);
    burst(at,hex(0x93a5ac),8,210,ParticleKind::chip);
    ring(at,hex(0xfff0c2),radius,.5f);
    ring(at,color,radius*.6f,.35f);
    shake=std::max(shake,8.f); punch=std::max(punch,.6f); flash=std::max(flash,.3f);
    audio.event("destroy",.8f,(at.x-player.box.x)/width,.78f+random()*.1f);
    for (auto& e:enemies) {
        if (!e.alive) continue;
        const Vec c=e.box.center();
        const float distance=std::max(0.f,length(c-at)-std::max(e.box.w,e.box.h)*.4f);
        if (distance>=radius) continue;
        damageEnemy(e,damage*(1-.65f*distance/radius),c-normal(c-at)*(std::max(e.box.w,e.box.h)*.3f),normal(c-at),false);
    }
    std::erase_if(hostile,[&](const Shot& h){return length(h.p-at)<radius;});
}
} // namespace vl
