#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
float Game::random() { return std::generate_canonical<float,22>(rng); }

void Game::resetTransient() {
    releaseBoth();
    ambient.clear(); skyFlash=0; ambientTimer=1.2f;
    shots.clear(); hostile.clear(); particles.clear(); messages.clear(); rings.clear(); corpses.clear();
    style={}; combo=0; comboTimer=0; flash=0; pain=0; punch=0; hitStop=0; timeScale=1; slowing=slowLocked=false;
    shake=0; entry=2.65f;
}

void Game::start(int sector,const Save& save) {
    coop=false; partner={}; partnerInput={};
    index=sector;
    stage=&stages[index]; campaign=save;
    difficulty=&difficulties[save.difficulty];
    const auto& rank=save.perks;
    speedScale=1+.04f*rank[1]; jumpScale=1+.03f*rank[1];
    slowDrain=.16f/(1+.25f*rank[2]); slowRegen=.045f*(1+.25f*rank[2]);
    pickupAmmo=.41f*(1+.20f*rank[3]); pickupHeal=42*(1+.15f*rank[4]);
    damageTaken=1-.06f*rank[5];
    platforms.clear();
    for (const auto& box:stage->ground) platforms.push_back({box,{},true});
    for (const auto& box:stage->ledges) platforms.push_back({box,{}});
    for (const auto& lift:stage->lifts) platforms.push_back({{lift.x,lift.y,lift.w,15},{},false,&lift});
    for (const auto& box:stage->cover) platforms.push_back({box,{},false,nullptr,true});
    crates.clear();
    for (const auto& placement:stage->crates) {
        const Vec base=placement.base;
        const auto& spec=crateSpec(placement.kind);
        crates.push_back({placement.kind,{base.x-spec.w*.5f,base.y-spec.h,spec.w,spec.h},spec.hits});
    }
    enemies.clear(); supplies.clear();
    const int later=std::max(0,index-4);
    for (const auto& item:stage->enemies) {
        const auto& s=spec(item.kind);
        const bool boss=item.kind==EnemyKind::boss;
        const int id=static_cast<int>(enemies.size());
        const float scaling=1+std::min(index,4)*(boss?.27f:.085f)+later*(boss?.11f:.042f);
        const int hp=std::lround(s.hp*scaling*(item.elite?1.45f:1.f)*difficulty->hp);
        const Vec origin{item.x-s.w*.5f,item.y-(item.kind==EnemyKind::drone?0:s.h)};
        enemies.push_back({item.kind,{origin.x,origin.y,s.w,s.h},origin,item.y,
            item.patrol>0?item.patrol:(boss?160.f:0.f),float((id*97+index*47)%239),
            .66f+(id%4)*.23f,s.damage*difficulty->damage*(1+std::min(index,4)*.045f+later*.025f)
                *(item.elite?1.18f:1.f),s.reach+(item.elite?35.f:0.f),
            s.speed*(item.elite?1.25f:1.f),hp,hp,id%2?-1:1});
        enemies.back().elite=item.elite;
    }
    for (const auto& item:stage->supplies) supplies.push_back({item.kind,{item.x,item.y-20}});
    setupLoadout(save);
    resetTransient(); backup={}; slowEnergy=1; bestCombo=0;
    score=0; kills=0; time=0; camera=0;
    activeBeacon=-1; bossActive=bossDead=false;
    spawn({100,stage->ground.front().y});
    moveLifts(0);
    mode=Mode::active;
}

void Game::setupLoadout(const Save& save) {
    character=&characters[save.character];
    maxHealth=int(std::lround(character->health*(1+.10f*save.perks[0])));
    gun=save.equipped;
    for (size_t i=0;i<guns.size();++i) {
        const auto& w=weapons[i];
        const auto& t=save.tiers[i];
        const int capacity=std::lround(w.magazine*(1+t[1]*.25f));
        const int reserve=w.reserve<0?-1:std::lround(w.reserve*(1+t[3]*.30f)*character->ammo);
        guns[i]={int(std::lround(w.damage*(1+t[0]*.20f))),capacity,reserve,capacity,reserve,
                 w.reload*(1-t[2]*.15f),0,save.cleared>=w.unlock,
                 w.range*(1+t[4]*.14f),w.interval*(1-t[5]*.08f),w.spread*(1-t[6]*.22f),t[7]*.07f};
    }
}

void Game::spawn(Vec safe) {
    player={};
    player.box={safe.x,safe.y-playerH,playerW,playerH};
    player.hp=maxHealth; player.safe=safe;
    for (size_t i=0;i<platforms.size();++i) {
        const auto& box=platforms[i].box;
        if (std::abs(box.y-safe.y)<.5f && box.x<safe.x+playerW && box.right()>safe.x) {
            player.support=static_cast<int>(i); break;
        }
    }
    resetPose();
}

