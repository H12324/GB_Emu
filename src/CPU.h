#pragma once
#include <cstdint>
#include <vector>

class CPU { // Maybe more apt to call this the Z80 or SM83
public:
 //   CPU() {}
    CPU(std::vector<uint8_t>& romData);

    // CPU functions
    void step();
    uint8_t readByte();
    void debugPrint(uint8_t opcode);

private:
    //void LD_r8_r8(uint8_t* src, uint8_t* dst);

	// Registers and related
    uint8_t A; // Accumulator
	uint8_t F; // Flags z: zero n: subtract h: half carry c: carry
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;

	/* // Thinking these may be unneccesary
    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    */
    //uint16_t HL;

    uint16_t PC;
    uint16_t SP;

    uint8_t* r8[8] = { &B, &C, &D, &E, &H, &L, nullptr, &A };// 8-bit loads

    /*).

    0x0000–0x7FFF: Game ROM.*

    0x8000–0x9FFF: VRAM (Video RAM).

    0xA000–0xBFFF: External RAM (if present in the cartridge).

    0xC000–0xDFFF: Work RAM (WRAM).

    0xE000–0xFDFF: Echo RAM (mirror of WRAM).

    0xFE00–0xFE9F: OAM (Sprite Attribute Table).

    0xFF00–0xFF7F: I/O Registers.

    0xFF80–0xFFFE: High RAM (HRAM).

    0xFFFF: Interrupt Enable Register (IE).
    */
    uint8_t ram[0x10000] = {0}; // 32kb rom, 8kb vram, external ram, internal ram, pain, i/o

};