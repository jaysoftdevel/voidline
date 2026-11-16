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
            int hitSurface=-1;
            for (int j=0;j<static_cast<int>(platforms.size());++j) {
                const float distance=segment(s.p,change,platforms[j].box);
                if (distance<first) { first=distance; hitSurface=j; }
            }
            const int crate=crateOnSegment(s.p,change,first);
            for (int i=0;i<static_cast<int>(enemies.size());++i) {
                if (!enemies[i].alive || std::find(s.passed.begin(),s.passed.end(),i)!=s.passed.end()) continue;
                const float t=segment(s.p,change,enemies[i].box);
                if (t<first) { first=t; hit=i; }
            }
            if (first>1) { s.p+=change; break; }
            s.p+=change*first;
            change=change*(1-first);
            burst(s.p,hex(hit>=0?0xffe5ae:0x9ba8ba),hit>=0?6:3,108);
            if (hit>=0) ring(s.p,s.color,16,.2f);
            if (hit<0) {
                if (crate>=0) strikeCrate(crates[crate],s.p);
                else if (hitSurface>=0 && s.blast<=0) {
                    audio.event(impactSound(platforms[hitSurface],stage->biome),.43f,
                        (s.p.x-player.box.x)/width,.95f+random()*.12f);
                }
                if (s.blast>0) explode(s.p,s.blast,s.damage,s.color);
                s.life=0; break;
            }
            auto& e=enemies[hit];
            if (s.blast>0) { explode(s.p,s.blast,s.damage,s.color); s.life=0; break; }
            if (e.hit<=0) audio.event("hit-metal",.3f,(s.p.x-player.box.x)/590,.92f+random()*.23f);
            float impact=s.damage;
            const float distance=length(e.box.center()-player.box.center());
            if (distance<270.f) impact*=s.closeMultiplier;
            damageEnemy(e,impact,s.p,normal(s.v),s.crit);
            if (s.pierce>0) { s.passed.push_back(hit); --s.pierce; }
            else s.life=0;
        }
    }
    for (auto& s:hostile) {
        s.life-=dt;
        if (s.life<=0) continue;
        const Vec change=s.v*dt;
        float cover=infinity;
        int hitSurface=-1;
        for (int j=0;j<static_cast<int>(platforms.size());++j) {
            const float distance=segment(s.p,change,platforms[j].box);
            if (distance<cover) { cover=distance; hitSurface=j; }
        }
        const int crate=crateOnSegment(s.p,change,cover);
        const auto hitBody=[&](const Player& body) {
            const auto& p=body.box;
            return segment(s.p,change,{p.x+4,p.y+3,p.w-8,p.h-7});
        };
        const float hit=hitBody(player);
        const float guestHit=coop?hitBody(partner.player):infinity;
        if (guestHit<=1 && guestHit<hit && guestHit<cover) {
            swapOperatives(); hurt(s.damage); swapOperatives();
            s.life=0;
        } else if (hit<=1 && hit<cover) { hurt(s.damage); s.life=0; }
        else if (cover<=1) {
            const Vec at=s.p+change*cover;
            if (crate>=0) strikeCrate(crates[crate],at);
            else {
                burst(at,s.color,3,77);
                if (hitSurface>=0) audio.event(impactSound(platforms[hitSurface],stage->biome),.32f,
                    (at.x-player.box.x)/width,.92f+random()*.14f);
            }
            s.life=0;
        }
        s.p+=change;
        if (mode!=Mode::active) break;
    }
    std::erase_if(shots,[](const Shot& s){return s.life<=0;});
    std::erase_if(hostile,[](const Shot& s){return s.life<=0;});
}

void Game::blastEffects(Vec at,float radius,Color color) {
    burst(at,color,26,330);
    burst(at,hex(0xfff0c2),12,420);
    burst(at,hex(0xc9ced4),6,120,ParticleKind::puff);
    burst(at,hex(0x6d747c),4,70,ParticleKind::puff);
    burst(at,hex(0x93a5ac),8,210,ParticleKind::chip);
    ring(at,hex(0xfff0c2),radius,.5f);
    ring(at,color,radius*.6f,.35f);
    shake=std::max(shake,8.f); punch=std::max(punch,.6f); flash=std::max(flash,.3f);
    audio.event("destroy",.8f,(at.x-player.box.x)/width,.78f+random()*.1f);
}

void Game::explode(Vec at,float radius,float damage,Color color) {
    blastEffects(at,radius,color);
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
