#include "content/stages.hpp"
#include "content/opening_sectors.hpp"
#include <utility>

namespace vl {
namespace {
enum class Crossing { span, stairs, spillway, lift, catwalk, conduit, fracture, ferry };
enum class Garrison { infantry, swarm, sentries, hunters, crossfire };
struct Bay { Crossing crossing; Garrison garrison; float floor; };
struct Mission {
    const char *name,*location,*briefing,*guardian,*accent,*cold;
    Biome biome;
    std::vector<Bay> bays;
    float arenaFloor;
};
using enum Crossing;
using enum Garrison;
using enum Biome;

const std::array<Mission,sectorCount-5> missions{{
    {"Relay Annex","Maintenance network","Follow the service catwalks and take the access codes from the annex guardian.","Keymaster",
     "#8fe9ff","#7891e3",orbital,{{span,infantry,502},{stairs,swarm,487},{lift,sentries,466},{spillway,hunters,500},{catwalk,infantry,477}},484},
    {"Blackwater Lock","Lower floodgate","Cross the pump chamber and shut down the lock’s security battery.","Undertow",
     "#67e5bc","#4aa9d9",water,{{spillway,swarm,510},{lift,sentries,490},{stairs,hunters,464},{conduit,infantry,505},{lift,crossfire,482}},475},
    {"Root Vault","Agriculture archives","Take the raised trellises past the spore vents and breach the seed vault.","Taproot",
     "#b9ed80","#9c74e1",biodome,{{stairs,hunters,507},{catwalk,swarm,473},{spillway,infantry,502},{stairs,sentries,460},{conduit,crossfire,496}},483},
    {"Slag Run","Reactor outflow","Follow the slag channel through the heat vents. The lower ducts offer cover from the turret line.","Furnace Hound",
     "#ff9574","#efb654",forge,{{conduit,infantry,509},{spillway,sentries,480},{lift,crossfire,500},{fracture,hunters,476},{catwalk,swarm,468},{stairs,crossfire,495}},480},
    {"Faultline","Deep extraction shaft","Climb the stepped shaft before crossing the collapsed winch bridge.","Fault Keeper",
     "#ffd37f","#70b8d9",mine,{{stairs,infantry,510},{stairs,hunters,460},{fracture,sentries,427},{spillway,swarm,474},{lift,crossfire,504},{conduit,hunters,464}},478},
    {"Cold Array","Thermal radiator field","Use the radiator trusses as cover from the interceptor patrols and reach the array controls.","Whiteout",
     "#a7d7ff","#8388f1",orbital,{{catwalk,swarm,500},{spillway,sentries,469},{conduit,infantry,499},{ferry,crossfire,462},{stairs,hunters,485},{fracture,swarm,467}},488},
    {"Drowned Grid","Submerged power station","Ride the suspended platforms through the transformer pits and approach the core from above.","Riptide Engine",
     "#64f0e1","#6978d9",water,{{lift,sentries,510},{spillway,hunters,492},{ferry,swarm,468},{conduit,crossfire,500},{lift,infantry,465},{fracture,sentries,480},{catwalk,crossfire,492}},470},
    {"Sporeline","Containment gardens","Cross the broken grow racks and dodge stalkers in the low service ducts.","Mycelium",
     "#d5ed84","#d175bd",biodome,{{conduit,hunters,503},{catwalk,swarm,473},{stairs,sentries,438},{spillway,crossfire,487},{fracture,hunters,472},{conduit,infantry,502},{lift,crossfire,465}},485},
    {"Ash Circuit","Reactor coolant loop","Time the vent cycles and use the moving coolant lifts to cross the reactor loop.","Cinder King",
     "#ffb366","#f97b75",forge,{{lift,crossfire,507},{conduit,sentries,472},{ferry,swarm,482},{fracture,infantry,449},{spillway,hunters,504},{catwalk,crossfire,464},{lift,sentries,488}},477},
    {"Gravemill","Ore processing works","Descend through the mill, then climb the loader platforms past the armored garrison.","Stonebreaker",
     "#edd49b","#83a9be",mine,{{spillway,infantry,476},{conduit,hunters,509},{stairs,sentries,478},{stairs,crossfire,438},{fracture,swarm,460},{ferry,infantry,497},{catwalk,hunters,474},{conduit,crossfire,494}},480},
    {"Antenna Spine","Communications mast","Cross the moving antenna mounts and clear the gun line above the transmitter deck.","Signal Harrow",
     "#c2b2ff","#71d6ff",orbital,{{catwalk,sentries,503},{ferry,swarm,477},{stairs,infantry,442},{catwalk,crossfire,420},{spillway,hunters,466},{fracture,sentries,449},{lift,swarm,476},{conduit,crossfire,492}},477},
    {"Pressure Hull","Sea barrier controls","Cut through the flood barrier and cross two pump gaps before the control room.","Leviathan",
     "#83ffd2","#459dd4",water,{{conduit,infantry,509},{lift,sentries,489},{ferry,crossfire,457},{spillway,swarm,500},{lift,hunters,467},{fracture,crossfire,481},{ferry,sentries,461},{catwalk,swarm,479},{conduit,infantry,500}},472},
    {"Glass Garden","Exobiology containment","Find openings between the trellis turrets and keep moving through the stalker nests.","Crown of Thorns",
     "#c5f48e","#ce83ee",biodome,{{stairs,hunters,511},{catwalk,swarm,471},{conduit,crossfire,501},{fracture,hunters,482},{stairs,sentries,442},{ferry,swarm,470},{spillway,infantry,500},{conduit,hunters,489},{catwalk,crossfire,458}},484},
    {"Sunken Crucible","Emergency smelter","Jump the fractured slag decks and cross the furnace lifts into the smelter’s last chamber.","Kiln Titan",
     "#ff815d","#f9ca73",forge,{{fracture,sentries,502},{lift,crossfire,466},{conduit,hunters,498},{ferry,swarm,469},{stairs,infantry,435},{spillway,crossfire,482},{lift,sentries,462},{fracture,hunters,481},{conduit,crossfire,506}},476},
    {"Last Transmission","Defense command core","Take the command core past the defense platforms. Its guardian controls the emergency uplink.","Helix Sovereign",
     "#ef9cff","#8ae7ff",orbital,{{conduit,infantry,505},{fracture,crossfire,478},{ferry,swarm,458},{catwalk,sentries,425},{spillway,hunters,483},{lift,crossfire,460},{conduit,infantry,492},{fracture,sentries,451},{ferry,swarm,470},{catwalk,crossfire,445}},478}
}};

constexpr float crossingWidth(Crossing crossing) {
    switch (crossing) {
    case span: return 180;
    case stairs: return 214;
    case spillway: return 257;
    case lift: return 328;
    case catwalk: return 296;
    case conduit: return 201;
    case fracture: return 368;
    case ferry: return 419;
    }
    return 0;
}

void layCrossing(Stage& stage,Crossing crossing,float edge,float from,float to,int rank) {
    const float upper=std::min(from,to);
    const float phase=rank*.37f;
    auto ledge=[&](float x,float y,float w) { stage.ledges.push_back({edge+x,y,w,17}); };
    auto platform=[&](float x,float y,float w,float dx,float dy,float period) {
        stage.lifts.push_back({edge+x,y,w,dx,dy,period,phase});
    };
    switch (crossing) {
    case span:
        ledge(53,upper-44,86);
        break;
    case stairs:
        ledge(-68,from-88,120);
        ledge(90,upper-104,98);
        ledge(206,to-79,113);
        break;
    case spillway:
        ledge(65,upper+35,92);
        ledge(195,upper-51,100);
        break;
    case lift:
        ledge(-67,from-75,115);
        platform(112,upper-62,104,37,50,2.7f);
        ledge(268,to-64,106);
        break;
    case catwalk:
        ledge(-86,from-104,130);
        ledge(106,upper-118,117);
        ledge(264,to-95,131);
        break;
    case conduit:
        // The maintenance ceiling leaves room for a crouching operative.
        ledge(-237,from-60,120);
        ledge(67,upper-26,101);
        ledge(220,to-89,101);
        break;
    case fracture:
        ledge(-50,from-66,95);
        ledge(130,upper-24,83);
        ledge(274,to-83,110);
        break;
    case ferry:
        ledge(-77,from-84,109);
        platform(131,upper-62,108,57,39,3.15f);
        ledge(303,to-98,140);
        break;
    }
}

void populateBay(Stage& stage,const Bay& bay,float x,float w,int rank,int number,int reinforcements) {
    const float floor=bay.floor;
    const bool elite=rank>=2 && (number+rank)%3!=0;
    const float deck=x+w*.48f-64;
    const float barricade=x+w*(number==0?.20f:.27f)-17;
    const float deckY=floor-(bay.crossing==catwalk?124.f:100.f);
    stage.ledges.push_back({deck,deckY,128,17});
    if (bay.crossing==catwalk || bay.crossing==stairs)
        stage.ledges.push_back({x+w*.18f,floor-78,98,17});
    if (number>0 || rank>=2) stage.cover.push_back({barricade,floor-43,45,43});
    const auto crate=rank>=9 && number%4==2?CrateKind::reactor
        :rank>=2 && number%3!=0?CrateKind::fuel:CrateKind::charge;
    stage.crates.push_back({crate,{x+w*.61f,floor}});
    if (rank>=7 && number%3==1) stage.crates.push_back({CrateKind::charge,{x+w*.80f,floor}});
    auto enemy=[&](EnemyKind kind,float at,float y,float patrol=0,bool veteran=false) {
        stage.enemies.push_back({kind,x+w*at,y,patrol,veteran});
    };
    using enum EnemyKind;
    switch (bay.garrison) {
    case infantry:
        enemy(ranger,.31f,floor,48);
        enemy(ranger,.76f,floor,51,elite);
        break;
    case swarm:
        enemy(drone,.33f,deckY-56,0,elite);
        enemy(drone,.75f,floor-77);
        break;
    case sentries:
        enemy(turret,.48f,deckY,0,elite);
        enemy(ranger,.75f,floor,55);
        break;
    case hunters:
        enemy(stalker,.44f,floor,55,elite);
        enemy(stalker,.76f,floor,53);
        break;
    case crossfire:
        enemy(turret,.48f,deckY,0,elite);
        enemy(stalker,.72f,floor,59,rank>=9);
        break;
    }
    if (bay.garrison==swarm) enemy(ranger,.53f,floor,53,rank>=7);
    else enemy(drone,.63f,deckY-59,0,rank>=6 && number%2==0);
    if (reinforcements>=1) {
        if (bay.garrison==swarm) enemy(turret,.48f,deckY,0,rank>=8);
        else enemy(bay.garrison==hunters?stalker:ranger,.91f,floor,28,rank>=6);
    }
    if (reinforcements>=2) enemy(drone,.86f,deckY-122,0,rank>=9);
    if (reinforcements>=3) enemy(turret,.55f,floor,0,rank>=12);

    stage.supplies.push_back({SupplyKind::ammo,x+w*.07f,floor});
    stage.supplies.push_back({number%2==0?SupplyKind::health:SupplyKind::salvage,
                              number%2==0?x+w*.82f:deck+91,number%2==0?floor:deckY});
    if ((rank>=4 || stage.biome==forge) && number%3!=0)
        stage.vents.push_back({x+w*.88f,floor,std::fmod((number+rank)*.79f,3.65f)});
}

Stage makeMission(const Mission& mission,int rank) {
    constexpr const char* art[]{"descent","tideworks","overgrowth","foundry","terminus"};
    const int count=static_cast<int>(mission.bays.size());
    const float length=4820.f+435.f*rank;
    const float arena=790.f+12.f*rank;
    float walkable=length-arena;
    for (const auto& bay:mission.bays) walkable-=crossingWidth(bay.crossing);
    const float bayWidth=walkable/count;
    Stage stage{mission.name,art[static_cast<int>(mission.biome)],mission.location,
        rank<4?"HIGH":rank<8?"SEVERE":rank<12?"EXTREME":"CRITICAL",
        mission.accent,mission.cold,mission.briefing,mission.guardian,mission.biome,
        length,{length-105,mission.arenaFloor},{},{},{},{},{},{},{}};
    float x=0;
    const int checkpoints=rank<7?2:3;
    const int escorts=int(rank>=4)+int(rank>=8)+int(rank>=12);
    const int reinforcements=19+rank*2-count*3-escorts-1;
    for (int i=0;i<count;++i) {
        const auto& bay=mission.bays[i];
        const float w=bayWidth+(i==0?80.f:i==count-1?-80.f:0.f);
        stage.ground.push_back({x,bay.floor,w,110});
        const int bayOrder=(i+rank)%count;
        const int extra=reinforcements/count+int(bayOrder<reinforcements%count);
        populateBay(stage,bay,x,w,rank,i,extra);
        for (int j=1;j<checkpoints;++j) {
            if (i==count*j/checkpoints) stage.beacons.push_back({x+36,bay.floor});
        }
        if (i==count-1) stage.beacons.push_back({x+w-61,bay.floor});
        layCrossing(stage,bay.crossing,x+w,bay.floor,
                    i+1<count?mission.bays[i+1].floor:mission.arenaFloor,rank+i);
        x+=w+crossingWidth(bay.crossing);
    }

    const float floor=mission.arenaFloor;
    stage.ground.push_back({x,floor,length-x,110});
    stage.ledges.push_back({x+106,floor-92,135,17});
    if (rank>=3) stage.ledges.push_back({x+282,floor-145,116,17});
    stage.cover.push_back({x+255,floor-43,46,43});
    if (rank>=7) stage.cover.push_back({x+arena*.81f-64,floor-43,46,43});
    stage.crates.push_back({rank>=9?CrateKind::reactor:CrateKind::fuel,{x+arena*.57f,floor}});
    stage.supplies.push_back({SupplyKind::health,x+60,floor});
    stage.supplies.push_back({SupplyKind::ammo,x+190,floor});
    if (rank>=4) stage.enemies.push_back({EnemyKind::ranger,x+210,floor,53,rank>=8});
    if (rank>=8) stage.enemies.push_back({EnemyKind::drone,x+332,floor-210,0,rank>=11});
    if (rank>=12) stage.enemies.push_back({EnemyKind::stalker,x+375,floor,45,true});
    stage.enemies.push_back({EnemyKind::boss,x+arena*.68f,floor,arena*.19f});
    return stage;
}
}

const std::array<Stage,sectorCount> stages=[] {
    std::array<Stage,sectorCount> campaign{};
    auto opening=campaign::openingSectors();
    for (size_t i=0;i<opening.size();++i) {
        auto& stage=opening[i];
        const auto& first=stage.ground.front();
        const auto& middle=stage.ground[stage.ground.size()/2-1];
        const auto& arena=stage.ground.back();
        stage.cover={{first.right()-95,first.y-43,44,43},
                     {middle.x+75,middle.y-43,46,43},
                     {arena.x+116,arena.y-43,46,43}};
        stage.crates={{CrateKind::charge,{first.right()-28,first.y}},
                      {i>=1?CrateKind::fuel:CrateKind::charge,{middle.x+320,middle.y}},
                      {i>=3?CrateKind::reactor:CrateKind::fuel,{stage.exit.x-235,arena.y}}};
        campaign[i]=std::move(stage);
    }
    for (size_t i=0;i<missions.size();++i) campaign[opening.size()+i]=makeMission(missions[i],static_cast<int>(i));
    return campaign;
}();
}
