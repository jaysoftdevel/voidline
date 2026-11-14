#include "engine/engine.hpp"
#include <iostream>

int main(int argc,char** argv) try {
    vl::Engine engine(vl::parseOptions(argc,argv));
    return engine.run();
} catch (const std::exception& error) {
    std::cerr<<"VOIDLINE: "<<error.what()<<'\n';
    return 1;
}
