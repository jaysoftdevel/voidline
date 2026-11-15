#include "workspace.hpp"
#include "content/model_io.hpp"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace workspace {
namespace {
fs::path normalized(const fs::path& p) {
    std::error_code ec;
    auto result=fs::weakly_canonical(p,ec);
    return ec?fs::absolute(p):result;
}
std::string key(const fs::path& path) {
    uint64_t h=14695981039346656037ull;
    for(const unsigned char c:normalized(path).generic_string()){h^=c;h*=1099511628211ull;}
    std::ostringstream s;s<<std::hex<<std::setw(16)<<std::setfill('0')<<h;
    return s.str();
}
fs::path dataRoot() {
    if(const char* custom=std::getenv("VOIDLINE_STUDIO_WORKSPACES"))
        if(*custom)return custom;
    if(const char* data=std::getenv("XDG_DATA_HOME"))
        if(*data)return fs::path(data)/"voidline-model-studio"/"workspaces";
    if(const char* home=std::getenv("HOME"))
        if(*home)return fs::path(home)/".local"/"share"/"voidline-model-studio"/"workspaces";
    return fs::temp_directory_path()/"voidline-model-studio-workspaces";
}
bool copyCustom(const fs::path& source,const fs::path& destination,Progress* progress,int& count,std::string& error) {
    if(!fs::is_directory(source))return true;
    std::error_code ec;
    for(const auto& item:fs::recursive_directory_iterator(source,fs::directory_options::skip_permission_denied,ec)){
        if(ec){error="Cannot read custom model directory";return false;}
        if(item.is_symlink(ec)){error="Symlinks not permitted in workspace models";return false;}
        if(!item.is_regular_file(ec))continue;
        const auto name=item.path().lexically_relative(source);
        if(!art::safePath(name))continue;
        const auto target=destination/name;
        fs::create_directories(target.parent_path(),ec);
        if(ec){error="Cannot create model directory";return false;}
        if(!fs::exists(target)){
            fs::copy_file(item.path(),target,fs::copy_options::none,ec);
            if(ec){error="Cannot import existing model file";return false;}
            ++count;
            if(progress)progress->report(count,count+1,"Importing "+name.filename().string());
        }
    }
    return true;
}
}

fs::path sourceFor(const fs::path& game) {
    auto current=normalized(game);
    std::error_code ec;
    for (int n=0;n<12;++n) {
        const auto cmake=current/"CMakeLists.txt";
        if (fs::is_regular_file(cmake,ec) && fs::is_regular_file(current/"scripts"/"pack-assets.py",ec)
            && fs::is_directory(current/"assets",ec)) {
            std::ifstream in(cmake,std::ios::binary);
            std::string beginning(512,'\0');in.read(beginning.data(),static_cast<std::streamsize>(beginning.size()));
            beginning.resize(static_cast<size_t>(in.gcount()));
            if (beginning.find("project(voidline VERSION ")!=std::string::npos)return current;
        }
        const auto parent=current.parent_path();
        if (parent.empty()||parent==current)break;
        current=parent;
    }
    return {};
}
Resolved resolve(const fs::path& selected) {
    Resolved result;
    std::error_code ec;
    const auto candidate=normalized(selected);
    auto folder=candidate;
    if(fs::is_regular_file(folder,ec)) {
        if(folder.filename()=="game.vpak" && folder.parent_path().filename()=="assets")
            folder=folder.parent_path().parent_path();
        else folder=folder.parent_path();
    }
    // The file picker can select assets/ or a build subdirectory. Prefer a
    // complete installed runtime at the selected location over its ancestor
    // source checkout, so the old installed-game workflow remains supported.
    if(folder.filename()=="assets" && fs::is_regular_file(folder/"game.vpak",ec))
        folder=folder.parent_path();
    if(fs::is_regular_file(folder/"assets"/"game.vpak",ec)) {
        result.root=folder;
        result.pack=folder/"assets"/"game.vpak";
        result.source=sourceFor(folder);
        result.valid=fs::file_size(result.pack,ec)>=12 && !ec;
        if(!result.valid)result.error="Invalid assets/game.vpak in "+folder.string();
        return result;
    }
    // A source checkout has no PAK before CMake has built the runtime. Locate
    // the authoritative source/assets directory via its game CMake structure.
    const auto source=sourceFor(folder);
    if(!source.empty()) {
        result.root=source;
        result.source=source;
        result.fromSource=true;
        result.valid=true;
        return result;
    }
    result.error="No VOIDLINE workspace found at "+folder.string()+
                 ". Choose the source root (CMakeLists.txt and assets/), "
                 "or a built game folder (assets/game.vpak).";
    return result;
}

