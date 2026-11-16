// VOIDLINE Model Studio headless model/archive backend.
// The desktop application UI uses the Linux Tk/ttk toolkit (ui/model_studio.py).
#include "content/stages.hpp"
#include "content/level_io.hpp"
#include "content/model_io.hpp"
#include "content/runtime_model.hpp"
#include "content/enemy_runtime_model.hpp"
#include "art.hpp"
#include "workspace.hpp"
#include "source_export.hpp"
#include <filesystem>
#include <json-c/json.h>
#include <fstream>
#include <chrono>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace fs=std::filesystem;
namespace {
using namespace vl;

// Restore editable action layers from an installed PAK when authoring files are
// initially created. Never overwrite files already edited in this workspace.
bool restoreActions(const fs::path& project,const fs::path& authorFile,int id,bool enemy,std::string& error){
    const auto packed=project/"assets"/"models"/(enemy?"enemies.actions.json":"characters.actions.json");
    if(!fs::is_regular_file(packed))return true;
    json_object* catalog=json_object_from_file(packed.string().c_str());
    if(!catalog){error="Invalid packed action catalog: "+packed.string();return false;}
    json_object *models=nullptr,*entry=nullptr,*sequences=nullptr,*depths=nullptr;
    const bool ok=json_object_object_get_ex(catalog,"models",&models)&&
                  json_object_is_type(models,json_type_array)&&
                  id<int(json_object_array_length(models))&&
                  (entry=json_object_array_get_idx(models,id))&&
                  json_object_object_get_ex(entry,"sequences",&sequences)&&
                  json_object_is_type(sequences,json_type_object);
    if(!ok){json_object_put(catalog);error="Invalid packed action model record";return false;}
    json_object* doc=json_object_from_file(authorFile.string().c_str());
    if(!doc){json_object_put(catalog);error="Unable to open seeded authoring model";return false;}
    json_object* skeleton=nullptr;
    if(!json_object_object_get_ex(doc,"skeleton",&skeleton)||
       !json_object_is_type(skeleton,json_type_object)){
        json_object_put(doc);json_object_put(catalog);error="Seeded model missing skeleton";return false;
    }
    json_object_object_add(skeleton,"sequences",json_object_get(sequences));
    if(json_object_object_get_ex(entry,"sequenceDepth",&depths)&&
       json_object_is_type(depths,json_type_object))
        json_object_object_add(skeleton,"sequenceDepth",json_object_get(depths));
    const int written=json_object_to_file_ext(authorFile.string().c_str(),doc,JSON_C_TO_STRING_PRETTY);
    json_object_put(doc);json_object_put(catalog);
    if(written<0){error="Unable to restore packed authoring action layers";return false;}
    return true;
}
bool seed(const fs::path& project,std::string& error) {
    for(int group=0;group<2;++group)for(int id=0;id<(group?cooperativeSectorCount:sectorCount);++id){
        const auto file=levelFile(project/"levels",id,group!=0);
        if(!fs::exists(file) && !writeLevel(file,group?cooperativeStages[id]:stages[id],id,group!=0,&error))return false;
    }
    for(int i=0;i<int(characters.size());++i){
        const auto file=characterModelFile(project/"models",i);
        if(!fs::exists(file)){
            if(!saveCharacterModelAt(file,i,&error)||!restoreActions(project,file,i,false,error))return false;
        }
    }
    for(int i=0;i<int(specs.size());++i){
        const auto file=enemyModelFile(project/"models",i);
        if(!fs::exists(file)){
            if(!saveEnemyModelAt(file,i,&error)||!restoreActions(project,file,i,true,error))return false;
        }
    }
    return true;
}
// Missing built-in levels are materialized once INSIDE the game source root.
// Existing level files are never regenerated, so external edits remain authoritative.
bool seedMissingSourceLevels(const fs::path& source,std::string& error){
    for(int group=0;group<2;++group)for(int id=0;id<(group?cooperativeSectorCount:sectorCount);++id){
        const auto file=levelFile(source/"levels",id,group!=0);
        if(!fs::exists(file) && !writeLevel(file,group?cooperativeStages[id]:stages[id],id,group!=0,&error))return false;
    }
    return true;
}
// Validate but NEVER create missing poses. The game's source JSON is the
// authority for which action states an individual character/enemy exposes.
bool completeInputMovementLayers(const fs::path& project,std::string& error){
    for(int group=0;group<2;++group)
      for(int id=0;id<(group?int(specs.size()):int(characters.size()));++id){
        const auto file=group?enemyModelFile(project/"models",id):characterModelFile(project/"models",id);
        json_object* doc=json_object_from_file(file.string().c_str());
        json_object *sk=nullptr,*seq=nullptr,*depth=nullptr;
        const bool valid=doc && json_object_object_get_ex(doc,"skeleton",&sk) &&
            json_object_is_type(sk,json_type_object) &&
            json_object_object_get_ex(sk,"sequences",&seq) && json_object_is_type(seq,json_type_object) &&
            json_object_object_get_ex(sk,"sequenceDepth",&depth) && json_object_is_type(depth,json_type_object);
        if(!valid){if(doc)json_object_put(doc);error="Invalid action source: "+file.string();return false;}
        json_object_object_foreach(seq, key, value){
            json_object* matching=nullptr;
            if(!json_object_object_get_ex(depth,key,&matching) ||
                !json_object_is_type(value,json_type_array) ||
                !json_object_is_type(matching,json_type_array) ||
                json_object_array_length(value)!=14 || json_object_array_length(matching)!=14){
                json_object_put(doc);error="Action sequence/depth mismatch: "+file.string()+" ("+key+")";return false;
            }
        }
        if(json_object_object_length(seq)!=json_object_object_length(depth)){
            json_object_put(doc);error="Orphaned action depth data: "+file.string();return false;
        }
        json_object_put(doc);
      }
    return true;
}
bool validate(const fs::path& project,std::string& error) {
    for(int group=0;group<2;++group)for(int id=0;id<(group?cooperativeSectorCount:sectorCount);++id){
        Stage stage{};
        if(!readLevel(levelFile(project/"levels",id,group!=0),group?cooperativeStages[id]:stages[id],
                      stage,id,group!=0,&error))return false;
    }
    for(int i=0;i<int(characters.size());++i)if(!loadCharacterModel(project/"models",i,&error))return false;
    for(int i=0;i<int(specs.size());++i)if(!loadEnemyModel(project/"models",i,&error))return false;
    return true;
}
bool recoverPacked(const fs::path& project,std::string& error){
    const auto root=project/"assets"/"models";
    if(fs::is_regular_file(root/"characters.vlrm")){
        std::ifstream stream(root/"characters.vlrm",std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)),{});
        std::array<SkeletonProfile,6> result{};
        if(!runtime_model::decode(bytes,result,&error))return false;
        characterSkeletons=result;
    }
    if(fs::is_regular_file(root/"enemies.vlem")){
        std::ifstream stream(root/"enemies.vlem",std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)),{});
        std::array<SkeletonProfile,10> result{};
        if(!enemy_runtime_model::decode(bytes,result,&error))return false;
        enemySkeletons=result;
    }
    return true;
}
bool generate(const fs::path& project,std::string& error){
    return source_export::generate(project,project/"export",error);
}
int fail(const std::string& error){std::cerr<<error<<'\n';return 1;}
void help(){
    std::cout<<"VOIDLINE Model Studio 0.0.17 · Linux C++ authoring backend\n"
             <<"  --prepare-workspace PATH         open LIVE game source (no local cache)\n"
             <<"  --import-game PATH PROJECT      legacy explicit copy, not used for workspace opening\n"
             <<"  --validate PROJECT             validate 26 levels and 16 models\n"
             <<"  --export-all PROJECT           seed missing built-in models\n"
             <<"  --export-project PROJECT       generate sources and VLRM/VLEM models\n"
             <<"  --publish-sources PATH [SOURCE]  update editable model inputs, baked source assets, levels (no PAK)\n"
             <<"  --update-workspace PATH [SOURCE] publish inputs and also refresh runtime PAKs\n"
             <<"  --detect-source PATH          find CMake source used by rebuild\n"
             <<"  --resolve-workspace PATH       show detected workspace structure\n"
             <<"  --export-single PROJECT character|enemy|level ID OUTPUT\n"
             <<"  --replace-asset PROJECT RELATIVE_NAME IMAGE_FILE\n";
}
}
int main(int argc,char** argv){
    if(argc<2 || std::string(argv[1])=="--help") {help();return 0;}
    const std::string action=argv[1];
    if((action=="--detect-source"||action=="--resolve-workspace")&&argc==3){
        const auto found=workspace::resolve(fs::absolute(argv[2]));
        if(action=="--resolve-workspace"){
            if(!found.valid)return fail(found.error);
            std::cout<<"GAME\t"<<found.root.string()<<'\n'
                     <<"MODE\t"<<(found.fromSource?"source":"runtime")<<'\n'
                     <<"ASSETS\t"<<(found.fromSource?found.source/"assets":found.pack)<<'\n';
        }
        if(!found.source.empty())std::cout<<"SOURCE\t"<<found.source.string()<<'\n';
        else std::cout<<"NO_SOURCE\tGame source checkout not found; archive is runtime-only\n";
        return 0;
    }
    if((action=="--prepare-workspace"&&argc==3)||
       ((action=="--update-workspace"||action=="--publish-sources")&&(argc==3||argc==4))){
        const fs::path game=fs::absolute(argv[2]);
        const auto project=workspace::prepare(game);
        if(!project.ok)return fail(project.error);
        std::string error;
        if(!seedMissingSourceLevels(project.project,error))return fail(error);
        // --prepare-workspace is read-only for model JSON. The only permitted
        // first-open writes are missing default levels in source/levels/.
        if(action!="--prepare-workspace" && !completeInputMovementLayers(project.project,error))return fail(error);
        if(action=="--prepare-workspace"){
            std::cout<<"GAME\t"<<project.game.string()<<'\n'
                     <<"MODE\t"<<(project.fromSource?"source":"runtime")<<'\n'
                     <<"WORKSPACE\t"<<project.project.string()<<'\n';
            if(!project.source.empty())std::cout<<"SOURCE\t"<<project.source.string()<<'\n';
            return 0;
        }
        if(!generate(project.project,error))return fail(error);
        std::error_code ec;
        int generatedAssets=0;
        const auto backup=project.project/"update-backups"/
                          ("model-studio-derived-"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        for(const char* file:{"characters.vlrm","enemies.vlem","characters.info.json","enemies.info.json","characters.actions.json","enemies.actions.json"}){
            const auto src=project.project/"export"/"assets"/"models"/file;
            const auto dest=project.project/"assets"/"models"/file;
            // Do not overwrite/re-timestamp unchanged source assets on publish.
            bool equal=false;
            if(fs::is_regular_file(dest,ec) && fs::file_size(src,ec)==fs::file_size(dest,ec) && !ec){
                std::ifstream a(src,std::ios::binary),b(dest,std::ios::binary);
                equal=a&&b&&std::equal(std::istreambuf_iterator<char>(a),{},std::istreambuf_iterator<char>(b),{});
            }
            if(equal)continue;
            if(fs::is_symlink(dest,ec))return fail("Refusing to replace linked asset "+dest.string());
            if(fs::is_regular_file(dest,ec)){
                fs::create_directories(backup,ec);
                if(ec)return fail("Cannot create generated asset backup directory");
                fs::copy_file(dest,backup/file,fs::copy_options::overwrite_existing,ec);
                if(ec)return fail(std::string("Cannot back up generated asset ")+file);
            }
            fs::create_directories(dest.parent_path(),ec);
            if(ec)return fail("Cannot create baked model folder");
            const auto temp=fs::path(dest.string()+".vlstudio-writing");
            fs::copy_file(src,temp,fs::copy_options::overwrite_existing,ec);
            if(ec)return fail(std::string("Cannot stage ")+file);
            fs::rename(temp,dest,ec);
            if(ec){std::error_code ignored;fs::remove(dest,ignored);ec.clear();fs::rename(temp,dest,ec);}
            if(ec)return fail(std::string("Cannot install ")+file);
            ++generatedAssets;
        }
        std::cout<<"GENERATED_ASSETS_UPDATED\t"<<generatedAssets<<'\n';
        // The source workspace is the only authoring workspace; all source
        // files are already saved in-place before publishing baked game assets.
        const fs::path source=argc==4?fs::absolute(argv[3]):project.source;
        if(!source.empty() && workspace::sourceFor(source)!=fs::weakly_canonical(source))
            return fail("SOURCE must be the VOIDLINE game source root (CMakeLists.txt and scripts/pack-assets.py)");
        if(source.empty() || fs::weakly_canonical(source)!=project.project)
            return fail("Publishing is limited to the open game source workspace.");
        int count=0,sourceFiles=0,inputFiles=0;
        if(!source.empty()){
            // Authoritative inputs are committed BEFORE any optional game.vpak.
            if(!workspace::publishEditableModels(project.project,source,inputFiles,error))return fail(error);
            std::cout<<"INPUT_MODELS_UPDATED\t"<<source.string()<<"/models\t"<<inputFiles<<'\n';
            if(!workspace::publishSourceAssets(project.project,source,sourceFiles,error))return fail(error);
            std::cout<<"SOURCE_UPDATED\t"<<source.string()<<"\t"<<(sourceFiles+generatedAssets)<<'\n';
        }else{
            std::cout<<"SOURCE_MISSING\tNo source checkout was found. Installed pack changes "
                        "will not survive a rebuild from unrelated source assets.\n";
        }
        int changedLevels=0;
        if(!workspace::publishLevels(project.project,project.game,source,changedLevels,error))return fail(error);
        std::cout<<"LEVELS_IN_SOURCE\t"<<(source.empty()?project.game:source).string()+"/levels"<<'\n';
        // Source-only mode publishes directly from its raw assets; a PAK is
        // generated into the editor export directory and optionally distributed
        // to already-built runtimes. No source-root game.vpak is required.
        fs::path newArchive;
        if(action=="--publish-sources"){
            std::cout<<"PACK_SKIPPED\tEditable model inputs and rebuild assets saved; runtime PAK unchanged.\n";
        }else if(project.fromSource){
            newArchive=project.project/"export"/"game.vpak";
            if(!art::repack(project.project/"assets",newArchive,count,error))return fail(error);
            std::cout<<"PACK_STAGED\t"<<newArchive.string()<<'\n';
        }else{
            if(!workspace::updatePak(project.game,project.project,count,error))return fail(error);
            newArchive=project.game/"assets"/"game.vpak";
            std::cout<<"RUNTIME_PAK\t"<<newArchive.string()<<'\n';
        }
        std::vector<fs::path> refreshed;
        if(!source.empty() && !newArchive.empty() && !workspace::syncRuntimePaks(newArchive,source,refreshed,error))return fail(error);
        for(const auto& path:refreshed)std::cout<<"RUNTIME_PAK\t"<<path.string()<<'\n';
        // Levels are saved in source/levels/ immediately, so source-vs-source
        // comparison is intentionally always unchanged. Still synchronize the
        // live workspace to existing built runtimes on every Publish.
        if(!source.empty()){
            std::vector<fs::path> levelRuntimes;
            if(!workspace::syncRuntimeLevels(source,levelRuntimes,error,project.game))return fail(error);
            for(const auto& path:levelRuntimes)std::cout<<"RUNTIME_LEVEL\t"<<path.string()<<'\n';
        }
        if(action=="--update-workspace" && project.fromSource && refreshed.empty())
            std::cout<<"RUNTIME_PENDING\tNo built game found. Build VOIDLINE to create the updated runtime PAK.\n";
        // Verify the generated binary from the live source tree.
        std::array<SkeletonProfile,6> verified{};
        const auto baked=project.project/"assets"/"models"/"characters.vlrm";
        std::ifstream in(baked,std::ios::binary);
        const std::vector<uint8_t> raw((std::istreambuf_iterator<char>(in)),{});
        if(!runtime_model::decode(raw,verified,&error))return fail("Exported character model failed verification: "+error);
        const auto& kade=verified[2];
        std::cout<<"MODEL_VERIFIED\tKade Vale (ID 2)\tbase.head="
                 <<kade.offsets[0][3].x<<","<<kade.offsets[0][3].y
                 <<"\trun.hand="<<kade.offsets[2][8].x<<","<<kade.offsets[2][8].y
                 <<"\tbodyHeight="<<kade.bodyHeight<<'\n';
        std::cout<<"Published "<<inputFiles<<" editable model input(s), "<<count<<" packed assets; "
                 <<(source.empty()?"installed archive only":"game source assets synchronized")
                 <<"; game source levels are authoritative. Rebuild or restart the game to load changes.\n";return 0;
    }
    if(action=="--import-game"&&argc==4){
        const fs::path game=fs::absolute(argv[2]),project=fs::absolute(argv[3]);
        if(fs::exists(project/"assets")||fs::exists(project/"levels")||fs::exists(project/"models"))
            return fail("Destination already contains project data; use --prepare-workspace instead.");
        int count=0;std::string error;
        const auto found=workspace::resolve(game);
        if(!found.valid)return fail(found.error);
        if(!workspace::importAssets(found,project/"assets",count,error) ||
           !recoverPacked(project,error))return fail(error);
        // Preserve existing editable files without overwriting any edits.
        for(const auto& folder:{"levels","models"}){
            if(!fs::is_directory(found.root/folder))continue;
            std::error_code ec;
            fs::copy(found.root/folder,project/folder,fs::copy_options::recursive|fs::copy_options::skip_existing,ec);
            if(ec)return fail("Could not import custom "+std::string(folder));
        }
        if(!seed(project,error))return fail(error);
        std::cout<<"Imported "<<count<<" resources and seeded 42 definitions into "<<project<<'\n';return 0;
    }
    if((action=="--export-all"||action=="--validate"||action=="--export-project")&&argc==3){
        const fs::path project=fs::absolute(argv[2]);std::string error;
        if(action=="--export-all"){
            if(!seed(project,error)||!completeInputMovementLayers(project,error))return fail(error);
            std::cout<<"EXPORTED 26 LEVELS + 16 MODELS\n";return 0;
        }
        if(action=="--validate"){
            if(!validate(project,error))return fail(error);
            std::cout<<"Validated 26 levels and 16 models\n";return 0;
        }
        if(!completeInputMovementLayers(project,error)||!generate(project,error))return fail(error);
        std::cout<<"OFFLINE EXPORT: C++ source, images, characters.vlrm, enemies.vlem\n";return 0;
    }
    if(action=="--export-single"&&argc==6){
        const fs::path project=fs::absolute(argv[2]);std::string error;
        int id=-1;
        try{id=std::stoi(argv[4]);}catch(...){return fail("Invalid model ID");}
        const auto type=std::string(argv[3]);
        if(type=="character"&&id>=0&&id<int(characters.size())){
            if(!loadCharacterModel(project/"models",id,&error)||!saveCharacterModelAt(argv[5],id,&error))return fail(error);
        }else if(type=="enemy"&&id>=0&&id<int(specs.size())){
            if(!loadEnemyModel(project/"models",id,&error)||!saveEnemyModelAt(argv[5],id,&error))return fail(error);
        }else if(type=="level"&&id>=0&&id<sectorCount+cooperativeSectorCount){
            const bool coop=id>=sectorCount;
            const int index=coop?id-sectorCount:id;
            Stage loaded{};
            if(!readLevel(levelFile(project/"levels",index,coop),
                          coop?cooperativeStages[index]:stages[index],loaded,index,coop,&error)||
               !writeLevel(argv[5],loaded,index,coop,&error))return fail(error);
        }else return fail("Unknown model type or ID");
        std::cout<<"Exported single model to "<<argv[5]<<'\n';return 0;
    }
    if(action=="--replace-asset"&&argc==5){
        const fs::path relative=argv[3];
        if(!art::safePath(relative)||!art::isImage(relative.string()))return fail("Invalid relative image path");
        art::Raster raster{};std::string error;
        if(!art::decode(argv[4],raster,error)||!art::encode(fs::path(argv[2])/"assets"/relative,raster,error))return fail(error);
        std::cout<<"Replaced authoring image "<<relative<<'\n';return 0;
    }
    help();return 2;
}
