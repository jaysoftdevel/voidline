#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
void Game::damageEnemy(Enemy& e,float damage,Vec point,Vec direction,bool crit) {
    if (!e.alive) return;
    e.hp-=std::max(1,static_cast<int>(damage));
    e.hit=.094f;
    registerHit(e,point,direction,damage,crit);
    hitMarker=.2f;
    if (crit) {
        ring(e.box.center(),hex(0xffe27a),26,.22f);
        hitStop=std::max(hitStop,.035f);
    }
    if (e.hp<=0) { e.hp=0; destroy(e); }
}

void Game::award(Enemy& e,int base) {
    const bool boss=e.kind==EnemyKind::boss;
    const Vec top{e.box.center().x,e.box.y-6};
    combo=comboTimer>0?combo+1:1;
    comboTimer=3.2f; bestCombo=std::max(bestCombo,combo);
    const float multiplier=std::min(3.f,1.f+(combo-1)*.25f);
    const bool airborne=player.support<0, flipping=player.flip>0;
    int bonus=0;
    std::string tag;
    if (flipping) { bonus+=12; tag="FLIP SHOT"; }
    else if (airborne) { bonus+=6; tag="AIRBORNE"; }
    if (slowing) { bonus+=8; tag=tag.empty()?"BULLET TIME":tag+" + SLOW"; }
    const int points=std::lround((base+bonus)*multiplier*difficulty->score);
    score+=points;
    message(top,"+"+std::to_string(points),hex(stage->accent));
    static constexpr const char* names[]{"","","DOUBLE","TRIPLE","QUAD","PENTA","RAMPAGE"};
    std::string title=combo>=2?names[std::min(combo,6)]:"";
    if (combo>=2) title+=" KILL  x"+std::to_string(combo);
    if (!tag.empty()) title=title.empty()?tag:title+"  /  "+tag;
    if (!title.empty()) {
        const Color tint=combo>=5?hex(0xff5f7e):combo>=3?hex(0xffd35c):hex(0x7ff2ff);
        style={title,tint,1.7f,1.7f,combo};
    }
    slowEnergy=std::min(1.f,slowEnergy+(boss?.5f:.07f));
    hitStop=std::max(hitStop,boss?.16f:.05f);
    punch=std::max(punch,boss?1.f:.38f);
    flash=std::max(flash,boss?.7f:.22f);
}

// The kill itself. The body dies at once for the rules (score, extraction), while the corpse plays out its own
// animation in game/death_*.cpp. The burst here is the instant of death.
void Game::destroy(Enemy& e) {
    e.alive=false; ++kills;
    killMarker=.45f;
    flushDamageNumber(e);
    e.bar=0;
    const bool boss=e.kind==EnemyKind::boss;
    e.fallDir=static_cast<float>(sign(e.box.center().x-player.box.center().x));
    bury(e);
    award(e,spec(e.kind).score+(boss?index*22:0));
    const Color accent=hex(stage->accent);
    const Vec at=e.box.center();
    switch (e.kind) {
    case EnemyKind::ranger:
        burstCone(at,e.hitDir,.7f,accent,10,170);
        burst(at,hex(0xfff0b8),5,150);
        burst(at,hex(0x98b4c7),5,70,ParticleKind::puff);
        burst(at,hex(0x93a5ac),6,125,ParticleKind::chip);
        ring(at,hex(0xfff0b8),44,.3f);
        break;
    case EnemyKind::stalker:
        burst(at,accent,12,170);
        burst(at,hex(0xfff0b8),7,230);
        burst(at,hex(0x93a5ac),8,150,ParticleKind::chip);
        ring(at,hex(0xfff0b8),50,.32f);
        break;
    default:
        burst(at,accent,boss?46:16,boss?315:158);
        burst(at,hex(0xfff0b8),boss?20:8,boss?380:230);
        burst(at,hex(0x98b4c7),boss?26:9,boss?220:104,ParticleKind::smoke);
        burst(at,hex(0x93a5ac),boss?24:7,boss?235:135,ParticleKind::chip);
        ring(at,hex(0xfff0b8),boss?150:62,boss?.7f:.4f);
        ring(at,accent,boss?96:38,boss?.55f:.3f);
        break;
    }
    shake=std::max(shake,boss?15.f:e.kind==EnemyKind::ranger?3.f:4.2f);
    if (e.kind==EnemyKind::ranger) {
        audio.event("hurt",.55f,(at.x-player.box.x)/width,.55f+random()*.12f);
        audio.event("destroy",.35f,(at.x-player.box.x)/width,1.1f+random()*.1f);
    } else {
        audio.event(boss?"guardian":"destroy",boss?1.f:.62f,(at.x-player.box.x)/width,.94f+random()*.16f);
    }
    if (boss) {
        bossDead=true; bossActive=false; hostile.clear();
        message(at-Vec{0,49},"EXTRACTION UNLOCKED",hex(0xb4f5ca),2);
        audio.event("checkpoint");
    }
}

void Game::hurt(float damage,bool force) {
    auto& p=player;
    if (mode!=Mode::active || (p.flash>0 && !force)) return;
    p.hp=std::max(0,p.hp-int(std::ceil(damage*damageTaken))); p.flash=.69f;
    burst({p.box.center().x,p.box.y+20},hex(0xff795d),10,132);
    shake=std::max(shake,8.7f); audio.event("hurt");
    hitStop=std::max(hitStop,.07f); punch=std::max(punch,.6f); pain=1; combo=0; comboTimer=0;
    ring({p.box.center().x,p.box.y+20},hex(0xff795d),52,.36f);
    if (!p.hp) { release(); mode=Mode::failed; }
}
} // namespace vl