void Game::resetPose() {
    auto& p=player;
    p.gait={};
    for (int i=0;i<2;++i) {
        auto& f=p.gait.feet[i];
        f.heel={p.box.center().x+(i==0?-6.f:5.f),p.box.bottom()};
        f.takeoff=f.heel;
        f.phase=.5f*i;
        f.surface=p.support;
        if (f.surface>=0) f.anchor=f.heel.x-platforms[f.surface].box.x;
    }
}

void Game::retry() {
    if (activeBeacon<0) {
        const auto save=campaign;
        if (coop) startCoop(index,save);
        else start(index,save);
        return;
    }
    resetTransient();
    time=backup.time; score=backup.score; kills=backup.kills; gun=backup.gun;
    guns=backup.guns; for (auto& g:guns) g.reload=0;
    enemies=backup.enemies; supplies=backup.supplies; crates=backup.crates;
    bossActive=backup.bossActive; bossDead=backup.bossDead;
    player.support=-1;
    moveLifts(0);
    const Vec beacon=stage->beacons[activeBeacon];
    spawn(beacon-Vec{playerW*.5f,0});
    camera=clamp(beacon.x-width*.32f,0,stage->worldWidth-width);
    player.flash=.9f; entry=0; mode=Mode::active;
    if (coop) {
        partner=backup.partner;
        for (auto& g:partner.guns) g.reload=0;
        spawnPartner(player.safe);
    }
}

void Game::pause(bool paused) { release(); mode=paused?Mode::paused:Mode::active; }

void Game::release() { keys.fill(false); pointerActive=false; stickActive=false; }

void Game::input(Key key,bool down) {
    auto& state=keys[static_cast<size_t>(key)];
    if (!down) { state=false; return; }
    if (state || mode!=Mode::active) return;
    state=true;
    if (key==Key::jump) player.jumpBuffer=.135f;
    if (key==Key::reload) reload();
    if (key==Key::fire || key==Key::automatic) player.fireBuffer=.115f;
}

float Game::advance(float dt) {
    if (mode!=Mode::active) { timeScale=1; slowing=false; return 1; }
    // Slow motion drains a reserve in real time. A drained reserve must refill a little before it re-engages.
    if (slowLocked && slowEnergy>.22f) slowLocked=false;
    const bool wants=(held(Key::slow) || (coop && partnerInput.keys[static_cast<size_t>(Key::slow)]))
        && !slowLocked && slowEnergy>0;
    if (wants && !slowing) audio.event("switch",.5f,0,.62f);
    if (!wants && slowing) audio.event("switch",.4f,0,1.25f);
    slowing=wants;
    if (slowing) {
        slowEnergy=std::max(0.f,slowEnergy-dt*slowDrain);
        if (slowEnergy==0) { slowing=false; slowLocked=true; }
    } else slowEnergy=std::min(1.f,slowEnergy+dt*slowRegen);
    const float target=slowing?.28f:1.f;
    timeScale=approach(timeScale,target,dt*(target<timeScale?12.f:4.5f));
    hitStop=std::max(0.f,hitStop-dt);
    punch=std::max(0.f,punch-dt*2.6f);
    flash=std::max(0.f,flash-dt*3.4f); pain=std::max(0.f,pain-dt*2.2f);
    return hitStop>0?.04f:timeScale;
}

void Game::select(int weapon) {
    if (mode!=Mode::active || gun==weapon || !guns.at(weapon).available) return;
    gun=weapon;
    player.fireDelay=std::max(.14f,player.fireDelay);
    audio.event("switch");
}

void Game::tick(float dt) {
    if (mode!=Mode::active) return;
    time+=dt; entry=std::max(0.f,entry-dt); shake=std::max(0.f,shake-36*dt);
    moveLifts(dt); movePlayer(dt);
    if (mode!=Mode::active) return;
    shoot(dt);
    if (coop) {
        swapOperatives();
        movePlayer(dt);
        if (mode==Mode::active) shoot(dt);
        updateCamera(dt);
        swapOperatives();
    }
    if (mode!=Mode::active) return;
    moveEnemies(dt); moveShots(dt); detonateCrates();
    if (mode!=Mode::active) return;
    triggers();
    if (coop && mode==Mode::active) {
        swapOperatives();
        triggers();
        swapOperatives();
    }
    if (mode==Mode::active) checkpoints();
    updateParticles(dt); updateCorpses(dt); updateAmbient(dt);
    updateCamera(dt);
    if (mode==Mode::active && bossDead && atExtraction(player) && (!coop || atExtraction(partner.player))) {
        score+=std::lround((60+index*18)*difficulty->score);
        releaseBoth(); mode=Mode::complete;
    }
}
} // namespace vl
