#pragma once
#include <cstdint>

class CPU { // Maybe more apt to call this the Z80 or SM83
public:
    CPU();
    void step();
private:
    uint8_t A; // Accumulator
    uint8_t F; // Flags
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;

    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;

    uint16_t PC;
    uint16_t SP;

    uint8_t ram[0xFFFF]; // 32kb rom, 8kb vram, external ram, internal ram, pain, i/o

};