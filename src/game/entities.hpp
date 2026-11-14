#pragma once
#include "content/content.hpp"
#include <string_view>

namespace vl {
struct AudioSink {
    virtual ~AudioSink() = default;
    virtual void event(std::string_view name,float gain=1,float pan=0,float rate=1)=0;
};
inline constexpr std::array<float,weaponCount> muzzleReach{44,48,53,26,38,46,66};
inline constexpr float flipSeconds=.58f;
// Seconds the weapon stays raised after the last aiming input.
inline constexpr float readyHold=1.6f;
enum class Key { left,right,jump,fire,automatic,reload,sprint,crouch,slow,count };
enum class Mode { active,paused,failed,complete };
enum class ParticleKind { spark,smoke,chip,casing,puff,weapon,ember };
// Background happenings that play out behind the action. They never touch the simulation except for light and rumble.
enum class AmbientKind { blast,flyer,fountain,streak,lightning,searchlight,steam,spores };
struct Ambient {
    AmbientKind kind;
    Vec p,v;
    float depth=.3f,life=1,maxLife=1,size=1,seed=0;
    int variant=0;
    float pulse() const;
};
struct Foot {
    float phase=0, angle=0;
    int surface=-1;
    float anchor=0;
    Vec heel, takeoff;
    bool planted=false;
};
struct Gait {
    std::array<Foot,2> feet{};
    float cycle=0, weight=0, run=0, lean=0, compression=0, compressVelocity=0;
    float crouch=0, tuck=0, reach=0, pelvis=0, launchFoot=0;
    int direction=1;
};
struct Platform {
    Rect box;
    Vec velocity;
    bool ground=false;
    const Lift* lift=nullptr;
    bool cover=false;
};
struct Player {
    Rect box;
    Vec velocity,safe;
    int support=-1, facing=1, hp=0, sprintDirection=0;
    float aim=0,coyote=0,jumpBuffer=0,jumpAge=0,windup=0,flash=0,muzzle=0,fireDelay=0,fireBuffer=0;
    float flip=0,flipDirection=1,turn=1,landed=0;
    bool crouched=false,sprinting=false,aiming=false,airJumpUsed=false,crouchLatch=false;
    // The weapon is raised (1) while aiming, firing or reloading and carried low (0) afterwards.
    float ready=1,readyTimer=0;
    Gait gait;
};
struct Gun {
    int damage=0,capacity=0,maxReserve=0,ammo=0,reserve=0;
    float duration=0,reload=0;
    bool available=false;
    float range=0,interval=0,spread=0,crit=0;
};
// A bullet hole on an enemy, stored relative to the center of its body so it moves and falls with it.
struct Wound { Vec local; float age=0,angle=0,size=1; };
struct Enemy {
    EnemyKind kind;
    Rect box;
    Vec origin;
    float base,patrol,phase,timer,damage,reach,speed;
    int hp,maxHp,facing=1,volley=0;
    float aim=0,warning=0,hit=0,vertical=0,jump=0,vx=0,distance=0;
    bool alive=true;
    // Animation state. These never affect the rules of the simulation except where noted.
    float turn=1,raise=0,flinch=0,kick=0,recoil=0,landed=0,stride=0;
    // Hit feedback. zone is where the last hit landed: -1 head, 0 torso, +1 legs.
    std::array<Wound,6> wounds{};
    int woundCount=0;
    Vec hitDir{1,0};
    float zone=0,headSnap=0,buckle=0;
    // Health bar that stays visible for a while after a hit, with a trailing segment showing the damage taken.
    float bar=0,trail=1;
    // Damage that was dealt since the last floating number.
    float pending=0,numberTimer=0;
    bool pendingCrit=false;
    // Death animation. deathAge counts up while the body is a corpse. fallDir is the side it topples toward.
    float deathAge=0,fallDir=1;
    bool elite=false;
};
struct Explosive {
    CrateKind kind;
    Rect box;
    int hits;
    bool spent=false;
};
struct Supply { SupplyKind kind; Vec p; bool used=false; };
struct Shot {
    Vec p,v;
    float life,damage;
    Color color;
    int pierce=0;
    std::vector<int> passed{};
    bool longShot=false,wave=false;
    float gravity=0,blast=0;
    bool crit=false;
    int weapon=-1;
    uint32_t id=0;
};
struct Particle {
    Vec p,v;
    Color color;
    float life,maxLife,size,spin,angle;
    ParticleKind kind;
};
struct Message { Vec p; std::string text; Color color; float life,maxLife; float scale=1; };
struct Corpse {
    Enemy body;
    Vec v;
    float angle=0,spin=0,rest=0,age=0,life=0;
    bool settled=false;
    // The figure topples about its feet instead of its center.
    bool pivotFeet=false;
    // Set when the wreck has already blown up and only smolders.
    bool scorched=false;
    float nextBlast=0;
    int blasts=0;
};
struct Ring { Vec p; Color color; float life,maxLife,radius; };
struct StyleCall { std::string text; Color color; float life=0,maxLife=1; int combo=0; };

}
