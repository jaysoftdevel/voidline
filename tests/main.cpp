#include "support.hpp"
#include <cstdlib>

int main() {
    runInputTests();
    runMovementTests();
    runCombatTests();
    runHitTests();
    runProgressionTests();
    runWorldTests();
    runCoopTests();
    if (test::failures) { std::cerr << test::failures << " check(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "All checks passed\n";
    return EXIT_SUCCESS;
}
