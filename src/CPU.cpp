#include "CPU.h"
#include "opcode.h"
#include <iostream>
#include <iomanip>
#include <cstring>

CPU::CPU(std::vector<uint8_t>& romData) 
    : PC(0x100), SP(0xFFFE), H(0x01), L(0x4D), F(0xB0),
      A(0x01), B(0x00), C(0x13), D(0x00), E(0xD8)  {

	// Skip boot rom

	std::memcpy(ram, romData.data(), romData.size());
	// TODO: Initialize rest of the ram later once it is more relevant

}

void CPU::step() {
    uint8_t opcode = readByte();

	uint8_t src = 0;
	uint8_t dst = 0;
	uint8_t numCycles = 1;

	std::cout << "Before Operation: " << std::endl;
	debugPrint(opcode); // Note: PC is already incremented so it should really be one before account for that later

    switch (opcode & 0xC0)
    {
	case 0x00: // Block 0: Lot's of different OPs 16-bit and imm 8-bit loads, misc, Math
        // Misc
        if (opcode == 0x00) NOP(); 
        else if (opcode == 0x10) STOP(); // Read-byte? stop clock?

        // 16-bit loads
        else if (opcode == 0x08 || (opcode & 0x0F) == 0x01) {
            uint16_t nn = getImm16(); // Maybe have this helper update numcycles
            numCycles += 2;

            if (opcode == 0x08) {
                numCycles++;
                LD_nn_SP(*this, SP, nn);
            }
            else {
                dst = (opcode & 0x30) >> 4; // 0x30 is 0011 0000
                LD_r16_nn(*this, dst, nn); // Gotta love functions that just call other functions
            }
        }

        // 16-bit Arithmetic
        else if ((opcode & 0x7) == 0x3 || (opcode & 0x0F) == 0x9) { // 16-bit INCs
            numCycles++;
            dst = (opcode >> 4) & 0x3;
			if ((opcode & 0x0F) == 0x3) INC_r16(*this, dst);
			else if ((opcode & 0x0F) == 0xB) DEC_r16(*this, dst);
			else ADD_HL_r16(*this, dst);
        }

        // 8-bit loads
		else if ((opcode & 07) == 06) { // 8-bit immediate loads
            numCycles++;

			uint8_t imm8 = readByte();
			dst = (opcode >> 3) & 0x07; 
            // make into helper?
            if (dst == 6) { 
                r8[6] = &ram[HL(H, L)];
                numCycles++;
            }
			LD_r8_n(r8[dst], imm8); // Could also use setR8(dst, imm8);
        }
		else if ((opcode & 07) == 02) { // 8-bit loads from/to memory
            numCycles++;
            dst = (opcode >> 4) & 0x07;
            
            if ((opcode & 0x0F) == 0x02) LD_r16mem_A(*this, dst); // LD [dst] , A
            else LD_A_r16mem(*this, dst); // LD A, [src]
        }

		// 8-bit arithmetic
		else if ((opcode & 0x7) == 0x4 || (opcode & 0x07) == 0x5) { // 8-bit INCs
            dst = (opcode >> 3) & 0x7; 
            src = opcode & 0x7;
            if (src == 0x4) INC_r8(*this, dst);
            else DEC_r8(*this, dst);
        }
		else if (opcode == 0x27) DAA(*this);
		else if (opcode == 0x2F) CPL(*this);
        else if (opcode == 0x37) SCF(*this);
		else if (opcode == 0x3F) CCF(*this);

        // Rotates and shifts
		else if (opcode == 0x07) RLCA(*this);
		else if (opcode == 0x17) RLA(*this);
		else if (opcode == 0x0F) RRCA(*this);
		else if (opcode == 0x1F) RRA(*this);

        // Relative Jumps
        else if (opcode == 0x18 || 
                (opcode & 0xE0) == 0x20 && (opcode & 0x07) == 0) {
			numCycles++;
			int8_t n = readByte();
			if (opcode == 0x18) {
				numCycles++;
				JR_n(*this, n);
			}
			else {
				uint8_t cc = (opcode & 0x18) >> 3;
                if (JR_cc_n(*this, cc, n)) numCycles++;
			}
        }
        else {
            unimplemented_code(opcode);
        }
        break;
	case 0x40: // Block 1: LD r8 r8 instructions
		/* 8-bit loads */ // 0x40-0x7F
        src = (opcode & 0x07); // feels strange using regular ints now
		dst = (opcode & 0x38) >> 3;

		if (src == 6 && dst == 6) { // should just check if opcode is 0x76
			HALT();
            break;
		}
		else if (src == 6 || dst == 6) {
			r8[6] = &ram[HL(H, L)]; 
			numCycles++;
        }

		LD_r8_r8(*this, r8[src], r8[dst]);
		break;
	case 0x80: // Block 2: 8-bit arithmetic
        src = opcode & 0x07; // operand
        if (src == 0x06) { // [HL] case
            r8[6] = &ram[HL(H, L)];
            numCycles = 2;
        }
		dst = (opcode >> 3) & 0x07; // Operation
		r8_ArithTable[dst](*this, r8[src]);
        break;
	case 0xC0: // Block 3: Misc part 2
        // 8-bit immediate arithmetic
        dst = opcode & 0x07; // Generally grouped by last 3-bits (column mask)
        src = opcode & 0x0F; // Alternate column mask 
        if (dst == 0x06) {
            numCycles++;
			uint8_t imm8 = readByte(); // I regret not making a u8 typedef
            uint8_t op = (opcode >> 3) & 07; // learning octal
			r8_ArithTable[op](*this, &imm8); // Should have just made functions take value
            break;
        }
         
        // Jumps, calls, and returns
        // - Returns
        if (opcode == 0xC9) {
            numCycles += 3;
            RET(*this);
        }
        else if (opcode == 0xD9) {
            numCycles += 3;
            RETI(*this);
        }
        else if ((opcode & 0xE0) == 0xC0 && (opcode & 0x07) == 0) { // RET cc
            uint8_t cc = (opcode >> 3) & 0x03;
            numCycles++;
            if (RET_cc(*this, cc)) numCycles += 2;
        }
        // - Jumps
        else if (opcode == 0xC3) { // JP imm16
            numCycles += 3;
			uint16_t nn = getImm16();
            JP_nn(*this, nn);
        }
		else if (opcode == 0xE9) JP_nn(*this, HL(H, L)); // JP HL
        else if ((opcode & 0xE0) == 0xC0 && (opcode & 0x07) == 0) { // JP CC, imm16
            numCycles += 2;
            uint16_t nn = getImm16();
			uint8_t cc = (opcode >> 3) & 0x03;
			if (JP_nn_cc(*this, cc, nn)) numCycles++;
        }
        // - Calls
        else if (opcode == 0xCD) { // CALL imm16
            numCycles += 5;
			CALL_nn(*this, getImm16());
		}
        else if ((opcode & 0xE0) == 0xC0 && (opcode & 0x07) == 0x04) { // CALL CC, imm16
            numCycles += 2;
            uint8_t cc = (opcode >> 3) & 0x03;
			if (CALL_cc_nn(*this, cc, getImm16())) numCycles+=3;
        }
        else if (dst == 0x07) { // RST tgt basically fast CALL
            numCycles += 3;
			uint8_t tgt = opcode & 0x78; // 0111 1000
			RST(*this, tgt); // tgt*8
        }
        // Push and Pop Stack
		else if (src == 0x1 || src == 0x5) // 0x_1 and 0x_5 are PUSH and POP
		{
			numCycles+=2;
			uint8_t reg = (opcode >> 4) & 0x3;
			if (src == 0x1) POP_r16(*this, reg);
            else {
				numCycles++;
                PUSH_r16(*this, reg);
            }
		}
        // SP Load and Add
        else if (opcode == 0xE8) {
            numCycles += 3;
			int8_t n = readByte();
			ADD_SP_n(*this, n);
        }
        else if (opcode == 0xF8) {
            numCycles += 2;
            int8_t n = readByte();
			LD_HL_SP_n(*this, n);
        }
		else if (opcode == 0xF9) {
			numCycles++;
			LD_SP_HL(*this);
		}
        // Rest of the 8-bit loads
        else if (opcode == 0xE0 || opcode == 0xF0) {
			numCycles+=2;
			uint8_t a8 = readByte();
			if (opcode == 0xE0) LDH_a8_A(*this, a8);
			else LDH_A_a8(*this, a8);
		}
        else if (opcode == 0xE2 || opcode == 0xF2) {
			numCycles++;
			if (opcode == 0xE2) LDH_C_A(*this);
			else LDH_A_C(*this);
        }
        else if (opcode == 0xEA || opcode == 0xFA) {
			numCycles += 3;
			uint8_t LSB = readByte();
			uint8_t MSB = readByte();
			if (opcode == 0xEA) LD_a16_A(*this, U16(MSB, LSB));
			else LD_A_a16(*this, U16(MSB, LSB));
        }
        // EI and DI
        // CB Chasm
		else unimplemented_code(opcode);
		break;
    default:
		unimplemented_code(opcode); // Or illegal opcode
        break;
    }

    std::cout << "\nAfter Operation: 0x" << std::hex << (uint32_t)opcode <<  std::endl;
    debugPrint(opcode);
}

// Reads a byte from memory and increments the PC
uint8_t CPU::readByte() {
	return ram[PC++];
}

uint8_t CPU::readAddr(uint16_t addr) {
	return ram[addr];
}

void CPU::writeByte(uint8_t val, uint16_t addr) {
	ram[addr] = val;
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