bool importAssets(const Resolved& game,const fs::path& destination,
                  int& count,std::string& error,Progress* progress) {
    if(!game.valid){error=game.error;return false;}
    if(!game.fromSource) {
        return art::unpack(game.pack,destination,count,error,
           [progress](int done,int total,const std::string& name){
               if(progress)progress->report(done,total,"Extracting "+name);
           });
    }
    count=0;
    const auto assets=game.source/"assets";
    if(!fs::is_directory(assets)) {error="Source assets folder missing";return false;}
    std::error_code ec;
    for(fs::recursive_directory_iterator it(assets,fs::directory_options::none,ec),end;
        it!=end && !ec;it.increment(ec)) {
        if(it->is_symlink(ec)) {error="Source assets cannot contain symlinks";return false;}
        if(!it->is_regular_file(ec))continue;
        const auto relative=it->path().lexically_relative(assets);
        if(!art::safePath(relative) || relative.extension()==".vpak" ||
           relative.begin()->string()=="licenses")continue;
        const auto target=destination/relative;
        fs::create_directories(target.parent_path(),ec);
        if(ec){error="Cannot create authoring asset directory";return false;}
        // Initial import never overwrites a half-created or recovered project.
        if(!fs::exists(target,ec)) {
            fs::copy_file(it->path(),target,fs::copy_options::none,ec);
            if(ec){error="Cannot copy source asset: "+relative.string();return false;}
            ++count;
            if(progress)progress->report(count,count+1,"Reading "+relative.generic_string());
        }
    }
    if(ec){error="Cannot scan source assets";return false;}
    return true;
}

bool publishSourceAssets(const fs::path& authoring,const fs::path& source,int& changed,std::string& error) {
    changed=0;
    if (source.empty()||!fs::is_regular_file(source/"scripts"/"pack-assets.py")||
        !fs::is_directory(source/"assets")) {error="Game SOURCE folder must contain scripts/pack-assets.py and assets/";return false;}
    if (!fs::is_directory(authoring/"assets")){error="Authoring assets not found";return false;}
    std::error_code ec;
    const auto root=authoring/"assets";
    // All copies are reversible, and no backup is placed under assets/ where CMake would package it.
    const auto stamp=std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    const auto backup=source/"update-backups"/("model-studio-"+stamp);
    for (fs::recursive_directory_iterator it(root,fs::directory_options::none,ec),end;it!=end&&!ec;it.increment(ec)){
        if(it->is_symlink(ec)){error="Symlink found in authoring assets";return false;}
        if(!it->is_regular_file(ec))continue;
        const auto relative=it->path().lexically_relative(root);
        if (!art::safePath(relative)||relative.extension()==".vpak"||relative.begin()->string()=="licenses")continue;
        const auto dest=source/"assets"/relative;
        if (fs::is_symlink(dest,ec)){error="Refusing to overwrite a linked source asset";return false;}
        bool identical=false;
        if (fs::is_regular_file(dest,ec)&&fs::file_size(dest,ec)==it->file_size(ec)){
            std::ifstream a(it->path(),std::ios::binary),b(dest,std::ios::binary);
            identical=std::equal(std::istreambuf_iterator<char>(a),std::istreambuf_iterator<char>(),
                                 std::istreambuf_iterator<char>(b),std::istreambuf_iterator<char>());
        }
        if(identical)continue;
        if(fs::exists(dest,ec)) {
            const auto saved=backup/relative;
            fs::create_directories(saved.parent_path(),ec);
            if(ec){error="Could not create source asset backup";return false;}
            fs::copy_file(dest,saved,fs::copy_options::overwrite_existing,ec);
            if(ec){error="Could not back up source asset: "+relative.string();return false;}
        }
        fs::create_directories(dest.parent_path(),ec);
        if(ec){error="Could not create destination asset folder";return false;}
        const auto temp=fs::path(dest.string()+".vlstudio-writing");
        fs::copy_file(it->path(),temp,fs::copy_options::overwrite_existing,ec);
        if(ec){error="Could not stage source asset: "+relative.string();return false;}
        fs::rename(temp,dest,ec);
        if(ec){ // e.g. platform file replace semantics
            std::error_code ignored;fs::remove(dest,ignored);ec.clear();fs::rename(temp,dest,ec);
        }
        if(ec){error="Could not replace source asset: "+relative.string();return false;}
        ++changed;
    }
    if(ec){error="Could not scan authoring asset folder";return false;}
    return true;
}

