#include "support.hpp"
#include "content/level_io.hpp"
#include "content/model_io.hpp"
#include "content/skeleton.hpp"
#include "content/runtime_model.hpp"
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
            CHECK(loaded.relays.size()==input.relays.size());
            CHECK(levelFingerprint(loaded)==levelFingerprint(input));
            CHECK(loaded.insertion.x==110.f);
        }
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
