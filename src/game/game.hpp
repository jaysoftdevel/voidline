#pragma once
#include "game/operative.hpp"
#include "game/geometry.hpp"
#include <random>

namespace vl {
class Game {
public:
    explicit Game(AudioSink& output):audio(output) {}
    Game(const Game&)=delete;
    Game& operator=(const Game&)=delete;
    void setTeammate(const Operative& value) { partner=value; }
    void start(int sector,const Save& save,bool cooperative=false);
    void startCoop(int sector,const Save& loadout,int guestCharacter=-1,bool cooperative=false);
    void retry();
    bool recoverLife(); // Called after a failed tick; never during physics iteration.
    void applyControls(const Controls& controls,bool second=false,bool pointerMoved=true);
    const Operative& teammate() const { return partner; }
    struct RelayState {
        float charge=0;  // Plate charge or lever activation hold
        bool latched=false;
        float lease=0;   // Lever grants a temporary opening so the partner can cross
        float openness=0; // Replicated shutter animation, zero closed and one raised
    };
    bool coop=false,puzzleMode=false;
    std::vector<RelayState> relayStates;
    bool relayOpen(size_t relayIndex) const;
    void updateRelays(float dt);
    bool atExtraction(const Player& body) const;
    void syncLifts() { moveLifts(0); }
    void tick(float dt=step);
    void pause(bool paused);
    void input(Key key,bool down);
    void release();
    // Synchronizes a held key without triggering its press behavior.
    void hold(Key key,bool down) { keys[static_cast<size_t>(key)]=down; }
    void select(int weapon);
    void pointer(Vec position) {
        const Vec target=position+Vec{camera,cameraY};
        pointAt(target,length(target-mouse)>2.5f);
    }
    void pointAt(Vec world,bool moved) {
        if (moved) player.readyTimer=readyHold;
        mouse=world; pointerActive=true;
    }
    void stick(Vec direction,bool active) { stickAim=direction; stickActive=active; }
    // Advances real-time effects. Returns the simulation speed to use for this frame.
    float advance(float realDt);
    bool slowMotion() const { return slowing; }
    // Applies damage, records the wound and plays every hit reaction. direction is the way the bullet was traveling.
    void damageEnemy(Enemy& e,float damage,Vec point,Vec direction,bool crit);
    // Starts a background event. Without arguments the sector picks one from its own weighted table.
    void spawnAmbient(int forcedKind=-1,int forcedVariant=0);
    int ventState(float phase) const;
    bool nearCover() const { return nearCover(player); }
    bool nearCover(const Player& body) const;
    Vec weaponPivot(const Player& body) const;
    Vec muzzle(const Operative& operative) const;
    Vec weaponPivot() const;
    Vec muzzle() const;
    void resetPose();

    const Stage* stage=nullptr;
    Operative operative;
    const Character*& character=operative.character;
    const Difficulty* difficulty=nullptr;
    int index=0,score=0,kills=0;
    int lives=4,maxLives=4; // A fresh allowance for each mission, including retries.
    int& gun=operative.gun;
    int activeBeacon=-1;
    bool bossActive=false,bossDead=false;
    Mode mode=Mode::paused;
    float time=0,entry=0,shake=0;
    float& camera=operative.camera;
    float& cameraY=operative.cameraY;
    float timeScale=1,hitStop=0,punch=0,slowEnergy=1,comboTimer=0,flash=0,pain=0;
    // Crosshair feedback. hitMarker lights up on every hit and killMarker on a kill.
    float hitMarker=0,killMarker=0;
    int combo=0,bestCombo=0;
    OperativeInput localInput;
    Vec& mouse=localInput.mouse;
    bool& pointerActive=localInput.pointerActive;
    Player& player=operative.player;
    std::array<Gun,weaponCount>& guns=operative.guns;
    int& maxHealth=operative.maxHealth;
    // Values derived from the operative upgrades that were purchased in the armory.
    float speedScale=1,jumpScale=1,slowDrain=.16f,slowRegen=.045f,pickupAmmo=.41f,pickupHeal=42,damageTaken=1;
    std::vector<Ambient> ambient;
    float ambientTimer=2;
    float skyFlash=0;
    Color skyColor{1,1,1,1};
    std::vector<Platform> platforms;
    std::vector<Enemy> enemies;
    std::vector<Supply> supplies;
    std::vector<Explosive> crates;
    std::vector<Shot> shots,hostile;
    std::vector<Particle> particles;
    std::vector<Message> messages;
    std::vector<Ring> rings;
    std::vector<Corpse> corpses;
    StyleCall style;
    uint32_t setpieceMask=0;
    int bossPhase=0;

private:
    struct Checkpoint {
        float time=0;
        int score=0,kills=0,gun=0;
        bool bossActive=false,bossDead=false;
        std::array<Gun,weaponCount> guns{};
        std::vector<Enemy> enemies;
        std::vector<Supply> supplies;
        std::vector<Explosive> crates;
        Operative partner;
        std::vector<RelayState> relayStates;
    } backup;
    AudioSink& audio;
    Save campaign;
    Stage editedStage; // Optional level file loaded at mission start; editor stays a separate executable.
    Operative partner;
    OperativeInput partnerInput;
    std::array<bool,static_cast<size_t>(Key::count)>& keys=localInput.keys;
    Vec& stickAim=localInput.stickAim;
    bool& stickActive=localInput.stickActive;
    bool controllingPartner=false;
    void swapOperatives();
    void releaseBoth();
    void updateCamera(float dt);
    void setupLoadout(const Save& save);
    void refreshCharacterBonuses();
    void spawnPartner(Vec safe);

