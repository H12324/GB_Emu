#include "CPU.h"

CPU::CPU() {
    // Load ROM into memory
}

void CPU::step() {
    uint16_t opcode = ram[PC];

    switch (opcode & 0x1)
    {
    case 0x0:
        /* code */
        break;
    
    default:
        break;
    }
}