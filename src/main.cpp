#include "engine/engine.hpp"
#include "content/runtime_model_catalog.hpp"
#include "content/skeleton.hpp"
#include "content/enemy_motion.hpp"
#include "content/characters.hpp"
#include "content/model_io.hpp"
#include "platform/paths.hpp"
#include <iomanip>
#include <iostream>
#ifdef _WIN32
#include "platform/paths.hpp"
#endif

namespace {
int run(int argc,char** argv) try {
    // Verify the exact models the executable will use, without initializing graphics.
    if(argc==2 && std::string_view(argv[1])=="--model-report") {
        const auto root=vl::executableDirectory();
        const auto assets=root/"assets";
        std::cout<<"Executable directory: "<<root.string()<<'\n';
        std::cout<<"Runtime archive: "<<(assets/"game.vpak").string()<<'\n';
        vl::loadBakedCharacterModels(assets);
        vl::loadBakedEnemyModels(assets);
        for(size_t id=0;id<vl::characters.size();++id) {
            const auto& model=vl::characterSkeletons[id];
            std::cout<<"Character "<<id<<" "<<vl::characters[id].name
                     <<": bodyHeight="<<model.bodyHeight
                     <<", bodyWidth="<<model.bodyWidth
                     <<", base.head=("<<model.offsets[0][3].x<<','<<model.offsets[0][3].y<<')'
                     <<", run.hand=("<<model.offsets[2][8].x<<','<<model.offsets[2][8].y<<')'
                     <<", walk.head=("<<model.actionOffsets[1][3].x<<','<<model.actionOffsets[1][3].y<<')'<<'\n';
        }
        for(size_t id=0;id<vl::specs.size();++id){
            const auto& spec=vl::specs[id];
            std::cout<<"Enemy "<<id<<" "<<vl::enemyNames[id]
                     <<": speed="<<spec.speed<<", health="<<spec.hp
                     <<", dimensions=("<<spec.w<<','<<spec.h<<")"
                     <<", fire.hand=("<<vl::enemySkeletons[id].actionOffsets[7][8].x<<','
                     <<vl::enemySkeletons[id].actionOffsets[7][8].y<<")"<<'\n';
        }
        return 0;
    }
    const auto options=vl::parseOptions(argc,argv);
#ifdef _WIN32
    if (options.report) {
        vl::printDirectXReport();
        return 0;
    }
#endif
    vl::Engine engine(options);
    return engine.run();
} catch (const std::exception& error) {
    std::cerr<<"VOIDLINE: "<<error.what()<<'\n';
    return 1;
}
}

#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
    std::vector<std::string> utf8;
    utf8.reserve(argc);
    for (int i=0;i<argc;++i) utf8.push_back(vl::utf8Path(argv[i]));
    std::vector<char*> arguments;
    arguments.reserve(utf8.size()+1);
    for (auto& arg:utf8) arguments.push_back(arg.data());
    arguments.push_back(nullptr);
    return run(argc,arguments.data());
}
#else
int main(int argc,char** argv) { return run(argc,argv); }
#endif
