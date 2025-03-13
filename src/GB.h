#pragma once

#include "CPU.h"
#include "PPU.h"

class GB {
    public:
    GB(std::vector<uint8_t> &romData);
    ~GB();
    void run();
    bool loadROM() {return false;}

    private:
    CPU *cpu;
//  PPU ppu;

};