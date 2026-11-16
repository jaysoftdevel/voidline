#include "content/stages.hpp"
#include "game/stairs.hpp"
#include <algorithm>
#include <string>

namespace vl {
// Six two-route relay missions. Both operatives deploy into a common, safe
// insertion bay and split at a marked staircase/lift junction. Host ascends
// to the overhead control route, guest stays on the industrial floor. Every
// relay requires one role to hold a pad while the other clears its own door.
// Roles alternate without swapping routes so each player has a real task.
const std::array<Stage,cooperativeSectorCount> cooperativeStages=[] {
    constexpr const char* names[]{"Split Signal","Pressure Exchange","Twin Conduits",
        "Resonance Bridge","Reactor Duplex","The Last Relay"};
    constexpr const char* brief[]{
        "Deploy together. Player 1 climbs to the upper switch route, Player 2 advances below; hold relays to open each other's doors.",
        "Start together, split at the access lift and alternate the pressure switches as the other operative passes a locked door.",
        "Separate from the shared insertion bay: one operative routes power overhead while the other clears floor conduits.",
        "Climb from the shared starting deck into separate gantries. Trade switch and gate duties at every relay.",
        "Start side by side, then coordinate upper coolant controls with floor pressure doors while fighting the guard patrols.",
        "Deploy as a pair and split routes: each operative must hold alternating uplink switches to guide the other forward."
    };
    constexpr const char* namesBoss[]{"Relay Warden","Flood Sentinel","Circuit Regent",
        "Gantry Marshal","Core Guardian","Helix Overseer"};
    constexpr const char* arts[]{"terminus","tideworks","descent","foundry","tideworks","terminus"};
    constexpr const char* accents[]{"#7adfff","#64f4ce","#e0b0ff","#ffbc75","#f8856b","#da91ff"};
    constexpr const char* cold[]{"#76b9ff","#4a9ab8","#8596f9","#e4a95c","#9b89dd","#8ae7ff"};
    constexpr Biome biomes[]{Biome::orbital,Biome::water,Biome::mine,Biome::forge,Biome::water,Biome::orbital};
    std::array<Stage,cooperativeSectorCount> result{};
    for (int sector=0;sector<cooperativeSectorCount;++sector) {
        Stage s{};
        s.name=names[sector]; s.art=arts[sector]; s.location="Dual-route coordination facility";
        s.threat=sector<2?"HIGH":sector<4?"SEVERE":"CRITICAL";
        s.accent=accents[sector]; s.cold=cold[sector]; s.briefing=brief[sector];
        s.boss=namesBoss[sector]; s.biome=biomes[sector];
        const float bottom=512.f, upper=326.f;
        const int gates=4+sector; const float pitch=970.f+sector*38.f;
        s.worldWidth=1530.f+gates*pitch;
        s.exit={s.worldWidth-112.f,bottom};
        // A shared flat landing bay allows both players to deploy together.
        // The fork is just ahead: Player 1 ascends; Player 2 stays on the floor.
        s.ground.push_back({0,bottom,s.worldWidth,100});
        constexpr float fork=400.f;
        // A deterministic set of one-way steps is always usable, even if the
        // nearby moving elevator is at the far end of its travel cycle.
        s.ledges.push_back({196,bottom-61.f,103,17});
        s.ledges.push_back({292,bottom-119.f,107,17});
        s.stairs.push_back({{107.f,bottom},{225.f,bottom-61.f}});
        s.stairs.push_back({{245.f,bottom-61.f},{324.f,bottom-119.f}});
        s.stairs.push_back({{351.f,bottom-119.f},{429.f,upper}});
        // Keep the overhead course separate from the shared ground start.
        // Short seams along the upper gantry reward jumps and teamwork.
        for (float x=fork;x<s.worldWidth-400.f;x+=600.f) {
            const float segment=std::min(600.f,s.worldWidth-400.f-x);
            s.ledges.push_back({x,upper,std::max(30.f,segment-(x>fork?68.f:0.f)),19});
        }
        s.ledges.push_back({s.worldWidth-300.f,upper-88.f,140,16});
        s.ledges.push_back({s.worldWidth-480.f,upper-122.f,110,16});
        // A lift supplements the stairs; later lifts and re-entry steps make
        // falling from the upper path recoverable rather than a softlock.
        for (float x=300.f;x<s.worldWidth-250.f;x+=1320.f)
            s.lifts.push_back({x,bottom-27.f,93.f,0.f,-167.f,5.1f,float(sector)*.4f});
        s.cover.push_back({620,bottom-43.f,44,43});
        s.supplies.push_back({SupplyKind::ammo,160,bottom});
        s.supplies.push_back({SupplyKind::health,455,upper});
        for (int j=0;j<gates;++j) {
            const float x=790.f+j*pitch;
            const int operatorPlayer=j%2==0?0:1;
            const float gateFloor=operatorPlayer==0?bottom:upper;
            const float padFloor=operatorPlayer==0?upper:bottom;
            const RelayControl control=j%3==1?RelayControl::lever:RelayControl::pressurePlate;
            s.relays.push_back({{x,gateFloor-(operatorPlayer==0?205.f:157.f),52.f,
                                 operatorPlayer==0?205.f:157.f},{x-275.f,padFloor},operatorPlayer,control});
            // Switches and levers always have solid footing on either route.
            if (operatorPlayer==0) s.ledges.push_back({x-343.f,upper,144.f,17});
            s.ledges.push_back({x-225.f,upper-86.f,120,16});
            if ((j+sector)%2==0) s.ledges.push_back({x+145.f,upper-145.f,116.f,17});
            if (sector>=2 && j%2==0) s.lifts.push_back({x+395.f,bottom-89.f,108.f,34.f,-95.f,
                                                       3.6f+sector*.15f,float(j)*.55f});
            if (j>0) {
                s.cover.push_back({x-370.f,bottom-43.f,38.f,43.f});
                s.crates.push_back({j%3==0?CrateKind::fuel:CrateKind::charge,{x-160.f,bottom}});
            }
            if (j%2==0) s.enemies.push_back({j>=4?EnemyKind::bulwark:EnemyKind::ranger,
                                                 x+156.f,bottom,85.f,j>=3});
            else s.enemies.push_back({EnemyKind::sniper,x+178.f,bottom,55.f,j>=3});
            if (sector>=2) s.enemies.push_back({j%3==0?EnemyKind::stalker:EnemyKind::scavenger,
                                               x+420.f,bottom,65.f,j>=4});
            s.enemies.push_back({j>=3?EnemyKind::interceptor:EnemyKind::drone,
                                 x+270.f,upper-60.f,0.f,j>=4});
            s.supplies.push_back({j%2?SupplyKind::health:SupplyKind::ammo,x+340.f,bottom});
            if (j%2==0) s.supplies.push_back({SupplyKind::salvage,x+330.f,upper});
            if (j==1 || (j>=3 && j%2==1)) {
                const float checkpointX=x+355.f;
                s.beacons.push_back({checkpointX,bottom});
                // Checkpoint retries reunite the pair on the floor. Give the
                // upper-route player a reliable way back to their gantry.
                s.ledges.push_back({checkpointX+45.f,bottom-59.f,99.f,17});
                s.ledges.push_back({checkpointX+143.f,bottom-117.f,99.f,17});
                s.ledges.push_back({checkpointX+241.f,upper,103.f,17});
            }
            if (sector>=2 && j%3==1) s.vents.push_back({x+460.f,bottom,float(j)*.4f});
        }
        s.beacons.push_back({s.worldWidth-440.f,bottom});
        // This final checkpoint is a shared safe gathering place before boss.
        s.cover.push_back({s.worldWidth-500.f,bottom-43.f,45.f,43.f});
        s.supplies.push_back({SupplyKind::health,s.worldWidth-545.f,bottom});
        s.enemies.push_back({EnemyKind::boss,s.worldWidth-345.f,bottom,140.f,sector>=4});
        // Co-op guardian encounters also have open final arenas. Keep all
        // relay gates and their original offsets unchanged.
        const float originalEnd=s.worldWidth;
        constexpr float extra=320.f;
        s.ground.back().w+=extra;
        s.worldWidth+=extra;
        s.exit.x+=extra;
        const float arenaLeft=s.worldWidth-1120.f;
        for (int side=0;side<2;++side) {
            const float base=arenaLeft+120.f+side*710.f;
            const float rise=side==0?88.f:116.f;
            s.ledges.push_back({base+109.f,bottom-rise,148.f,17.f});
            s.stairs.push_back({{base,bottom},{base+148.f,bottom-rise}});
        }
        s.enemies.back().x=s.worldWidth-430.f;
        s.enemies.back().patrol=450.f;
        std::erase_if(s.cover,[&](const Rect& wall) {
            return wall.x>originalEnd-930.f && wall.y>=bottom-85.f;
        });
        std::erase_if(s.stairs,[&](const Stair& stair){return exitStairConflict(stair,s.exit);});
        result[sector]=std::move(s);
    }
    return result;
}();
}
