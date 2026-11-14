#include "game/game.hpp"
#include "game/detail.hpp"

namespace vl {
// Everything an enemy shows when it is hit: the wound, the flinch for the body part that was struck, sparks and
// chips thrown out of the entry and exit points, a floating number, the health bar and the crosshair marker.
void Game::registerHit(Enemy& e,Vec point,Vec direction,float damage,bool crit) {
    const Vec center=e.box.center();
    Vec local=point-center;
    local.x=clamp(local.x,-e.box.w*.5f,e.box.w*.5f);
    local.y=clamp(local.y,-e.box.h*.5f,e.box.h*.5f);
    const Vec dir=length(direction)>.001f?normal(direction):Vec{1,0};
    auto& wound=e.wounds[static_cast<size_t>(e.woundCount)%e.wounds.size()];
    ++e.woundCount;
    wound={local,0,std::atan2(dir.y,dir.x),clamp(damage/45.f,.6f,1.7f)};
    e.hitDir=dir;
    e.zone=clamp(local.y/(e.box.h*.5f),-1.f,1.f);
    e.flinch=1;
    const float push=clamp(damage/60.f,.35f,1.f);
    e.kick=sign(dir.x)*push;
    // Soldiers react by body part: a head hit snaps the head back, a leg hit buckles the knees.
    const bool soldier=e.kind==EnemyKind::ranger;
    e.headSnap=soldier && e.zone<-.3f?1.f:0.f;
    e.buckle=soldier && e.zone>.35f?1.f:0.f;
    e.bar=2.6f;

    const Color spark=crit?hex(0xffe27a):hex(0xffd9a0);
    burstCone(point,dir*-1.f,.9f,spark,crit?8:4,190);
    burstCone(point,dir,.75f,hex(0xffb36a),crit?7:3,260);
    burstCone(point,dir*-1.f,1.1f,hex(0x93a5ac),2+int(push*3),135,ParticleKind::chip);
    if (soldier) burstCone(point,dir*-1.f,.8f,hex(0xcfd4da),1,45,ParticleKind::puff);
    if (damage>=40) ring(point,hex(0xfff0c8),14+push*12,.18f);

    // Floating numbers are merged while the gun is firing quickly, so rapid fire stays readable.
    e.pending+=damage;
    e.pendingCrit=e.pendingCrit || crit;
    if (e.numberTimer<=0) flushDamageNumber(e);
}

void Game::flushDamageNumber(Enemy& e) {
    if (e.pending<=0) return;
    const bool crit=e.pendingCrit;
    const float jitter=(random()-.5f)*22.f;
    message({e.box.center().x+jitter,e.box.y-4},std::to_string(int(std::lround(e.pending))),
            crit?hex(0xffe27a):hex(0xffffff),crit?.95f:.75f);
    messages.back().scale=crit?.95f:.72f;
    e.pending=0; e.pendingCrit=false; e.numberTimer=.3f;
}

// Per-step upkeep of the hit feedback: wounds cool, the health bar trails behind, merged numbers appear and
// a badly damaged machine smokes and throws sparks from its wounds.
void Game::updateEnemyFeedback(Enemy& e,float dt) {
    e.headSnap=std::max(0.f,e.headSnap-dt*4.f);
    e.buckle=std::max(0.f,e.buckle-dt*3.2f);
    e.bar=std::max(0.f,e.bar-dt);
    const float fraction=e.maxHp>0?float(e.hp)/e.maxHp:0.f;
    e.trail=e.trail>fraction?std::max(fraction,e.trail-dt*.55f):fraction;
    e.numberTimer=std::max(0.f,e.numberTimer-dt);
    if (e.pending>0 && e.numberTimer<=0) flushDamageNumber(e);
    const int count=std::min<int>(e.woundCount,static_cast<int>(e.wounds.size()));
    for (int i=0;i<count;++i) e.wounds[static_cast<size_t>(i)].age+=dt;
    if (!e.alive || count==0 || fraction>.45f) return;
    // Below 45% health. The closer to death, the more it smokes.
    const float distress=1-fraction/.45f;
    const auto& w=e.wounds[static_cast<size_t>((e.woundCount-1)%static_cast<int>(e.wounds.size()))];
    if (random()<dt*(4+10*distress)) burst(e.box.center()+w.local,hex(0x59616a),1,26,ParticleKind::puff);
    if (random()<dt*(1.5f+5*distress)) burstCone(e.box.center()+w.local,{0,-1},1.3f,hex(0xffc27a),2,110);
}
} // namespace vl
