// Headless round-trip checks for the file types accepted by Model Studio.
#include "content/stages.hpp"
#include "content/level_io.hpp"
#include "content/model_io.hpp"
#include "content/runtime_model.hpp"
#include "content/enemy_runtime_model.hpp"
#include "../editor/art.hpp"
#include "../editor/workspace.hpp"
#include "../editor/rig_view.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    namespace fs=std::filesystem;
    using namespace vl;
    const auto folder=fs::temp_directory_path()/"voidline-editor-107-io-tests";
    fs::remove_all(folder);
    fs::create_directories(folder);
    std::string error;
    const auto one=folder/"opening-copy.vllevel.json";
    assert(writeLevel(one,stages[0],0,false,&error));
    Stage copy{};assert(readLevel(one,stages[0],copy,0,false,&error));
    assert(copy.worldWidth==stages[0].worldWidth && copy.stairs.size()==stages[0].stairs.size());
    const auto coop=folder/"coop-copy.vllevel.json";
    assert(writeLevel(coop,cooperativeStages[2],2,true,&error));
    Stage joint{};assert(readLevel(coop,cooperativeStages[2],joint,2,true,&error));
    assert(joint.relays.size()==cooperativeStages[2].relays.size());
    const auto player=folder/"player-copy.vlcharacter.json";
    const auto opponent=folder/"enemy-copy.vlenemy.json";
    const auto fingerprintBefore=modelFingerprint();
    characterSkeletons[3].offsets[0][static_cast<size_t>(SkeletonJoint::head)]={4.f,-3.f};
    characterSkeletons[3].offsets[1][static_cast<size_t>(SkeletonJoint::nearHand)]={6.f,-5.f};
    characterSkeletons[3].offsets[2][static_cast<size_t>(SkeletonJoint::nearKnee)]={-3.f,2.f};
    characterSkeletons[3].stride=1.15f;
    assert(modelFingerprint()!=fingerprintBefore);
    assert(saveCharacterModelAt(player,3,&error));
    characterSkeletons[3]={};
    assert(loadCharacterModelAt(player,3,&error));
    assert(characterSkeletons[3].offsets[0][static_cast<size_t>(SkeletonJoint::head)].x==4.f);
    assert(characterSkeletons[3].offsets[1][static_cast<size_t>(SkeletonJoint::nearHand)].y==-5.f);
    assert(characterSkeletons[3].offsets[2][static_cast<size_t>(SkeletonJoint::nearKnee)].x==-3.f);
    assert(characterSkeletons[3].stride==1.15f);
    characterSkeletons[3].depths[2][static_cast<size_t>(SkeletonJoint::nearHand)]=6.f;
    characterSkeletons[3].bodyWidth=1.25f;
    characterSkeletons[3].bodyHeight=1.4f;
    characterSkeletons[3].acceleration=1.3f;
    characterSkeletons[3].weaponSeat=4.f;
    assert(saveCharacterModelAt(player,3,&error));
    characterSkeletons[3]={};
    assert(loadCharacterModelAt(player,3,&error));
    assert(characterSkeletons[3].depths[2][static_cast<size_t>(SkeletonJoint::nearHand)]==6.f);
    assert(characterSkeletons[3].bodyWidth==1.25f);
    assert(characterSkeletons[3].bodyHeight==1.4f);
    assert(characterSkeletons[3].acceleration==1.3f);
    assert(characterSkeletons[3].weaponSeat==4.f);
    const auto baked=runtime_model::encode(characterSkeletons);
    assert(baked.size()==20+vl::runtime_model::payloadSize);
    std::array<SkeletonProfile,6> parsed{};
    assert(runtime_model::decode(baked,parsed,&error));
    assert(parsed[3].bodyWidth==characterSkeletons[3].bodyWidth);
    assert(parsed[3].depths[2][static_cast<size_t>(SkeletonJoint::nearHand)]==6.f);
    // VLRM v1 compatibility: existing games can be imported without losing XY.
    std::vector<uint8_t> legacy{'V','L','R','M'};
    runtime_model::writeU32(legacy,1);
    runtime_model::writeU32(legacy,runtime_model::modelCount);
    runtime_model::writeU32(legacy,static_cast<uint32_t>(runtime_model::legacyPayloadSize));
    runtime_model::writeU32(legacy,0);
    for(const auto& m:characterSkeletons){
        const auto floats=runtime_model::flatten(m);
        for(size_t i=0;i<14;++i)runtime_model::writeU32(legacy,std::bit_cast<uint32_t>(floats[i]));
        for(size_t i=14;i<floats.size();i+=3){
            runtime_model::writeU32(legacy,std::bit_cast<uint32_t>(floats[i]));
            runtime_model::writeU32(legacy,std::bit_cast<uint32_t>(floats[i+1]));
        }
    }
    const auto sum=runtime_model::checksum(std::span(legacy).subspan(20));
    for(int i=0;i<4;++i)legacy[16+i]=static_cast<uint8_t>(sum>>(8*i));
    std::array<SkeletonProfile,6> fromOld{};
    assert(runtime_model::decode(legacy,fromOld,&error));
    assert(fromOld[3].depths[2][static_cast<size_t>(SkeletonJoint::nearHand)]==0.f);
    assert(fromOld[3].offsets[2][static_cast<size_t>(SkeletonJoint::nearHand)].x==characterSkeletons[3].offsets[2][static_cast<size_t>(SkeletonJoint::nearHand)].x);
    const vl::studio::Camera camera{};
    const auto before=vl::studio::joint(parsed[3],SkeletonJoint::nearHand,2,0);
    const auto next=vl::studio::joint(parsed[3],SkeletonJoint::nearHand,2,15);
    assert(std::abs(before.x-next.x)>1.f);
    assert(vl::studio::nextFrame(59)==0);
    auto point=parsed[3];
    vl::studio::moveJoint(point,0,8,vl::studio::dragDelta(vl::studio::View::right,camera,27,0));
    assert(point.depths[0][8]>0.f);
    vl::studio::moveJoint(point,0,8,vl::studio::dragDelta(vl::studio::View::top,camera,0,-27));
    assert(point.depths[0][8] < 1.f);
    auto a=vl::studio::axes(vl::studio::View::orbit,camera);
    assert(std::abs(vl::studio::dot(a.right,a.down))<.0001f);

    const auto saved=parsed;
    auto corrupt=baked;corrupt[22]^=0x80;
    assert(!runtime_model::decode(corrupt,parsed,&error));
    assert(parsed[3].bodyWidth==saved[3].bodyWidth);
    assert(!runtime_model::decode(std::span(baked).first(60),parsed,&error));
    assert(!loadCharacterModelAt(player,2,&error));
    assert(saveEnemyModelAt(opponent,8,&error));
    assert(loadEnemyModelAt(opponent,8,&error));
    assert(!loadEnemyModelAt(opponent,9,&error));
    // The enemy model now shares the same editable Base/Aim/Run XYZ skeleton.
    enemySkeletons[8].offsets[2][static_cast<size_t>(SkeletonJoint::nearHand)]={4.f,-6.f};
    enemySkeletons[8].depths[1][static_cast<size_t>(SkeletonJoint::nearElbow)]=7.f;
    enemySkeletons[8].walkCycle=1.3f;
    const auto enemyFingerprint=modelFingerprint();
    assert(saveEnemyModelAt(opponent,8,&error));
    enemySkeletons[8]=SkeletonProfile{};
    assert(modelFingerprint()!=enemyFingerprint);
    assert(loadEnemyModelAt(opponent,8,&error));
    assert(enemySkeletons[8].offsets[2][static_cast<size_t>(SkeletonJoint::nearHand)].y==-6.f);
    assert(enemySkeletons[8].depths[1][static_cast<size_t>(SkeletonJoint::nearElbow)]==7.f);
    assert(enemySkeletons[8].walkCycle==1.3f);
    const auto packedEnemies=enemy_runtime_model::encode(enemySkeletons);
    std::array<SkeletonProfile,10> decodedEnemies{};
    assert(enemy_runtime_model::decode(packedEnemies,decodedEnemies,&error));
    assert(decodedEnemies[8].walkCycle==1.3f);
    auto badEnemies=packedEnemies;badEnemies.back()^=0x02;
    assert(!enemy_runtime_model::decode(badEnemies,decodedEnemies,&error));

    art::Raster pixels{};pixels.width=4;pixels.height=4;pixels.rgba.assign(4*4*4,0);
    for(size_t i=0;i<pixels.rgba.size();i+=4){pixels.rgba[i]=255;pixels.rgba[i+3]=255;}
    const auto png=folder/"paint-copy.png", webp=folder/"paint-copy.webp";
    for(const auto& file:{png,webp}){
        assert(art::encode(file,pixels,error));art::Raster decoded{};
        assert(art::decode(file,decoded,error));
        assert(decoded.width==4&&decoded.height==4&&decoded.rgba==pixels.rgba);
    }
    // The installed game archive is updated only by an explicit workspace export.
    const auto installed=folder/"installed game";
    const auto working=folder/"original assets";
    const auto pack=installed/"assets"/"game.vpak";
    assert(art::encode(working/"test.png",pixels,error));
    int packed=0;
    assert(art::repack(working,pack,packed,error) && packed==1);
    assert(fs::is_regular_file(pack));
    const auto sidecar=folder/"workspace cache";
    setenv("VOIDLINE_STUDIO_WORKSPACES",sidecar.c_str(),1);
    auto imported=workspace::prepare(installed);
    assert(imported.ok && !imported.existing && imported.count==1);
    auto reopened=workspace::prepare(installed);
    assert(reopened.ok && reopened.existing && reopened.project==imported.project);
    art::Raster revised=pixels;revised.rgba[1]=88;
    assert(art::encode(imported.project/"assets"/"test.png",revised,error));
    packed=0;
    assert(workspace::updatePak(installed,imported.project,packed,error) && packed==1);
    int validated=0;
    const auto extracted=folder/"unpacked verification";
    assert(art::unpack(pack,extracted,validated,error) && validated==1);
    art::Raster actual{};
    assert(art::decode(extracted/"test.png",actual,error));
    assert(actual.rgba==revised.rgba);
    int backups=0;
    for(const auto& item:fs::directory_iterator(pack.parent_path()))
        if(item.path().filename().string().find("game.vpak.backup-")==0)++backups;
    assert(backups==1);
    // Reject a workspace that's not associated with the target installation.
    assert(!workspace::updatePak(folder/"unrelated game",imported.project,packed,error));
    // Regression for the real editor -> PAK -> CMake-rebuild workflow: the
    // source asset tree must receive the edited binary as well as the runtime.
    const auto source=folder/"source game";
    fs::create_directories(source/"scripts");
    {std::ofstream file(source/"CMakeLists.txt");
     file<<"cmake_minimum_required(VERSION 3.16)\nproject(voidline VERSION 2.10.9 LANGUAGES CXX)\n";}
    {std::ofstream file(source/"scripts"/"pack-assets.py");file<<"# fake pack command\n";}
    fs::create_directories(source/"assets");
    assert(workspace::sourceFor(source/"build"/"dist"/"VOIDLINE")==source);
    int modified=0;
    assert(workspace::publishSourceAssets(imported.project,source,modified,error));
    assert(modified>=1);
    assert(fs::is_regular_file(source/"assets"/"test.png"));
    assert(fs::file_size(source/"assets"/"test.png")==fs::file_size(imported.project/"assets"/"test.png"));
    art::Raster again=pixels;again.rgba[1]=72;
    assert(art::encode(imported.project/"assets"/"test.png",again,error));
    modified=0;
    assert(workspace::publishSourceAssets(imported.project,source,modified,error));
    assert(modified==1);
    assert(art::decode(source/"assets"/"test.png",actual,error) && actual.rgba==again.rgba);
    assert(fs::is_directory(source/"update-backups"));
    // The standard game packer reads source/assets; the modified file will
    // therefore survive the next build rather than reverting to stock assets.
    assert(art::repack(source/"assets",source/"rebuild"/"game.vpak",packed,error));
    assert(art::unpack(source/"rebuild"/"game.vpak",extracted,validated,error));
    assert(art::decode(extracted/"test.png",actual,error) && actual.rgba==again.rgba);
    // Source checkouts without game.vpak are directly authorable. The editor
    // automatically detects assets/, saves a private cache, and never overwrites
    // edits on reopen. Runtime folders still resolve their actual PAKs.
    assert(workspace::validGame(source));
    const auto sourceResolved=workspace::resolve(source);
    assert(sourceResolved.valid && sourceResolved.fromSource && sourceResolved.pack.empty());
    assert(workspace::resolve(source/"build"/"voidline").fromSource);
    auto sourceWorkspace=workspace::prepare(source);
    assert(sourceWorkspace.ok && sourceWorkspace.fromSource && sourceWorkspace.count==1);
    auto sameSource=workspace::prepare(source);
    assert(sameSource.ok && sameSource.existing && sameSource.project==sourceWorkspace.project);
    assert(workspace::resolve(source/"assets").fromSource);
    assert(!workspace::updatePak(source,sourceWorkspace.project,packed,error));
    const auto built=source/"build"/"voidline";
    fs::create_directories(built/"assets");
    {std::ofstream binary(built/"voidline");binary<<"test game";}
    assert(art::repack(source/"assets",built/"assets"/"game.vpak",packed,error));
    assert(workspace::resolve(built).valid && !workspace::resolve(built).fromSource);
    art::Raster newest=pixels;newest.rgba[2]=91;
    assert(art::encode(sourceWorkspace.project/"assets"/"test.png",newest,error));
    assert(workspace::publishSourceAssets(sourceWorkspace.project,source,modified,error) && modified==1);
    const auto staged=sourceWorkspace.project/"export"/"game.vpak";
    assert(art::repack(sourceWorkspace.project/"assets",staged,packed,error));
    std::vector<fs::path> copied;
    assert(workspace::syncRuntimePaks(staged,source,copied,error) && copied.size()==1);
    assert(copied[0]==fs::canonical(built/"assets"/"game.vpak"));
    assert(art::unpack(built/"assets"/"game.vpak",extracted,validated,error));
    assert(art::decode(extracted/"test.png",actual,error) && actual.rgba==newest.rgba);
    assert(!workspace::resolve(folder/"missing installation").valid);
    fs::remove_all(folder);
    std::cout<<"Workspace/file tests (including source-only load/publish, runtime archive refresh and backups) passed\n";
    return 0;
}
