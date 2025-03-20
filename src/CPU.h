#pragma once
#include <cstdint>
#include <vector>

#define HL(H, L) (((uint32_t)(H) << 8) | ((uint32_t)(L)))
#define U16(MSB, LSB) (((uint32_t)(MSB) << 8) | ((uint32_t)(LSB)))

class CPU { // Maybe more apt to call this the Z80 or SM83
public:
 //   CPU() {}
    CPU(std::vector<uint8_t>& romData);

    // CPU functions
    void step();
    uint8_t readByte();
	uint8_t readAddr(uint16_t addr);
	void writeByte(uint8_t val, uint16_t addr);
    void debugPrint(uint8_t opcode);

    // Helpers
    //void setFlags(uint8_t flags) { F = flags; }
    void setFlags(bool z, bool n, bool h, bool c) { F = (z << 7) | (n << 6) | (h << 5) | (c << 4); }
	void setZ(bool z) { F = (F & 0x7F) | (z << 7); }
	void setN(bool n) { F = (F & 0xBF) | (n << 6); }
	void setH(bool h) { F = (F & 0xDF) | (h << 5); }
	void setC(bool c) { F = (F & 0xEF) | (c << 4); }
    void setA(uint8_t a) { A = a; }
	void setPC(uint16_t pc) { PC = pc; }
	void setSP(uint16_t sp) { SP = sp; }

    bool getZ() { return (F >> 7) & 0x01; }
    bool getN() { return (F >> 6) & 0x01; }
    bool getH() { return (F >> 5) & 0x01; }
    bool getC() { return (F >> 4) & 0x01; }
    bool getCond(uint8_t cc) {
        static const bool conds[] = { // magic
            !getZ(), getZ(), !getC(), getC()
        };

        return (cc < 4) ? conds[cc] : false;
    }

    void setR8(uint8_t reg, uint8_t val) {
        if (reg == 6) { // [HL] memory case
            ram[HL(H, L)] = val;
        }
       else *r8[reg] = val; 
    }

	uint16_t getPC() { return PC; }

    uint8_t getR8(uint8_t reg) {
        if (reg == 6) {
            return ram[HL(H, L)]; // Alternatively 
        }
        else return *r8[reg];
    }

	uint16_t getR16(uint8_t reg) {
		if (reg == 3) return SP;
		return U16(*r16[reg], *r16[reg + 3]);
	}

    void setR16(uint8_t reg, uint16_t val) {
		// Reg Map; 0: BC, 1: DE, 2: HL, 3: SP, 4: AF, 5/6: HL+/-1
        if (reg == 3) SP = val; // Adjust 0x08 case
		else if (reg == 4) { // Maybe incorporate this into r16 array 
			A = (val & 0xFF00) >> 8;
			F = val & 0x00FF;
		}
		else {
			*r16[reg] = (val & 0xFF00) >> 8;
            *r16[reg + 3] = val & 0x00FF;
		}
    }

	uint16_t getImm16() {
		uint8_t LSB = readByte();
		uint8_t MSB = readByte();
		return U16(MSB, LSB);
	}

	uint8_t getImm8() {
		return readByte();
	}

    uint8_t getFlags() { return F; }
    uint8_t getA() { return A; }

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
	uint8_t* r16[6] = { &B, &D, &H, &C, &E, &L}; // 16-bit loads
    /*).

    0x0000�0x7FFF: Game ROM.*

    0x8000�0x9FFF: VRAM (Video RAM).

    0xA000�0xBFFF: External RAM (if present in the cartridge).

    0xC000�0xDFFF: Work RAM (WRAM).

    0xE000�0xFDFF: Echo RAM (mirror of WRAM).

    0xFE00�0xFE9F: OAM (Sprite Attribute Table).

    0xFF00�0xFF7F: I/O Registers.

    0xFF80�0xFFFE: High RAM (HRAM).

    0xFFFF: Interrupt Enable Register (IE).
    */
    uint8_t ram[0x10000] = {0}; // 32kb rom, 8kb vram, external ram, internal ram, pain, i/o

};