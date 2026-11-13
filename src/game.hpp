#pragma once
#include "campaign.hpp"
#include <random>
#include <string_view>

namespace vl {
struct AudioSink {
    virtual ~AudioSink() = default;
    virtual void event(std::string_view name,float gain=1,float pan=0,float rate=1)=0;
};
inline constexpr std::array<float,4> muzzleReach{44,48,53,26};
inline constexpr float flipSeconds=.58f;
enum class Key { left,right,jump,fire,automatic,reload,sprint,crouch,slow,count };
enum class Mode { active,paused,failed,complete };
enum class ParticleKind { spark,smoke,chip,casing };
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
};
struct Player {
    Rect box;
    Vec velocity,safe;
    int support=-1, facing=1, hp=0, sprintDirection=0;
    float aim=0,coyote=0,jumpBuffer=0,jumpAge=0,windup=0,flash=0,muzzle=0,fireDelay=0,fireBuffer=0;
    float flip=0,flipDirection=1;
    bool crouched=false,sprinting=false,aiming=false,airJumpUsed=false;
    Gait gait;
};
struct Gun {
    int damage=0,capacity=0,maxReserve=0,ammo=0,reserve=0;
    float duration=0,reload=0;
    bool available=false;
};
struct Enemy {
    EnemyKind kind;
    Rect box;
    Vec origin;
    float base,patrol,phase,timer,damage,reach,speed;
    int hp,maxHp,facing=1,volley=0;
    float aim=0,warning=0,hit=0,vertical=0,jump=0,vx=0,distance=0;
    bool alive=true;
};
struct Supply { SupplyKind kind; Vec p; bool used=false; };
struct Shot {
    Vec p,v;
    float life,damage;
    Color color;
    int pierce=0;
    uint32_t passed=0;
    bool longShot=false,wave=false;
};
struct Particle {
    Vec p,v;
    Color color;
    float life,maxLife,size,spin,angle;
    ParticleKind kind;
};
struct Message { Vec p; std::string text; Color color; float life,maxLife; };
struct Ring { Vec p; Color color; float life,maxLife,radius; };
struct StyleCall { std::string text; Color color; float life=0,maxLife=1; int combo=0; };

class Game {
public:
    explicit Game(AudioSink& output):audio(output) {}
    void start(int sector,const Save& save);
    void retry();
    void tick(float dt=step);
    void pause(bool paused);
    void input(Key key,bool down);
    void release();
    // Synchronizes a held key without triggering its press behavior.
    void hold(Key key,bool down) { keys[static_cast<size_t>(key)]=down; }
    void select(int weapon);
    void pointer(Vec position) { mouse=position; pointerActive=true; }
    void stick(Vec direction,bool active) { stickAim=direction; stickActive=active; }
    // Advances real-time effects. Returns the simulation speed to use for this frame.
    float advance(float realDt);
    bool slowMotion() const { return slowing; }
    int ventState(float phase) const;
    Vec weaponPivot() const;
    Vec muzzle() const;
    void resetPose();

    const Stage* stage=nullptr;
    const Character* character=nullptr;
    const Difficulty* difficulty=nullptr;
    int index=0, gun=0, score=0,kills=0;
    bool checkpoint=false,bossActive=false,bossDead=false,pointerActive=false;
    Mode mode=Mode::paused;
    float time=0,entry=0,shake=0,camera=0;
    float timeScale=1,hitStop=0,punch=0,slowEnergy=1,comboTimer=0,flash=0,pain=0;
    int combo=0,bestCombo=0;
    Vec mouse;
    Player player;
    std::array<Gun,4> guns{};
    std::vector<Platform> platforms;
    std::vector<Enemy> enemies;
    std::vector<Supply> supplies;
    std::vector<Shot> shots,hostile;
    std::vector<Particle> particles;
    std::vector<Message> messages;
    std::vector<Ring> rings;
    StyleCall style;

private:
    struct Checkpoint {
        float time=0;
        int score=0,kills=0,gun=0;
        bool bossActive=false,bossDead=false;
        std::array<Gun,4> guns{};
        std::vector<Enemy> enemies;
        std::vector<Supply> supplies;
    } backup;
    AudioSink& audio;
    Save campaign;
    std::array<bool,static_cast<size_t>(Key::count)> keys{};
    Vec stickAim{};
    bool stickActive=false,slowing=false,slowLocked=false;
    std::minstd_rand rng{0x2796};
    float random();
    bool held(Key k) const { return keys[static_cast<size_t>(k)]; }
    void spawn(Vec safe);
    void resetTransient();
    void moveLifts(float dt);
    void movePlayer(float dt);
    void updateGait(float dt,float travel,float accel,int priorSupport,float landing);
    void jumpFrom(int support);
    bool headroom(float x,float feet) const;
    void aimPlayer(int direction);
    bool blocked(Vec start,Vec end) const;
    void reload();
    void shoot(float dt);
    void moveEnemies(float dt);
    void enemyFire(Enemy& e);
    void moveShots(float dt);
    void destroy(Enemy& e);
    void award(Enemy& e,int base);
    void ring(Vec p,Color color,float radius,float life=.42f);
    void hurt(float damage,bool force=false);
    void triggers();
    void burst(Vec p,Color color,int count,float speed,ParticleKind kind=ParticleKind::spark);
    void message(Vec p,std::string text,Color color,float duration=1.05f);
    void updateParticles(float dt);
};
Vec solveKnee(Vec hip,Vec ankle,float thigh,float shin,int forward);
}
