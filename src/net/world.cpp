#include "net/world.hpp"
#include <bit>
#include <limits>
#include <type_traits>

namespace vl::net {
namespace {
class Writer {
public:
    Bytes bytes;
    template<class... T> void operator()(const T&... values) { (put(values),...); }
    template<class T> void list(const std::vector<T>& list,size_t limit) {
        const auto count=std::min(limit,list.size());
        put(static_cast<uint16_t>(count));
        // Transient particles and sounds are newest at the back.
        for (size_t i=list.size()-count;i<list.size();++i) put(list[i]);
    }
private:
    template<class T> void put(const T& value) {
        if constexpr (std::is_same_v<T,bool>) bytes.push_back(value);
        else if constexpr (std::is_enum_v<T>) put(static_cast<std::underlying_type_t<T>>(value));
        else if constexpr (std::is_same_v<T,float>) put(std::bit_cast<uint32_t>(value));
        else if constexpr (std::is_integral_v<T>) {
            using U=std::make_unsigned_t<T>;
            for (size_t i=0;i<sizeof(T);++i) bytes.push_back(uint8_t(static_cast<U>(value)>>(i*8)));
        } else if constexpr (std::is_same_v<T,std::string>) {
            const auto size=std::min<size_t>(255,value.size());
            put(static_cast<uint8_t>(size));
            bytes.insert(bytes.end(),value.begin(),value.begin()+size);
        } else fields(*this,value);
    }
};

class Reader {
public:
    explicit Reader(std::span<const uint8_t> data):bytes(data) {}
    bool done() const { return valid && offset==bytes.size(); }
    template<class... T> void operator()(T&... values) { (get(values),...); }
    template<class T> void list(std::vector<T>& list,size_t limit) {
        uint16_t count=0;
        get(count);
        if (!valid || count>limit || count>bytes.size()-offset) { valid=false; return; }
        list.resize(count);
        for (auto& element:list) get(element);
    }
private:
    std::span<const uint8_t> bytes;
    size_t offset=0;
    bool valid=true;
    template<class T> void get(T& value) {
        if (!valid) return;
        if constexpr (std::is_same_v<T,bool>) {
            uint8_t v=0;
            get(v); valid=valid && v<=1; value=v!=0;
        } else if constexpr (std::is_enum_v<T>) {
            std::underlying_type_t<T> v{};
            get(v); value=static_cast<T>(v);
        } else if constexpr (std::is_same_v<T,float>) {
            uint32_t v=0;
            get(v); value=std::bit_cast<float>(v);
            valid=valid && std::isfinite(value) && std::abs(value)<=1e8f;
        } else if constexpr (std::is_integral_v<T>) {
            if (sizeof(T)>bytes.size()-offset) { valid=false; return; }
            using U=std::make_unsigned_t<T>;
            U v=0;
            for (size_t i=0;i<sizeof(T);++i) v|=static_cast<U>(static_cast<U>(bytes[offset++])<<(i*8));
            value=std::bit_cast<T>(v);
        } else if constexpr (std::is_same_v<T,std::string>) {
            uint8_t size=0;
            get(size);
            if (!valid || size>bytes.size()-offset) { valid=false; return; }
            value.assign(reinterpret_cast<const char*>(bytes.data()+offset),size);
            offset+=size;
        } else fields(*this,value);
    }
};

// The wire format names fields explicitly and uses fixed-width little-endian values.
static_assert(sizeof(int)==4 && sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
#define FIELDS(Type,...) \
    template<class IO,class T> requires std::is_same_v<std::remove_const_t<T>,Type> \
    void fields(IO& io,T& v) { io(__VA_ARGS__); }
FIELDS(Vec,v.x,v.y)
FIELDS(Rect,v.x,v.y,v.w,v.h)
FIELDS(Color,v.r,v.g,v.b,v.a)
FIELDS(Foot,v.heel,v.angle)
FIELDS(Gait,v.feet[0],v.feet[1],v.cycle,v.weight,v.run,v.lean,v.compression,
    v.crouch,v.tuck,v.reach,v.pelvis,v.launchFoot,v.direction)
FIELDS(Player,v.box,v.velocity,v.support,v.facing,v.hp,v.aim,v.flash,v.muzzle,v.flip,v.flipDirection,
    v.turn,v.landed,v.crouched,v.sprinting,v.aiming,v.ready,v.gait)
FIELDS(Gun,v.capacity,v.ammo,v.reserve,v.duration,v.reload,v.available)
FIELDS(Wound,v.local,v.age,v.angle,v.size)
FIELDS(Enemy,v.kind,v.box,v.phase,v.hp,v.maxHp,v.facing,v.aim,v.warning,v.hit,v.vx,v.distance,v.alive,
    v.turn,v.raise,v.flinch,v.kick,v.recoil,v.landed,v.stride,v.woundCount,v.hitDir,v.zone,v.headSnap,
    v.buckle,v.bar,v.trail,v.deathAge,v.fallDir,v.elite)
FIELDS(Corpse,v.body,v.angle,v.age,v.life,v.pivotFeet)
FIELDS(Particle,v.p,v.v,v.color,v.life,v.maxLife,v.size,v.spin,v.angle,v.kind)
FIELDS(Shot,v.p,v.v,v.color,v.longShot,v.wave,v.blast)
FIELDS(Supply,v.kind,v.p,v.used)
FIELDS(Explosive,v.kind,v.box,v.hits,v.spent)
FIELDS(Ambient,v.kind,v.p,v.v,v.depth,v.life,v.maxLife,v.size,v.seed,v.variant)
FIELDS(Message,v.p,v.text,v.color,v.life,v.maxLife,v.scale)
FIELDS(Ring,v.p,v.color,v.life,v.maxLife,v.radius)
FIELDS(StyleCall,v.text,v.color,v.life,v.maxLife,v.combo)
FIELDS(Sound,v.name,v.gain,v.pan,v.rate)
#undef FIELDS

// Wound arrays are encoded separately for the live figures and omitted on corpses.
template<class IO,class T> void wounds(IO& io,T& enemies) {
    for (auto& e:enemies) for (auto& w:e.wounds) io(w);
}

template<class IO,class T> void operative(IO& io,T& op,int& character) {
    io(character,op.gun,op.maxHealth,op.camera,op.player);
    for (auto& gun:op.guns) io(gun);
}

template<class IO,class T,class S> void world(IO& io,T& g,S& sounds) {
    io(g.mode,g.time,g.entry,g.shake,g.timeScale,g.punch,g.slowEnergy,g.comboTimer,g.flash,g.pain,
        g.hitMarker,g.killMarker,g.combo,g.score,g.kills,g.activeBeacon,g.bossActive,g.bossDead,
        g.skyFlash,g.skyColor,g.style);
    io.list(g.enemies,128); wounds(io,g.enemies);
    io.list(g.supplies,128); io.list(g.crates,128);
    io.list(g.ambient,16); io.list(g.shots,240); io.list(g.hostile,240);
    io.list(g.corpses,50); io.list(g.particles,320); io.list(g.rings,50); io.list(g.messages,40);
    io.list(sounds,32);
}

bool validOperative(const Game& game,const Operative& op,int character) {
    return character>=0 && character<static_cast<int>(characters.size()) &&
        op.gun>=0 && op.gun<static_cast<int>(weaponCount) && op.maxHealth>0 &&
        op.player.support>=-1 && op.player.support<static_cast<int>(game.platforms.size()) &&
        op.player.box.w>0 && op.player.box.h>0;
}
bool validEnemy(const Enemy& e) {
    return e.kind>=EnemyKind::drone && e.kind<=EnemyKind::boss && e.maxHp>0 && e.woundCount>=0;
}
}

Bytes encodeControls(const Controls& c,uint32_t mission) {
    Writer io;
    io(mission,c.held,c.pressed,c.weapon,c.cycle,c.aimMode,c.aim);
    return std::move(io.bytes);
}
bool decodeControls(std::span<const uint8_t> bytes,Controls& c,uint32_t& mission) {
    Reader io(bytes);
    io(mission,c.held,c.pressed,c.weapon,c.cycle,c.aimMode,c.aim);
    const auto mask=keyBit(Key::count)-1u;
    return io.done() && mission>0 && c.held<=mask && c.pressed<=mask && c.weapon>=-1 &&
        c.weapon<static_cast<int>(weaponCount) && c.cycle>=-1 && c.cycle<=1 && c.aimMode<=AimMode::stick &&
        (c.aimMode!=AimMode::stick || (std::abs(c.aim.x)<=1.01f && std::abs(c.aim.y)<=1.01f));
}
Bytes encodeMission(int sector,int difficulty,uint32_t mission) {
    Writer io;
    io(sector,difficulty,mission);
    return std::move(io.bytes);
}
bool decodeMission(std::span<const uint8_t> bytes,int& sector,int& difficulty,uint32_t& mission) {
    Reader io(bytes);
    io(sector,difficulty,mission);
    return io.done() && mission>0 && sector>=0 && sector<sectorCount && difficulty>=0 &&
        difficulty<static_cast<int>(difficulties.size());
}
Bytes encodeWorld(const Game& g,const std::vector<Sound>& sounds) {
    Writer io;
    const auto& guest=g.teammate();
    int guestCharacter=static_cast<int>(guest.character-characters.data());
    int hostCharacter=static_cast<int>(g.character-characters.data());
    operative(io,guest,guestCharacter);
    operative(io,g.operative,hostCharacter);
    world(io,g,sounds);
    return std::move(io.bytes);
}
bool decodeWorld(std::span<const uint8_t> bytes,Game& g,std::vector<Sound>& sounds) {
    Reader io(bytes);
    Operative guest{},host{};
    int guestCharacter=0,hostCharacter=0;
    operative(io,guest,guestCharacter);
    operative(io,host,hostCharacter);
    world(io,g,sounds);
    if (!io.done() || !validOperative(g,guest,guestCharacter) || !validOperative(g,host,hostCharacter) ||
        g.enemies.size()!=g.stage->enemies.size() || g.supplies.size()!=g.stage->supplies.size() ||
        g.crates.size()!=g.stage->crates.size() || g.mode<Mode::active || g.mode>Mode::complete ||
        g.activeBeacon<-1 || g.activeBeacon>=static_cast<int>(g.stage->beacons.size()) ||
        !std::all_of(g.enemies.begin(),g.enemies.end(),validEnemy) ||
        !std::all_of(g.corpses.begin(),g.corpses.end(),[](const Corpse& c) { return validEnemy(c.body); }) ||
        !std::all_of(g.crates.begin(),g.crates.end(),[](const Explosive& c) { return c.kind>=CrateKind::charge && c.kind<=CrateKind::reactor; }) ||
        !std::all_of(g.supplies.begin(),g.supplies.end(),[](const Supply& s) {
            return s.kind>=SupplyKind::ammo && s.kind<=SupplyKind::health;
        }) ||
        !std::all_of(g.ambient.begin(),g.ambient.end(),[](const Ambient& a) {
            return a.kind>=AmbientKind::blast && a.kind<=AmbientKind::spores && a.maxLife>0;
        }) ||
        !std::all_of(g.particles.begin(),g.particles.end(),[](const Particle& p) { return p.maxLife>0; }) ||
        !std::all_of(g.rings.begin(),g.rings.end(),[](const Ring& r) { return r.maxLife>0; }) ||
        !std::all_of(g.messages.begin(),g.messages.end(),[](const Message& m) { return m.maxLife>0; }) ||
        !std::all_of(sounds.begin(),sounds.end(),[](const Sound& s) {
            return s.gain>=0 && s.gain<=4 && std::abs(s.pan)<=100 && s.rate>.05f && s.rate<=4;
        })) return false;
    guest.character=&characters[guestCharacter];
    host.character=&characters[hostCharacter];
    g.operative=guest; g.setTeammate(host);
    g.syncLifts();
    return true;
}
}
