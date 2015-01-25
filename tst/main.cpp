#include <memory>
#include <iostream>
#include "engine.hpp"
#include "timeout.hpp"

using namespace Sb;
namespace Sb {
    extern size_t test_ocb();
}

int main(const int argc, const char* const argv[])
{
    int numWorkers = 1;
    if(argc == 2) {
        numWorkers = std::atoi(argv[1]);
    }
    if(argc > 2 || numWorkers < 1) {
        std::cerr << "Usage: " << argv[0] << " [numworkerthreads >= 1]\n";
        return 1;
    }
    try {
    test_ocb();
        Engine::AddRoot(new ExitTimer(110000000000));
        Engine::Go();
    } catch(std::exception& e) {
        std::cerr << e.what() << "\n";
    } catch(...) {
        std::cerr << argv[0] <<  " exception.\n";
    }
    std::cerr << argv[0] << " exited.\n";
    return 0;
}
