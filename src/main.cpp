#include "engine/engine.hpp"
#include <iostream>
#ifdef _WIN32
#include "platform/paths.hpp"
#endif

namespace {
int run(int argc,char** argv) try {
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
