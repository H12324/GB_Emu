#include <iostream>

#include "GB.h"

int main() {
    std::cout << "Starting GB Emulator" << std::endl;
    GB gameboy;
    while (true) { // Change to a running flag
        gameboy.run();
        break;
    }
    return 0;
}