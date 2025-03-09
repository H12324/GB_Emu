#pragma once

#include "CPU.h"
#include "PPU.h"

class GB {
    public:
    GB();
    ~GB() {}
    void run();

    private:
    CPU cpu;
//  PPU ppu;

};