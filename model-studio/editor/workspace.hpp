#pragma once
#include "art.hpp"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace workspace {
namespace fs=std::filesystem;
struct Progress {
    std::atomic<int> current{0},total{1};
    mutable std::mutex lock;
    std::string label="Preparing workspace";
    void report(int at,int of,const std::string& detail){
        current.store(at);total.store(std::max(1,of));
        std::lock_guard guard(lock);label=detail;
    }
    std::string caption() const {std::lock_guard guard(lock);return label;}
};
struct Result {bool ok=false;bool existing=false;int count=0;fs::path project;fs::path game;fs::path source;bool fromSource=false;std::string error;};
// A workspace can be a source checkout (assets/ files, no PAK required), an
// installed/runtime game (assets/game.vpak), or a folder inside either one.
struct Resolved {
    fs::path root,source,pack;
    bool fromSource=false;
    bool valid=false;
    std::string error;
};
Resolved resolve(const fs::path& selected);
bool importAssets(const Resolved& game,const fs::path& destination,
                  int& count,std::string& error,Progress* progress=nullptr);
fs::path projectFor(const fs::path& game);
// Locate the source tree CMake uses to regenerate game.vpak during a rebuild.
fs::path sourceFor(const fs::path& game);
// Copies validated edited media into source/assets with backups outside the asset tree.
bool publishSourceAssets(const fs::path& authoring, const fs::path& source, int& changed, std::string& error);
// Synchronize already-built executable-adjacent archives after a successful
// Update PAK. The function does not touch the current live game executable.
bool syncRuntimePaks(const fs::path& archive,const fs::path& source,
                     std::vector<fs::path>& published,std::string& error);
bool validGame(const fs::path& game);
Result prepare(const fs::path& game,Progress* progress=nullptr);
// For installed game mode only: modify game/assets/game.vpak with a backup.
// All calls must be made with a complete editor authoring asset tree.
bool updatePak(const fs::path& game,const fs::path& authoring,int& assets,std::string& error,Progress* progress=nullptr);
}
