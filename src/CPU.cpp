#include "CPU.h"
#include "opcode.h"
#include <iostream>
#include <iomanip>

#define HL(H, L) (((uint32_t)(H) << 8) | ((uint32_t)(L)))

CPU::CPU(std::vector<uint8_t>& romData) 
    : PC(0x100), SP(0xFFFE), H(0x01), L(0x4D), F(0xB0),
      A(0x01), B(0x00), C(0x13), D(0x00), E(0xD8)  {

	// Skip boot rom

	std::memcpy(ram, romData.data(), romData.size());
	// TODO: Initialize rest of the ram later once it is more relevant

}

// Steps to goal/Progress on opcodes
/*  Note: I used AI to generate these numbers so is probably incorrect
//  0. Decide Opcode schema and decoding procedure
//  1. 8-bit Loads (63/63)              STATUS: COMPLETE
    2. 16-bit loads (9)
    3. 8-bit Arithmetic (45)
    4. 16-bit Arithmetic (12)
    5. Rotates/Shifts (6 + 32 CB)
    6. Bit Operations (96 CB: pain)
    7. Jumps (14)
    8. Control (5)
    9. Misc (6)
    10.Make clock accurate timing system
*/

void CPU::step() {
    uint8_t opcode = readByte();
	//opcode = 0x41; // Testing purposes
    //opcode = 0x4E;
    //opcode = 0x76;
	opcode = 0x71;

	int src = 0;
	int dst = 0;
	int numCycles = 1;

	std::cout << "Before Operation: " << std::endl;
	debugPrint(opcode); // Note: PC is already incremented so it should really be one before account for that later

    switch (opcode & (uint8_t)0xC0)
    {
    case 0x00: // NOP: No OPeration
        /* do nothing, later make it skip 4 tick cycles: 1 clock*/
		std::cerr << "Unimplemented opcode: " << opcode<< std::endl;
        break;
	case 0x40: // Block 1: LD r8 r8 instructions
		/* 8-bit loads */ // 0x40-0x7F
		// All take 1 clock cycle except when [HL] is involved it's 2
        // loading into itself does nothing but I'll implement it anyway
		// Exception: LD [HL][HL] 0x76 is HALT, 

        src = (opcode & 0x07); // feels strange using regular ints now
		dst = (opcode & 0x38) >> 3;

		if (src == 6 && dst == 6) {
			HALT();
		}
		else if (src == 6 || dst == 6) {
			r8[6] = &ram[HL(H, L)]; // Look into making macro
			numCycles = 2;
        }

		LD_r8_r8(*this, r8[src], r8[dst]);

		std::cout << "\nAfter Operation: " << std::endl;
		debugPrint(opcode);

		// TODO: Add clock cycle
		break;
    case 0x80:
        /*Literally 200+ instructions in this case alone*/
        std::cerr << "Unimplemented opcode: " << opcode << std::endl;
        exit(1);
        break;
	case 0xC0:
		/* 8-bit arithmetic */
        std::cerr << "Unimplemented opcode: " << opcode << std::endl;
        exit(1);
		break;
    default:
        std::cerr << "Unimplemented opcode: " << opcode << std::endl;
		exit(1);
        break;
    }
}

// Reads a byte from memory and increments the PC
uint8_t CPU::readByte() {
	return ram[PC++];
}

void CPU::debugPrint(uint8_t opcode) {
    std::cout << "Debug State:" << std::endl;
    std::cout << "  Next Opcode: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(opcode) << std::endl;
    std::cout << "  A: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(A) << std::endl;
    std::cout << "  F: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(F) << " (ZNHC: "
        << ((F & 0x80) ? "1" : "0") << ((F & 0x40) ? "1" : "0") << ((F & 0x20) ? "1" : "0") << ((F & 0x10) ? "1" : "0") << ")" << std::endl; // Displays flags
    std::cout << "  B: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(B) << std::endl;
    std::cout << "  C: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(C) << std::endl;
    std::cout << "  D: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(D) << std::endl;
    std::cout << "  E: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(E) << std::endl;
    std::cout << "  H: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(H) << std::endl;
    std::cout << "  L: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(L) << std::endl;
    std::cout << "  HL: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(HL(H,L)) << std::endl;
    std::cout << "  [HL]: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ram[HL(H,L)]) << std::endl;
    std::cout << "  PC: 0x" << std::hex << std::setw(4) << std::setfill('0') << PC << std::endl; // should decrement to account; tis a future issue.
    std::cout << "  SP: 0x" << std::hex << std::setw(4) << std::setfill('0') << SP << std::endl;
}