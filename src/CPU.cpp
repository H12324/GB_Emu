#include "CPU.h"
#include "opcode.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>

CPU::CPU(std::vector<uint8_t>& romData) 
    : A(0x01), F(0xB0), B(0x00), C(0x13), D(0x00), E(0xD8),
      H(0x01), L(0x4D), PC(0x100), SP(0xFFFE) {

    rom = romData;
    romBankCount = std::max<uint8_t>(1, static_cast<uint8_t>(rom.size() / 0x4000));
	std::memcpy(ram, romData.data(), std::min<size_t>(romData.size(), 0x8000));
	// TODO: Initialize rest of the ram later once it is more relevant
    ram[0xFF44] = 0x90; // Skips waiting for V-blank or something
}

void CPU::step() {
    if (serviceInterrupt()) return;

    if (halted || stopped) {
        if (!interruptPending()) {
            tickTimers(1);
            return;
        }

        halted = false;
        stopped = false;
        if (serviceInterrupt()) return;
    }

    uint8_t opcode = readByte();

	uint8_t src = 0;
	uint8_t dst = 0;
	uint8_t numCycles = 1;

    switch (opcode & 0xC0)
    {
	case 0x00: // Block 0: Lot's of different OPs 16-bit and imm 8-bit loads, misc, Math
        // Misc
        if (opcode == 0x00) NOP(); 
        else if (opcode == 0x10) STOP(*this); // Read-byte? stop clock?

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
            if (dst == 6) numCycles++;
			setR8(dst, imm8);
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
                ((opcode & 0xE0) == 0x20 && (opcode & 0x07) == 0)) {
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
			HALT(*this);
            break;
		}
		else if (src == 6 || dst == 6) numCycles++;

		setR8(dst, getR8(src));
		break;
	case 0x80: // Block 2: 8-bit arithmetic
        src = opcode & 0x07; // operand
        uint8_t hlVal;
        if (src == 0x06) { // [HL] case
            hlVal = getR8(6);
            r8[6] = &hlVal;
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
        else if ((opcode & 0xE0) == 0xC0 && (opcode & 0x07) == 0x2) { // JP CC, imm16
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
			uint8_t tgt = opcode & 0x38; // 0011 1000
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
		else if (opcode == 0xF3) DI(*this);
		else if (opcode == 0xFB) EI(*this);
		
        // CB Chasm
		else if (opcode == 0xCB) {
			numCycles++;
			uint8_t cbOp = readByte(); // 0xCB Opcode
            uint8_t reg = cbOp & 0x07;
            uint8_t block = cbOp >> 6;
            uint8_t bit = (cbOp >> 3) & 0x07;

            if (reg == 0x06) numCycles += 2; // [HL] case
            if (block == 0) r8_ManipTable[bit](*this, reg);
            else {
				if (block == 1 && reg == 0x06) numCycles--; // BIT has 3 cycles in [HL] for some reason
                r8_BitTable[block - 1](*this, bit, reg);
            }
		}
		else unimplemented_code(opcode); // Fake upcodes can't hurt you
		break;
    default:
		unimplemented_code(opcode); // Or illegal opcode
        break;
    }

    tickTimers(numCycles);
    updateIMEAfterInstruction();
}

bool CPU::serviceInterrupt() {
    uint8_t pending = ram[0xFFFF] & ram[0xFF0F] & 0x1F;
    if (!IME || pending == 0) return false;

    halted = false;
    stopped = false;
    IME = false;
    imeEnableDelay = 0;

    uint8_t interrupt = 0;
    while (((pending >> interrupt) & 0x1) == 0) interrupt++;

    ram[0xFF0F] &= ~(1 << interrupt);
    writeByte((PC >> 8) & 0xFF, --SP);
    writeByte(PC & 0xFF, --SP);
    PC = 0x40 + interrupt * 0x08;
    return true;
}

void CPU::tickTimers(uint8_t machineCycles) {
    const uint16_t cycles = machineCycles * 4;

    divCounter += cycles;
    ram[0xFF04] = divCounter >> 8;

    if ((ram[0xFF07] & 0x04) == 0) return;

    static constexpr uint16_t periods[] = {1024, 16, 64, 256};
    timerCounter += cycles;
    const uint16_t period = periods[ram[0xFF07] & 0x03];

    while (timerCounter >= period) {
        timerCounter -= period;
        if (ram[0xFF05] == 0xFF) {
            ram[0xFF05] = ram[0xFF06];
            ram[0xFF0F] |= 0x04;
        }
        else {
            ram[0xFF05]++;
        }
    }
}

// Reads a byte from memory and increments the PC
uint8_t CPU::readByte() {
    if (haltBug) {
        haltBug = false;
        return readAddr(PC);
    }
	return readAddr(PC++);
}

uint8_t CPU::readAddr(uint16_t addr) {
    if (addr < 0x4000) {
        return addr < rom.size() ? rom[addr] : 0xFF;
    }

    if (addr < 0x8000) {
        size_t bankAddr = static_cast<size_t>(currentRomBank) * 0x4000 + (addr - 0x4000);
        return bankAddr < rom.size() ? rom[bankAddr] : 0xFF;
    }

	return ram[addr];
}

void CPU::writeByte(uint8_t val, uint16_t addr) {
    if (addr < 0x8000) {
        if (addr >= 0x2000 && addr < 0x4000) {
            uint8_t bank = val & 0x1F;
            if (bank == 0) bank = 1;
            currentRomBank = bank % romBankCount;
            if (currentRomBank == 0) currentRomBank = 1;
        }
        return;
    }

    if (addr == 0xFF04) {
        divCounter = 0;
        ram[addr] = 0;
        return;
    }

    // Intercept serial output, won't work with [HL] arithmetic though
	if (addr == 0xFF02 && (val & 0x81) == 0x81) {
		char c = ram[0xFF01];
		std::cout << c << std::flush;
        ram[0xFF02] &= 0x7F;
	}
	ram[addr] = val;
}

// Alternate debugPrint to match GB Doctor
void CPU::debugPrint() {
    std::cout << std::hex << std::uppercase << std::setfill('0');

    std::cout << "A:" << std::setw(2) << static_cast<int>(A) << " ";
    std::cout << "F:" << std::setw(2) << static_cast<int>(F) << " ";
    std::cout << "B:" << std::setw(2) << static_cast<int>(B) << " ";
    std::cout << "C:" << std::setw(2) << static_cast<int>(C) << " ";
    std::cout << "D:" << std::setw(2) << static_cast<int>(D) << " ";
    std::cout << "E:" << std::setw(2) << static_cast<int>(E) << " ";
    std::cout << "H:" << std::setw(2) << static_cast<int>(H) << " ";
    std::cout << "L:" << std::setw(2) << static_cast<int>(L) << " ";
    std::cout << "SP:" << std::setw(4) << SP << " ";
    std::cout << "PC:" << std::setw(4) << PC << " ";

    // Print PCMEM (PC, PC+1, PC+2, PC+3)
    std::cout << "PCMEM:" 
              << std::setw(2) << static_cast<int>(ram[PC]) << ","
              << std::setw(2) << static_cast<int>(ram[PC + 1]) << ","
              << std::setw(2) << static_cast<int>(ram[PC + 2]) << ","
              << std::setw(2) << static_cast<int>(ram[PC + 3]);

    std::cout << std::endl;
}
