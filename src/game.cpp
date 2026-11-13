#include "game.hpp"
#include <limits>

namespace vl {
namespace {
constexpr float infinity=std::numeric_limits<float>::infinity();
constexpr float gravity=1510, playerW=32, playerH=56, crouchHeight=39;
constexpr float windupDuration=.032f;
constexpr const char* gunSounds[]{"carbine","shotgun","arc","pistol"};
constexpr const char* footSounds[]{"step-rock","step-grate","step-rock","step-deck","step-deck"};
struct EnemySpec { float w,h,speed,reach,delay; int hp,damage,score; };
constexpr EnemySpec specs[]{
    {48,30,78,390,1.65f,57,10,14}, {35,54,72,435,1.34f,82,13,20},
    {48,46,0,490,1.52f,102,14,27}, {54,37,176,330,1.65f,93,17,25},
    {108,97,67,600,1.30f,420,16,135}
};
struct Pattern { int fans; float spread,velocity,interval; uint32_t color; };
constexpr Pattern patterns[]{
    {3,.33f,320,1.29f,0xffa55e}, {5,.64f,295,1.48f,0x7ef1e9}, {3,.39f,352,1.09f,0xbaff9d},
    {5,.73f,378,1.27f,0xff905d}, {5,.58f,440,1.17f,0xa9c9ff}
};
const EnemySpec& spec(EnemyKind kind) { return specs[static_cast<int>(kind)]; }

float segment(Vec p,Vec d,Rect r) {
    float near=0,far=1;
    const float origins[]{p.x,p.y}, deltas[]{d.x,d.y};
    const float lows[]{r.x,r.y}, highs[]{r.right(),r.bottom()};
    for (int axis=0;axis<2;++axis) {
        if (std::abs(deltas[axis])<.0001f) {
            if (origins[axis]<lows[axis] || origins[axis]>highs[axis]) return infinity;
            continue;
        }
        const float a=(lows[axis]-origins[axis])/deltas[axis];
        const float b=(highs[axis]-origins[axis])/deltas[axis];
        near=std::max(near,std::min(a,b));
        far=std::min(far,std::max(a,b));
        if (near>far) return infinity;
    }
    return near;
}
} // namespace

Vec solveKnee(Vec hip,Vec ankle,float thigh,float shin,int forward) {
    Vec delta=ankle-hip;
    const float d=clamp(length(delta),std::abs(thigh-shin)+.01f,thigh+shin-.01f);
    const Vec along=length(delta)>.001f ? normal(delta) : Vec{0,1};
    const float projection=(thigh*thigh-shin*shin+d*d)/(2*d);
    const float bend=std::sqrt(std::max(0.f,thigh*thigh-projection*projection));
    return hip+along*projection+Vec{along.y,-along.x}*(bend*forward);
}

float Game::random() { return std::generate_canonical<float,22>(rng); }
void Game::resetTransient() {
    release();
    shots.clear(); hostile.clear(); particles.clear(); messages.clear(); rings.clear(); corpses.clear();
    style={}; combo=0; comboTimer=0; flash=0; pain=0; punch=0; hitStop=0; timeScale=1; slowing=slowLocked=false;
    shake=0; entry=2.65f;
}
void Game::start(int sector,const Save& save) {
    index=sector;
    stage=&stages[index]; campaign=save;
    character=&characters[save.character]; difficulty=&difficulties[save.difficulty];
    platforms.clear();
    for (const auto& box:stage->ground) platforms.push_back({box,{},true});
    for (const auto& box:stage->ledges) platforms.push_back({box,{}});
    for (const auto& lift:stage->lifts) platforms.push_back({{lift.x,lift.y,lift.w,15},{},false,&lift});
    enemies.clear(); supplies.clear();
    for (const auto& item:stage->enemies) {
        const auto& s=spec(item.kind);
        const bool boss=item.kind==EnemyKind::boss;
        const int id=static_cast<int>(enemies.size());
        const int hp=std::lround(s.hp*(1+index*(boss?.27f:.085f))*difficulty->hp);
        const Vec origin{item.x-s.w*.5f,item.y-(item.kind==EnemyKind::drone?0:s.h)};
        enemies.push_back({item.kind,{origin.x,origin.y,s.w,s.h},origin,item.y,
            item.patrol>0?item.patrol:(boss?160.f:0.f),float((id*97+index*47)%239),
            .66f+(id%4)*.23f,s.damage*difficulty->damage*(1+index*.045f),s.reach,s.speed,hp,hp,id%2?-1:1});
    }
    for (const auto& item:stage->supplies) supplies.push_back({item.kind,{item.x,item.y-20}});
    for (size_t i=0;i<guns.size();++i) {
        const auto& w=weapons[i];
        const std::array<int,4> tiers=i<save.tiers.size()?save.tiers[i]:std::array<int,4>{};
        const int capacity=std::lround(w.magazine*(1+tiers[1]*.25f));
        const int reserve=w.reserve<0?-1:std::lround(w.reserve*(1+tiers[3]*.30f)*character->ammo);
        guns[i]={int(std::lround(w.damage*(1+tiers[0]*.20f))),capacity,reserve,capacity,reserve,
                 w.reload*(1-tiers[2]*.15f),0,save.cleared>=w.unlock};
    }
    resetTransient(); backup={}; slowEnergy=1; bestCombo=0;
    gun=save.equipped; score=0; kills=0; time=0; camera=0;
    checkpoint=bossActive=bossDead=false;
    spawn({100,stage->ground.front().y});
    moveLifts(0);
    mode=Mode::active;
}
void Game::spawn(Vec safe) {
    player={};
    player.box={safe.x,safe.y-playerH,playerW,playerH};
    player.hp=character->health; player.safe=safe;
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
    if (!checkpoint) { start(index,campaign); return; }
    resetTransient();
    time=backup.time; score=backup.score; kills=backup.kills; gun=backup.gun;
    guns=backup.guns; for (auto& g:guns) g.reload=0;
    enemies=backup.enemies; supplies=backup.supplies;
    bossActive=backup.bossActive; bossDead=backup.bossDead;
    player.support=-1;
    moveLifts(0);
    spawn(stage->beacon+Vec{14,0});
    camera=clamp(stage->beacon.x-width*.32f,0,stage->worldWidth-width);
    player.flash=.9f; entry=0; mode=Mode::active;
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
    const bool wants=held(Key::slow) && !slowLocked && slowEnergy>0;
    if (wants && !slowing) audio.event("switch",.5f,0,.62f);
    if (!wants && slowing) audio.event("switch",.4f,0,1.25f);
    slowing=wants;
    if (slowing) {
        slowEnergy=std::max(0.f,slowEnergy-dt*.16f);
        if (slowEnergy==0) { slowing=false; slowLocked=true; }
    } else slowEnergy=std::min(1.f,slowEnergy+dt*.045f);
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
    shoot(dt); moveEnemies(dt); moveShots(dt);
    if (mode!=Mode::active) return;
    triggers(); updateParticles(dt); updateCorpses(dt);
    const float target=clamp(player.box.x-width*.34f+player.velocity.x*.28f,0,stage->worldWidth-width);
    camera=mix(camera,target,1-std::exp(-4.9f*dt));
    if (mode==Mode::active && bossDead && std::abs(player.box.center().x-stage->exit.x)<48 &&
        std::abs(player.box.bottom()-stage->exit.y)<45) {
        score+=std::lround((60+index*18)*difficulty->score);
        release(); mode=Mode::complete;
    }
}
void Game::moveLifts(float dt) {
    for (size_t i=0;i<platforms.size();++i) {
        auto& p=platforms[i];
        if (!p.lift) continue;
        const auto& l=*p.lift;
        const float phase=time*2*pi/l.period+l.phase;
        const Vec offset{l.x+std::sin(phase)*l.dx-p.box.x,l.y+std::sin(phase)*l.dy-p.box.y};
        p.velocity={std::cos(phase)*l.dx*2*pi/l.period,std::cos(phase)*l.dy*2*pi/l.period};
        p.box.x+=offset.x; p.box.y+=offset.y;
        if (dt>0 && player.support==static_cast<int>(i)) {
            player.box.x+=offset.x; player.box.y+=offset.y;
        }
    }
}
bool Game::headroom(float x,float feet) const {
    const Rect upper{x+3,feet-playerH,playerW-6,playerH-crouchHeight-.5f};
    for (const auto& platform:platforms) {
        if (upper.overlaps(platform.box)) return false;
    }
    return true;
}
void Game::jumpFrom(int support) {
    auto& p=player;
    const Vec lift=support>=0?platforms[support].velocity:Vec{};
    p.jumpBuffer=0; p.coyote=0; p.jumpAge=.001f; p.windup=0;
    p.velocity.y=-character->jump+clamp(lift.y,-45,45);
    p.velocity.x+=clamp(lift.x,-75,75);
    p.support=-1;
    const auto& feet=p.gait.feet;
    const int push=(feet[0].heel.x-feet[1].heel.x)*p.gait.direction<0?0:1;
    p.gait.launchFoot=float(push);
    audio.event("jump",p.sprinting?.83f:.69f,0,.94f+campaign.character*.05f);
    burst(feet[push].heel,hex(stage->cold),p.sprinting?10:6,75,ParticleKind::smoke);
}
void Game::movePlayer(float dt) {
    auto& p=player;
    const int direction=int(held(Key::right))-int(held(Key::left));
    const int priorSupport=p.support;
    const bool grounded=priorSupport>=0;
    const bool wantsCrouch=held(Key::crouch);
    const bool shouldCrouch=(grounded && wantsCrouch) ||
        (p.crouched && (wantsCrouch || !headroom(p.box.x,p.box.bottom())));
    if (p.crouched!=shouldCrouch) {
        const float feet=p.box.bottom();
        p.crouched=shouldCrouch;
        p.box.h=p.crouched?crouchHeight:playerH;
        p.box.y=feet-p.box.h;
        if (grounded) audio.event("cloth",p.crouched?.58f:.42f,0,p.crouched?.91f:1.09f);
    }
    const float oldX=p.box.x, oldV=p.velocity.x;
    const bool reversing=direction && p.velocity.x*direction<0;
    const float pace=clamp(std::abs(p.velocity.x)/character->speed);
    if (grounded) {
        p.sprinting=!p.crouched && held(Key::sprint) && direction;
        p.sprintDirection=p.sprinting?direction:0;
    }
    if (p.crouched) p.sprinting=false;
    const float speed=character->speed*(p.crouched?.40f:p.sprinting?1.28f:1.f);
    const float acceleration=grounded
        ? (reversing?3380.f:p.crouched && std::abs(p.velocity.x)>speed?3200.f:direction?mix(2700.f,1730.f,pace):3000.f)
        : (direction?1320.f:105.f);
    float targetSpeed=direction*speed;
    if (!grounded && p.sprinting && direction!=p.sprintDirection) targetSpeed=direction*character->speed;
    p.velocity.x=approach(p.velocity.x,targetSpeed,acceleration*dt);
    const float edge=bossDead?stage->worldWidth-playerW:stage->exit.x-106;
    const float nextX=p.box.x+p.velocity.x*dt;
    p.box.x=clamp(nextX,12,edge);
    if (p.box.x!=nextX) p.velocity.x=0;
    for (const auto& surface:platforms) {
        const auto& s=surface.box;
        // Ledges remain one-way during jumps. Grounded actors need clearance to walk beneath them.
        const bool lowCeiling=grounded && p.box.bottom()>=s.bottom()-.5f;
        if ((!surface.ground && !lowCeiling) || p.box.bottom()<=s.y+.5f || p.box.y>=s.bottom()) continue;
        if (oldX+playerW<=s.x && p.box.right()>s.x) {
            p.box.x=s.x-playerW; p.velocity.x=0;
        } else if (oldX>=s.right() && p.box.x<s.right()) {
            p.box.x=s.right(); p.velocity.x=0;
        }
    }

    for (float* timer:{&p.flash,&p.muzzle,&p.fireDelay,&p.fireBuffer,&p.jumpBuffer})
        *timer=std::max(0.f,*timer-dt);
    p.coyote=grounded?.095f:std::max(0.f,p.coyote-dt);
    if (p.crouched) { p.windup=0; p.jumpBuffer=0; }
    if (p.windup>0) {
        p.windup-=dt;
        if (p.windup<=0 && (grounded || p.coyote>0)) jumpFrom(priorSupport);
    } else if (p.jumpBuffer>0 && p.coyote>0 && !p.crouched) {
        if (grounded) { p.windup=windupDuration; p.jumpBuffer=0; }
        else jumpFrom(-1);
    } else if (p.jumpBuffer>0 && !grounded && !p.airJumpUsed && !p.crouched && p.windup<=0) {
        // One acrobatic flip per airtime, a little weaker than the ground jump.
        p.airJumpUsed=true; p.jumpBuffer=0; p.jumpAge=.001f;
        p.velocity.y=-character->jump*.82f;
        p.flip=flipSeconds; p.flipDirection=p.facing>=0?1.f:-1.f;
        if (direction) p.velocity.x=approach(p.velocity.x,direction*character->speed*1.08f,160);
        audio.event("jump",.8f,0,1.22f);
        burst({p.box.center().x,p.box.bottom()-6},hex(stage->cold),9,92,ParticleKind::smoke);
        ring({p.box.center().x,p.box.bottom()-4},hex(stage->cold),34,.32f);
        message({p.box.center().x,p.box.y-14},"FLIP",hex(0xffe27a),.55f);
    }
    p.flip=std::max(0.f,p.flip-dt);
    p.turn=approach(p.turn,float(p.facing),dt*9.5f);
    p.landed=std::max(0.f,p.landed-dt*4.f);
    if (p.jumpAge>0) p.jumpAge+=dt;
    const bool rising=p.velocity.y<0;
    const bool releasedEarly=p.jumpAge>.13f && !held(Key::jump) && p.velocity.y<-80;
    const float weight=rising?(releasedEarly?2.65f:1.f):1.13f;
    const float feet=p.box.bottom();
    const int attached=p.support;
    p.velocity.y=std::min(1075.f,p.velocity.y+gravity*weight*dt);
    p.box.y+=p.velocity.y*dt;
    p.support=-1;
    float landing=0;
    if (p.velocity.y>=0) {
        for (size_t i=0;i<platforms.size();++i) {
            const auto& platform=platforms[i];
            const auto& s=platform.box;
            const Vec surfaceMotion=static_cast<int>(i)==attached?Vec{}:platform.velocity*dt;
            const float previousSurface=s.y-surfaceMotion.y;
            if (feet>previousSurface+2 || p.box.bottom()<s.y) continue;
            const float t=clamp((previousSurface-feet)/std::max(.001f,p.box.bottom()-feet-surfaceMotion.y));
            const float x=mix(oldX,p.box.x,t)+surfaceMotion.x*(1-t);
            if (x+playerW-5<=s.x || x+5>=s.right()) continue;
            // Once attached, the final footprint must still overlap that surface.
            if (p.box.right()-5<=s.x || p.box.x+5>=s.right()) continue;
            if (p.support<0 || s.y<platforms[p.support].box.y) p.support=static_cast<int>(i);
        }
        if (p.support>=0) {
            const auto& surface=platforms[p.support];
            const auto& s=surface.box;
            const float impact=p.velocity.y-surface.velocity.y;
            if (attached<0 && impact>185) {
                landing=clamp((impact-105)/525,.16f,1);
                p.landed=landing;
                const char* surfaceSound=surface.ground?footSounds[index]:"step-grate";
                audio.event("land",.30f+landing*.66f,0,1.05f-landing*.15f+random()*.06f);
                audio.event(surfaceSound,.26f+landing*.37f,0,.75f+random()*.11f);
                burst({p.box.center().x,s.y},hex(stage->cold),5+int(landing*9),62+landing*54,ParticleKind::smoke);
            }
            p.velocity.y=0; p.box.y=s.y-p.box.h; p.jumpAge=0; p.airJumpUsed=false; p.flip=0;
            if (surface.ground && p.box.x>s.x+40 && p.box.x<s.right()-68) p.safe={p.box.x,s.y};
        }
    }
    updateGait(dt,p.box.x-oldX,(p.velocity.x-oldV)/dt,attached,landing);
    if (p.box.y>height+95) {
        hurt(26,true);
        if (mode!=Mode::active) return;
        const int hp=p.hp;
        spawn(p.safe);
        p.hp=hp; p.flash=.69f;
        std::erase_if(hostile,[&](const Shot& s){return std::abs(s.p.x-p.box.x)<160;});
        message({p.box.center().x,p.box.y-12},"FALL RECOVERY",hex(0xffad6a));
    }
    p.aiming=held(Key::fire) || held(Key::automatic) || p.fireBuffer>0 || p.muzzle>0;
    aimPlayer(direction);
}

void Game::updateGait(float dt,float travel,float accel,int priorSupport,float landing) {
    auto& p=player;
    auto& g=p.gait;
    const float speed=std::abs(p.velocity.x), pace=clamp(speed/character->speed,0,1.28f);
    const bool onGround=p.support>=0;
    const float center=p.box.center().x, floor=p.box.bottom();
    g.crouch=mix(g.crouch,p.crouched?1.f:0.f,1-std::exp(-(p.crouched?29:24)*dt));
    g.run=mix(g.run,smooth((pace-.29f)/.88f)*(1-g.crouch),1-std::exp(-11*dt));
    const float tuckTarget=onGround?0.f:smooth((p.jumpAge-.055f)/.14f)*(1-smooth((p.velocity.y-45)/610));
    g.tuck=mix(g.tuck,tuckTarget,1-std::exp(-14*dt));
    g.reach=mix(g.reach,onGround?0.f:smooth(p.velocity.y/510),1-std::exp(-13*dt));
    const float anticip=p.windup>0?3.4f*(1-p.windup/windupDuration):0.f;
    const float bob=std::cos(g.cycle*4*pi-.4f)*g.weight*(.9f+g.run*1.2f);
    g.pelvis=mix(g.pelvis,bob+anticip,1-std::exp(-28*dt));
    g.weight=mix(g.weight,onGround?smooth(speed/65):0.f,1-std::exp(-14*dt));
    g.lean=mix(g.lean,clamp(p.velocity.x/character->speed*(.15f+g.run*.11f)+accel*.000061f,-.39f,.39f),1-std::exp(-11*dt));
    if (landing>0) g.compressVelocity+=landing*124;
    g.compressVelocity+=(-161*g.compression-20*g.compressVelocity)*dt;
    g.compression=std::max(0.f,g.compression+g.compressVelocity*dt);
    if (g.compression==0 && g.compressVelocity<0) g.compressVelocity=0;

    const int oldDirection=g.direction;
    if (speed>25) g.direction=sign(p.velocity.x);
    const bool changedSurface=p.support!=priorSupport;
    if (onGround && changedSurface) {
        // Lead with the foot nearest the surface. Swing the other into the next step.
        const int lead=g.feet[0].heel.y>=g.feet[1].heel.y?0:1;
        g.cycle=lead==0?0.f:.5f;
    }
    if (onGround && oldDirection!=g.direction) {
        const int lead=g.feet[0].planted?0:1;
        g.cycle=lead==0?.18f:.68f;
    }
    const float stance=mix(mix(.62f,.39f,g.run),.69f,g.crouch);
    const float cycleLength=mix(mix(47.f,118.f,g.run),42.f,g.crouch);
    if (onGround) g.cycle=std::fmod(g.cycle+std::abs(travel)/cycleLength,1.f);

    for (int i=0;i<2;++i) {
        auto& foot=g.feet[i];
        const float phase=std::fmod(g.cycle+.5f*i,1.f);
        if (!onGround) {
            foot.surface=-1; foot.planted=false;
            const bool trail=i==int(g.launchFoot);
            const float ahead=trail?mix(-13.f,-6.f,g.reach):mix(16.f,11.f,g.reach);
            const float rise=(trail?6.f:13.f)*g.tuck+(trail?7.f:2.f)*(1-g.reach);
            const Vec target{center+g.direction*ahead,floor-rise};
            foot.heel=mix(foot.heel,target,1-std::exp(-dt*(13+15*g.reach)));
            const float extension=trail?-.47f:.26f;
            foot.angle=mix(foot.angle,extension*(1-g.reach)+.13f*g.reach,1-std::exp(-15*dt));
        } else {
            const auto& surface=platforms[p.support].box;
            const float rest=center+(i==0?-5.f:5.f);
            const float reach=cycleLength*stance*.46f*g.weight;
            const float front=clamp(center+reach*g.direction,surface.x+2,surface.right()-2);
            if (speed<13) {
                if (foot.surface!=p.support || !foot.planted || std::abs(foot.heel.x-rest)>12) {
                    foot.heel=mix(foot.heel,{clamp(rest,surface.x+2,surface.right()-2),floor},1-std::exp(-19*dt));
                    foot.anchor=foot.heel.x-surface.x;
                } else {
                    foot.heel={surface.x+foot.anchor,floor};
                }
                foot.surface=p.support; foot.planted=true;
                foot.angle=approach(foot.angle,0,dt*8);
            } else if (phase<stance) {
                const bool touchdown=!foot.planted || phase<foot.phase || foot.surface!=p.support;
                if (touchdown) {
                    const float projected=changedSurface?foot.heel.x:front;
                    foot.anchor=clamp(clamp(projected,center-24,center+24),surface.x+2,surface.right()-2)-surface.x;
                    if (!changedSurface && phase<.13f && speed>40) {
                        const auto& s=platforms[p.support];
                        audio.event(s.ground?footSounds[index]:"step-grate",g.crouch>.6f?.28f:.31f+pace*.40f,0,.95f+random()*.10f+campaign.character*.022f);
                        if (pace>.52f) burst({surface.x+foot.anchor,floor},hex(stage->cold),2,32,ParticleKind::smoke);
                    }
                }
                foot.heel={surface.x+foot.anchor,floor};
                const float u=phase/stance;
                foot.angle=.29f*(1-smooth(u/.20f))-.40f*smooth((u-.72f)/.28f);
                foot.planted=true; foot.surface=p.support;
            } else {
                if (foot.planted || changedSurface || oldDirection!=g.direction) {
                    foot.takeoff=foot.heel-Vec{surface.x,surface.y};
                }
                const float u=clamp((phase-stance)/(1-stance));
                const Vec start=Vec{surface.x,surface.y}+foot.takeoff;
                foot.heel={mix(start.x,front,smooth(u)),floor-std::sin(pi*u)*mix(mix(8.f,19.f,g.run),6.f,g.crouch)*g.weight};
                foot.angle=mix(-.40f,.29f,smooth(u));
                foot.planted=false; foot.surface=p.support;
            }
        }
        foot.phase=phase;
    }
}
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
        const Vec d=mouse+Vec{camera,0}-origin;
        if (std::abs(d.x)>4) p.facing=sign(d.x);
        p.aim=std::atan2(d.y,d.x); return;
    }
    if (direction) p.facing=direction;
    float best=580;
    p.aim=p.facing>0?-.025f:pi+.025f;
    for (const auto& enemy:enemies) {
        if (!enemy.alive) continue;
        const Vec target{enemy.box.center().x,enemy.box.y+enemy.box.h*.48f}, d=target-origin;
        const float distance=std::hypot(d.x,d.y*1.35f)*(sign(d.x)==p.facing?1.f:1.45f);
        if (distance>=best || blocked(origin,target)) continue;
        best=distance; p.aim=std::atan2(d.y,d.x);
    }
    p.facing=std::cos(p.aim)<0?-1:1;
}
Vec Game::weaponPivot() const { return {player.box.center().x,player.box.bottom()-mix(29.f,17.f,player.gait.crouch)}; }
Vec Game::muzzle() const { return weaponPivot()+ray(player.aim)*muzzleReach[gun]; }
void Game::reload() {
    auto& g=guns[gun];
    if (g.reload>0 || g.ammo>=g.capacity || g.reserve==0) return;
    g.reload=g.duration;
    audio.event("reload",1,0,clamp(.73f/g.duration,.58f,1.65f));
}
void Game::shoot(float dt) {
    for (auto& g:guns) {
        if (g.reload<=0) continue;
        g.reload=std::max(0.f,g.reload-dt);
        if (g.reload>0) continue;
        const int refill=g.reserve<0?g.capacity-g.ammo:std::min(g.capacity-g.ammo,g.reserve);
        g.ammo+=refill;
        if (g.reserve>=0) g.reserve-=refill;
    }
    auto& p=player;
    if ((!held(Key::fire) && !held(Key::automatic) && p.fireBuffer<=0) || p.fireDelay>0) return;
    p.fireBuffer=0;
    auto& g=guns[gun];
    if (g.reload>0) return;
    if (!g.ammo) {
        reload();
        if (!g.reload) { p.fireDelay=.32f; audio.event("empty"); }
        return;
    }
    const auto& w=weapons[gun];
    --g.ammo; p.fireDelay=w.interval; p.muzzle=.069f;
    const Vec exit=muzzle();
    if (blocked(weaponPivot(),exit)) {
        burst(exit,hex(0x9ba8ba),5,96); audio.event(gunSounds[gun],.62f); return;
    }
    for (int i=0;i<w.pellets;++i) {
        const float spread=w.pellets==1?random()-.5f:float(i)/(w.pellets-1)-.5f;
        shots.push_back({exit,ray(p.aim+spread*w.spread)*w.speed,w.range/w.speed,float(g.damage),hex(w.color),w.pierce,0,gun==2});
    }
    burst(exit,hex(w.color),gun==1?9:4,140);
    {
        const Vec eject=weaponPivot()+ray(p.aim)*10.f;
        const float side=p.facing>0?-1.f:1.f;
        const int shells=gun==2?0:1;
        for (int i=0;i<shells;++i) {
            const float life=.9f+random()*.5f, angle=random()*2*pi;
            const Vec v=Vec{side*(70+random()*90),-(150+random()*120)};
            particles.push_back({eject,v,gun==1?hex(0xff6a55):hex(0xf2c46b),life,life,1.6f,(random()-.5f)*26,angle,ParticleKind::casing});
        }
    }
    shake=std::max(shake,gun==1?6.8f:gun==2?4.8f:2.6f);
    punch=std::max(punch,gun==1?.30f:gun==2?.22f:.10f);
    audio.event(gunSounds[gun],.88f,0,.96f+random()*.074f);
}
void Game::moveEnemies(float dt) {
    const auto& p=player.box;
    const float pace=difficulty->pace;
    for (auto& e:enemies) {
        if (!e.alive) continue;
        e.hit=std::max(0.f,e.hit-dt);
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
void Game::moveShots(float dt) {
    for (auto& s:shots) {
        s.life-=dt;
        if (s.life<=0) continue;
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
            if (hit<0) { s.life=0; break; }
            auto& e=enemies[hit];
            if (e.hit<=0) audio.event("hit-metal",.3f,(s.p.x-player.box.x)/590,.92f+random()*.23f);
            s.passed|=1u<<hit;
            e.hp-=static_cast<int>(s.damage); e.hit=.094f; e.flinch=1; e.kick=sign(s.v.x)*clamp(s.damage/60,.35f,1.f);
            if (e.hp<=0) destroy(e);
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
void Game::bury(const Enemy& e) {
    if (e.kind==EnemyKind::boss) return;
    const float away=sign(e.box.center().x-player.box.center().x);
    Corpse c;
    c.body=e;
    c.body.alive=false;
    c.body.hit=.12f;
    c.v=Vec{away*(70+random()*90),-(150+random()*110)};
    switch (e.kind) {
    case EnemyKind::drone: c.spin=away*(3+random()*4); c.rest=away*.5f; c.v.y=-60; c.life=1.5f; break;
    case EnemyKind::turret: c.spin=0; c.rest=0; c.v=Vec{0,-40}; c.life=1.1f; break;
    case EnemyKind::stalker: c.spin=away*(5+random()*3); c.rest=away*pi; c.life=1.9f; break;
    default: c.spin=away*(2.2f+random()*1.4f); c.rest=away*1.52f; c.life=2.3f; break;
    }
    corpses.push_back(c);
}
void Game::updateCorpses(float dt) {
    for (auto& c:corpses) {
        c.age+=dt;
        const bool flying=c.body.kind==EnemyKind::drone;
        Vec center=c.body.box.center();
        if (!c.settled) {
            c.v.y+=(flying?900.f:1500.f)*dt;
            c.v.x*=1-.9f*dt;
            center+=c.v*dt;
            const float half=c.body.box.h*.5f*(1-.3f*std::abs(std::sin(c.angle)));
            for (const auto& surface:platforms) {
                const auto& s=surface.box;
                if (c.v.y<=0 || center.x<s.x || center.x>s.right()) continue;
                const float before=center.y-c.v.y*dt+half;
                if (before>s.y+3 || center.y+half<s.y) continue;
                center.y=s.y-half;
                if (c.v.y>260 && c.body.kind!=EnemyKind::turret) {
                    c.v.y*=-.28f; c.v.x*=.6f;
                    burst(Vec{center.x,s.y},hex(stage->cold),4,70,ParticleKind::smoke);
                } else { c.v=Vec{}; c.settled=true; }
                break;
            }
            if (c.body.kind==EnemyKind::turret) c.v.x=0;
            c.angle+=c.spin*dt;
            if (c.rest!=0 && std::abs(c.angle)>std::abs(c.rest) && !flying) { c.angle=c.rest; c.spin*=.2f; }
        } else {
            c.angle=approach(c.angle,c.rest,dt*4);
        }
        c.body.box.x=center.x-c.body.box.w*.5f; c.body.box.y=center.y-c.body.box.h*.5f;
        c.body.hit=std::max(0.f,c.body.hit-dt);
        if (flying && !c.settled && random()<dt*20) burst(center,hex(0xffa55e),1,40,ParticleKind::smoke);
        if (c.life-c.age<.4f && c.life-c.age+dt>=.4f) burst(center,hex(0xffd9a0),6,120);
    }
    for (const auto& c:corpses) if (c.age>=c.life) burst(c.body.box.center(),hex(0x98b4c7),8,90,ParticleKind::smoke);
    std::erase_if(corpses,[](const Corpse& c){return c.age>=c.life;});
}
void Game::ring(Vec p,Color color,float radius,float life) { rings.push_back({p,color,life,life,radius}); }
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
void Game::destroy(Enemy& e) {
    e.alive=false; ++kills;
    bury(e);
    const bool boss=e.kind==EnemyKind::boss;
    award(e,spec(e.kind).score+(boss?index*22:0));
    const Color accent=hex(stage->accent);
    burst(e.box.center(),accent,boss?46:16,boss?315:158);
    burst(e.box.center(),hex(0xfff0b8),boss?20:8,boss?380:230);
    burst(e.box.center(),hex(0x98b4c7),boss?26:9,boss?220:104,ParticleKind::smoke);
    burst(e.box.center(),hex(0x93a5ac),boss?24:7,boss?235:135,ParticleKind::chip);
    ring(e.box.center(),hex(0xfff0b8),boss?150:62,boss?.7f:.4f);
    ring(e.box.center(),accent,boss?96:38,boss?.55f:.3f);
    shake=std::max(shake,boss?15.f:4.2f);
    audio.event(boss?"guardian":"destroy",boss?1.f:.62f,(e.box.center().x-player.box.x)/width,.94f+random()*.16f);
    if (boss) {
        bossDead=true; bossActive=false; hostile.clear();
        message(e.box.center()-Vec{0,49},"EXTRACTION UNLOCKED",hex(0xb4f5ca),2);
        audio.event("checkpoint");
    }
}
void Game::hurt(float damage,bool force) {
    auto& p=player;
    if (mode!=Mode::active || (p.flash>0 && !force)) return;
    p.hp=std::max(0,p.hp-int(std::ceil(damage))); p.flash=.69f;
    burst({p.box.center().x,p.box.y+20},hex(0xff795d),10,132);
    shake=std::max(shake,8.7f); audio.event("hurt");
    hitStop=std::max(hitStop,.07f); punch=std::max(punch,.6f); pain=1; combo=0; comboTimer=0;
    ring({p.box.center().x,p.box.y+20},hex(0xff795d),52,.36f);
    if (!p.hp) { release(); mode=Mode::failed; }
}
int Game::ventState(float phase) const {
    const float cycle=std::fmod(time+phase,3.65f);
    return cycle>2.76f?2:cycle>2.22f?1:0;
}
void Game::triggers() {
    auto& p=player;
    for (auto& item:supplies) {
        if (item.used || std::abs(item.p.x-p.box.center().x)>29 || std::abs(item.p.y-(p.box.bottom()-18.f))>31) continue;
        std::string text;
        if (item.kind==SupplyKind::health) {
            if (p.hp>=character->health) continue;
            p.hp=std::min(character->health,p.hp+42); text="SUIT REPAIRED";
        } else if (item.kind==SupplyKind::salvage) {
            const int points=std::lround(22*difficulty->score);
            score+=points; text="+"+std::to_string(points)+" SCORE";
        } else {
            bool resupplied=false;
            for (auto& g:guns) {
                if (!g.available || g.reserve<0 || g.reserve>=g.maxReserve) continue;
                g.reserve=std::min(g.maxReserve,g.reserve+int(std::ceil(g.maxReserve*.41f)));
                resupplied=true;
            }
            if (!resupplied) continue;
            text="AMMO RESUPPLIED";
        }
        const Color tint=hex(item.kind==SupplyKind::health?0x9dedb4:0xffd190);
        item.used=true; message(item.p-Vec{0,16},text,tint); burst(item.p,tint,10,105);
        audio.event("pickup",1,0,.95f+random()*.10f);
    }
    if (!checkpoint && std::abs(p.box.center().x-stage->beacon.x)<44 && std::abs(p.box.bottom()-stage->beacon.y)<155) {
        checkpoint=true; p.hp=character->health;
        backup={time,score,kills,gun,bossActive,bossDead,guns,enemies,supplies};
        message(stage->beacon-Vec{0,70},"CHECKPOINT ONLINE",hex(0xb9ffcf),1.75f);
        audio.event("checkpoint");
    }
    for (const auto& v:stage->vents) {
        if (ventState(v.phase)==2 && p.box.overlaps({v.x-19,v.y-75,38,75})) hurt(12+index*2);
    }
}
void Game::message(Vec p,std::string text,Color color,float duration) {
    messages.push_back({p,std::move(text),color,duration,duration});
}
void Game::burst(Vec p,Color color,int count,float speed,ParticleKind kind) {
    count=std::min(count,420-int(particles.size()));
    for (int i=0;i<count;++i) {
        const float life=.18f+random()*(kind==ParticleKind::smoke?.66f:.46f);
        const float angle=random()*2*pi;
        const Vec v=ray(angle)*(speed*(.25f+random()*.75f))-Vec{0,18};
        particles.push_back({p,v,color,life,life,1+random()*3,(random()-.5f)*12,angle,kind});
    }
}
void Game::updateParticles(float dt) {
    for (auto& p:particles) {
        p.life-=dt;
        p.angle+=p.spin*dt;
        const Vec before=p.p;
        const bool smoke=p.kind==ParticleKind::smoke;
        const bool casing=p.kind==ParticleKind::casing;
        p.v=p.v*(1.f-(smoke?2.6f:casing?.2f:.62f)*dt)+Vec{0,(smoke?-24.f:casing?1100.f:390.f)*dt};
        p.p+=p.v*dt;
        if (p.v.y<=0 || smoke) continue;
        for (const auto& surface:platforms) {
            const auto& s=surface.box;
            if (before.y>s.y || p.p.y<s.y || p.p.x<s.x || p.p.x>s.right()) continue;
            p.p.y=s.y-.1f; p.v.y*=-.34f; p.v.x*=.62f; break;
        }
    }
    for (auto& r:rings) r.life-=dt;
    if (comboTimer>0 && (comboTimer-=dt)<=0) combo=0;
    style.life=std::max(0.f,style.life-dt);
    for (auto& m:messages) { m.life-=dt; m.p.y-=25*dt; }
    std::erase_if(particles,[](const Particle& p){return p.life<=0;});
    std::erase_if(messages,[](const Message& m){return m.life<=0;});
    std::erase_if(rings,[](const Ring& r){return r.life<=0;});
}
} // namespace vl