bool syncRuntimePaks(const fs::path& archive,const fs::path& source,
                     std::vector<fs::path>& published,std::string& error) {
    published.clear();
    std::error_code ec;
    const auto original=normalized(archive);
    if(!fs::is_regular_file(original,ec)){error="Exported game.vpak not found";return false;}
    const auto base=normalized(source);
    const auto stamp=std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    // Only build directories directly within this source checkout. Never
    // traverse external symlinked directories, caches or nested source trees.
    std::vector<fs::path> buildDirectories;
    for(fs::directory_iterator it(base,fs::directory_options::skip_permission_denied,ec),end;
        it!=end&&!ec;it.increment(ec)){
        if(!it->is_directory(ec)||it->is_symlink(ec))continue;
        const auto name=it->path().filename().string();
        if(name=="build"||name=="build-windows"||name=="build-release"||
           name=="build-debug"||name=="linux-runtime")
            buildDirectories.push_back(it->path());
    }
    if(ec){error="Cannot scan game source folders";return false;}
    for(const auto& build:buildDirectories){
        for(fs::recursive_directory_iterator it(build,fs::directory_options::skip_permission_denied,ec),end;
            it!=end && !ec;it.increment(ec)) {
            if(it.depth()>6){it.disable_recursion_pending();continue;}
            if(it->is_symlink(ec)){it.disable_recursion_pending();continue;}
            if(!it->is_regular_file(ec)||it->path().filename()!="game.vpak"||
               it->path().parent_path().filename()!="assets")continue;
            const auto target=normalized(it->path());
            if(target==original)continue;
            // Check an executable in the same runtime folder; ignore old
            // extracted archives elsewhere in build trees.
            const auto parent=target.parent_path().parent_path();
            bool executable=false;
            for(const auto& file:{"voidline","voidline.exe","VOIDLINE.exe"})
                if(fs::is_regular_file(parent/file,ec))executable=true;
            // Windows multi-config build: exe and archive in Release.
            if(!executable && parent.filename()=="assets")continue;
            if(!executable)continue;
            const auto backup=fs::path(target.string()+".backup-"+stamp);
            fs::copy_file(target,backup,fs::copy_options::none,ec);
            if(ec){error="Cannot back up runtime archive: "+target.string();return false;}
            const auto temp=fs::path(target.string()+".vlstudio-writing");
            fs::copy_file(original,temp,fs::copy_options::overwrite_existing,ec);
            if(ec){error="Cannot write runtime archive: "+target.string();return false;}
            fs::rename(temp,target,ec);
            if(ec){
                ec.clear();fs::remove(target,ec);ec.clear();fs::rename(temp,target,ec);
            }
            if(ec){error="Cannot install runtime archive: "+target.string();return false;}
            published.push_back(target);
        }
        if(ec){error="Could not scan runtime archives in "+build.string();return false;}
    }
    return true;
}

fs::path projectFor(const fs::path& game) {
    const auto found=resolve(game);
    return dataRoot()/key(found.valid?found.root:game);
}
bool validGame(const fs::path& game) {return resolve(game).valid;}
Result prepare(const fs::path& selected,Progress* progress) {
    Result result;
    const auto found=resolve(selected);
    if(!found.valid){result.error=found.error;return result;}
    result.game=found.root;
    result.source=found.source;
    result.fromSource=found.fromSource;
    result.project=projectFor(found.root);
    std::error_code ec;
    fs::create_directories(result.project,ec);
    if(ec){result.error="Cannot create authoring workspace";return result;}
    const auto marker=result.project/".workspace-ready";
    // Keep all user edits when a saved authoring workspace is reopened.
    if(fs::is_regular_file(marker) && fs::is_directory(result.project/"assets") &&
       fs::is_directory(result.project/"models") && fs::is_directory(result.project/"levels")) {
        std::ifstream in(marker);std::string recorded;std::getline(in,recorded);
        if(recorded!=normalized(found.root).string()){
            result.error="Workspace marker does not match selected game";return result;
        }
        if(progress)progress->report(1,1,"Opening saved authoring workspace");
        result.existing=true;result.ok=true;return result;
    }
    if(progress)progress->report(0,1,found.fromSource?"Reading source assets":"Reading game.vpak");
    if(!importAssets(found,result.project/"assets",result.count,result.error,progress))return result;
    if(progress)progress->report(1,3,"Importing editable models and levels");
    int copied=0;
    if(!copyCustom(found.root/"levels",result.project/"levels",progress,copied,result.error)||
       !copyCustom(found.root/"models",result.project/"models",progress,copied,result.error))return result;
    fs::create_directories(result.project/"levels",ec);
    fs::create_directories(result.project/"models",ec);
    if(ec){result.error="Cannot create authoring model folders";return result;}
    std::ofstream out(marker,std::ios::binary|std::ios::trunc);
    out<<normalized(found.root).string()<<'\n';
    if(!out){result.error="Cannot save workspace marker";return result;}
    if(progress)progress->report(1,1,"Loaded "+std::to_string(result.count)+" game assets");
    result.ok=true;return result;
}
bool updatePak(const fs::path& selected,const fs::path& authoring,int& assets,std::string& error,Progress* progress) {
    const auto found=resolve(selected);
    if(!found.valid){error=found.error;return false;}
    if(found.fromSource){error="Source workspace has no installed game.vpak; publish source assets and generate an export pack instead";return false;}
    if(!fs::is_regular_file(authoring/".workspace-ready")){
        error="Workspace was not imported from a VOIDLINE game";return false;
    }
    std::ifstream marker(authoring/".workspace-ready");std::string recorded;
    std::getline(marker,recorded);
    if(recorded!=normalized(found.root).string()){
        error="Workspace/game path mismatch; refusing to modify another installation";return false;
    }
    return art::repack(authoring/"assets",found.pack,assets,error,
       [progress](int done,int total,const std::string& name){
           if(progress)progress->report(done,total,"Packing "+name);
       });
}
}
