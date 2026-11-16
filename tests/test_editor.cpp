#include "support.hpp"
#include "content/level_io.hpp"
#include "content/model_io.hpp"
#include "content/skeleton.hpp"
#include "content/runtime_model.hpp"
#include "content/enemy_motion.hpp"
#include "content/enemy_runtime_model.hpp"
#include "game/stairs.hpp"
#include "game/enemy_ground.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>

using namespace vl;
using namespace test;
namespace fs=std::filesystem;

void runEditorTests() {
    const auto folder=fs::temp_directory_path()/("voidline-editor-roundtrip-"+std::to_string(std::time(nullptr)));
    std::error_code ec;
    fs::create_directories(folder,ec);
    CHECK(!ec);
    // The editor can import/export EVERY built-in campaign and co-op mission,
    // including the dynamic terrain, relay switches, walkable stairs and foes.
    for (int coop=0;coop<2;++coop) {
        const int count=coop?cooperativeSectorCount:sectorCount;
        for (int i=0;i<count;++i) {
            Stage input=coop?cooperativeStages[i]:stages[i];
            input.insertion={110,input.ground.front().y};
            const auto file=levelFile(folder/"levels",i,coop!=0);
            std::string error;
            CHECK(writeLevel(file,input,i,coop!=0,&error));
            Stage loaded{};
            CHECK(readLevel(file,input,loaded,i,coop!=0,&error));
            CHECK(loaded.worldWidth==input.worldWidth);
            CHECK(loaded.enemies.size()==input.enemies.size());
            CHECK(loaded.stairs.size()==input.stairs.size());
            CHECK(loaded.ladders.size()==input.ladders.size());
            CHECK(loaded.relays.size()==input.relays.size());
            CHECK(levelFingerprint(loaded)==levelFingerprint(input));
            CHECK(loaded.insertion.x==110.f);
        }
    }
    // Ladder edits must round-trip through the same source JSON and change the
    // multiplayer compatibility fingerprint, without an editor-only cache.
    {
        Stage input=stages[0];
        input.ladders={{275.f,350.f,500.f}};
        const auto file=levelFile(folder/"levels",0,false);
        std::string error;
        CHECK(writeLevel(file,input,0,false,&error));
        Stage output{};
        CHECK(readLevel(file,stages[0],output,0,false,&error));
        CHECK(output.ladders.size()==1);
        CHECK(output.ladders[0].x==275.f);
        CHECK(levelFingerprint(input)==levelFingerprint(output));
        CHECK(levelFingerprint(input)!=levelFingerprint(stages[0]));
    }
    // Optional user-authored angled structure outline persists through the
    // level model and affects its compatibility hash (collision stays rectangular).
    {
        Stage input=cooperativeStages[1];
        CHECK(!input.cover.empty());
        const Rect cover=input.cover.front();
        StructurePolygon poly{};
        poly.layer=2;poly.index=0;
        poly.corners={{cover.x,cover.y},{cover.x+cover.w,cover.y},
                      {cover.x+cover.w+8,cover.y+cover.h*.5f},
                      {cover.x+cover.w,cover.y+cover.h},{cover.x,cover.y+cover.h}};
        input.structurePolygons.push_back(poly);
        const auto file=levelFile(folder/"levels",1,true);
        std::string error;
        CHECK(writeLevel(file,input,1,true,&error));
        Stage roundtrip{};
        CHECK(readLevel(file,cooperativeStages[1],roundtrip,1,true,&error));
        CHECK(roundtrip.structurePolygons.size()==1);
        CHECK(roundtrip.structurePolygons[0].corners.size()==5);
        CHECK(levelFingerprint(input)==levelFingerprint(roundtrip));
        CHECK(levelFingerprint(roundtrip)!=levelFingerprint(cooperativeStages[1]));
    }
    // Author action vectors blend on top of the legacy base/aim/run rig;
    // an archive without action layers must preserve its previous appearance.
    {
        SkeletonProfile profile{};
        profile.offsets[0][3]={2.f,-3.f};
        profile.offsets[2][3]={4.f,-1.f};
        profile.actionOffsets[static_cast<size_t>(ActionPose::walk)][3]={10.f,-6.f};
        const auto idle=skeletonOffset(profile,SkeletonJoint::head,0.f,.5f);
        CHECK(idle.x==4.f && idle.y==-3.5f);
        const auto walking=skeletonOffset(profile,SkeletonJoint::head,0.f,.5f,ActionPose::walk,.5f);
        CHECK(walking.x==9.f && walking.y==-6.5f);
        CHECK(skeletonOffset(profile,SkeletonJoint::head,0.f,.5f,ActionPose::walk,0.f).x==idle.x);
    }
    // Client/server compatibility must also detect user-published action layers.
    {
        const auto old=characterSkeletons[2].actionOffsets[static_cast<size_t>(ActionPose::walk)][3];
        const uint32_t before=modelFingerprint();
        characterSkeletons[2].actionOffsets[static_cast<size_t>(ActionPose::walk)][3].x+=3;
        CHECK(modelFingerprint()!=before);
        characterSkeletons[2].actionOffsets[static_cast<size_t>(ActionPose::walk)][3]=old;
        CHECK(modelFingerprint()==before);
    }
    // One character file and one enemy file per archetype. Edits affect the
    // simulation values and change the multiplayer compatibility fingerprint.
    const auto originalCharacters=characters;
    const auto originalSpecs=specs;
    const auto originalHash=modelFingerprint();
    std::string error;
    for(int i=0;i<int(characters.size());++i)CHECK(saveCharacterModel(folder/"models",i,&error));
    for(int i=0;i<int(specs.size());++i)CHECK(saveEnemyModel(folder/"models",i,&error));
    characters[4].speed+=20;
    specs[3].speed+=15;
    CHECK(modelFingerprint()!=originalHash);
    for(int i=0;i<int(characters.size());++i)CHECK(loadCharacterModel(folder/"models",i,&error));
    for(int i=0;i<int(specs.size());++i)CHECK(loadEnemyModel(folder/"models",i,&error));
    CHECK(characters[4].speed==originalCharacters[4].speed);
    CHECK(specs[3].speed==originalSpecs[3].speed);
    CHECK(modelFingerprint()==originalHash);
    characters=originalCharacters;
    specs=originalSpecs;
    // Authoring parameters bake into a compact, checksummed runtime format.
    const auto rigsBefore=characterSkeletons;
    characterSkeletons[2].bodyWidth=1.35f;
    characterSkeletons[2].headScale=.85f;
    characterSkeletons[2].acceleration=1.2f;
    characterSkeletons[2].offsets[1][static_cast<size_t>(SkeletonJoint::nearHand)]={4,-3};
    const auto baked=runtime_model::encode(characterSkeletons);
    CHECK(baked.size()==20+runtime_model::payloadSize);
    std::array<SkeletonProfile,6> unpacked{};
    CHECK(runtime_model::decode(baked,unpacked,&error));
    CHECK(unpacked[2].bodyWidth==1.35f && unpacked[2].headScale==.85f);
    CHECK(unpacked[2].acceleration==1.2f && unpacked[2].offsets[1][8].x==4.f);
    auto corrupt=baked;
    corrupt.back()^=0x7;
    CHECK(!runtime_model::decode(corrupt,unpacked,&error));
    CHECK(!runtime_model::decode(std::span(baked).first(120),unpacked,&error));
    const auto originalJoint=modelJoint(characterSkeletons[2],SkeletonJoint::head);
    characterSkeletons[2].bodyHeight=1.5f;
    CHECK(modelJoint(characterSkeletons[2],SkeletonJoint::head).y<originalJoint.y);
    characterSkeletons=rigsBefore;
    CHECK(modelFingerprint()==originalHash);
    // Ten baked enemy motion rigs must round-trip and affect the multiplayer hash.
    const auto enemyBefore=enemySkeletons;
    enemySkeletons[7].offsets[2][8]={5.f,-2.f};
    enemySkeletons[7].depths[1][8]=6.f;
    enemySkeletons[7].acceleration=1.3f;
    CHECK(modelFingerprint()!=originalHash);
    const auto enemyData=enemy_runtime_model::encode(enemySkeletons);
    std::array<SkeletonProfile,10> decodedEnemy{};
    CHECK(enemy_runtime_model::decode(enemyData,decodedEnemy,&error));
    CHECK(decodedEnemy[7].offsets[2][8].x==5.f);
    CHECK(decodedEnemy[7].depths[1][8]==6.f);
    CHECK(decodedEnemy[7].acceleration==1.3f);
    auto damagedEnemy=enemyData;damagedEnemy.back()^=0x02;
    CHECK(!enemy_runtime_model::decode(damagedEnemy,decodedEnemy,&error));
    enemySkeletons=enemyBefore;
    CHECK(modelFingerprint()==originalHash);
    // Reject malformed geometry and invalid model tuning, not just bad JSON.
    Stage level=stages[0];
    level.stairs.push_back({{100,500},{110,100}});
    CHECK(writeLevel(levelFile(folder/"levels",0,false),level,0,false,&error));
    Stage rejected{};
    CHECK(!readLevel(levelFile(folder/"levels",0,false),stages[0],rejected,0,false,&error));
    // A ground enemy can walk UP a staircase and reach the upper landing.
    Stage slope{};
    slope.ground={{0,500,195,110}};
    slope.ledges={{342,390,230,17}};
    slope.stairs.push_back({{184,500},{350,390}});
    CHECK(validStair(slope.stairs.front()));
    Enemy e{};
    e.kind=EnemyKind::ranger;e.box={150,446,35,54};e.base=500;
    float x=e.box.x;
    for(int i=0;i<55;++i){
        const float next=groundedEnemyX(slope,e,x,x+4);
        CHECK(next>=x-0.001f);
        x=next;e.box.x=x;
        if(auto y=stairFooting(slope,e.box.center().x,e.base,19.f)) e.base=*y;
    }
    CHECK(x>295.f);
    CHECK(e.base<460.f);
    fs::remove_all(folder,ec);
}
