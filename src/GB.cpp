#include "GB.h"

GB::GB() {
    // Add a load ROM function
}

void GB::run() {
    cpu.step();
}