    std::minstd_rand ambientRng{0x51f};
    float ambientRandom() { return std::generate_canonical<float,22>(ambientRng); }
    bool slowing=false,slowLocked=false;
    std::minstd_rand rng{0x2796};
    float random();
    uint32_t nextShot=0;
    bool held(Key k) const { return keys[static_cast<size_t>(k)]; }
    void spawn(Vec safe);
    void resetTransient();
    void moveLifts(float dt);
    // The operative's movement, one function per phase.
    struct Landing { float strength; int attached; };
    void movePlayer(float dt);
    void updateStance(bool grounded);
    void runHorizontal(float dt,int direction,bool grounded);
    void collideWalls(float oldX,bool grounded);
    void tickPlayerTimers(float dt,bool grounded);
    void handleJump(bool grounded,int priorSupport,int direction,float dt);
    void airFlip(int direction);
    Landing moveVertical(float dt,float oldX);
    void recoverFromFall();
    void updateGait(float dt,float travel,float accel,int priorSupport,float landing);
    void jumpFrom(int support,int takeoffDirection);
    bool headroom(float x,float feet) const;
    void aimPlayer(int direction);
    bool blocked(Vec start,Vec end) const;
    void reload();
    void shoot(float dt);
    void moveEnemies(float dt);
    void enemyFire(Enemy& e,float targetX);
    void scriptedSetpieces();
    void moveShots(float dt);
    void destroy(Enemy& e);
    void explode(Vec at,float radius,float damage,Color color);
    int crateOnSegment(Vec start,Vec change,float& first) const;
    void strikeCrate(Explosive& crate,Vec hit);
    void detonateCrates();
    void blastEffects(Vec at,float radius,Color color);
    void registerHit(Enemy& e,Vec point,Vec direction,float damage,bool crit);
    void flushDamageNumber(Enemy& e);
    void updateEnemyFeedback(Enemy& e,float dt);
    void burstCone(Vec at,Vec direction,float spread,Color color,int count,float speed,ParticleKind kind=ParticleKind::spark);
    void updateAmbient(float dt);

    void award(Enemy& e,int base);
    void ring(Vec p,Color color,float radius,float life=.42f);
    void hurt(float damage,bool force=false);
    void triggers();
    void checkpoints();
    void burst(Vec p,Color color,int count,float speed,ParticleKind kind=ParticleKind::spark);
    void message(Vec p,std::string text,Color color,float duration=1.05f);
    void updateParticles(float dt);
    void updateCorpses(float dt);
    // Death animations, one per kind of enemy (game/death_*.cpp).
    bool dropCorpse(Corpse& c,float dt,float pull);
    void updateSoldierCorpse(Corpse& c,float dt);
    void updateDroneCorpse(Corpse& c,float dt);
    void updateTurretCorpse(Corpse& c,float dt);
    void updateStalkerCorpse(Corpse& c,float dt);
    void updateGuardianCorpse(Corpse& c,float dt);
    void wreckBlast(Vec at,float size);
    void bury(const Enemy& e);
};
}
