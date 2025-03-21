#pragma once

#include "CPU.h"
#include <iostream>
#include <iomanip>

void unimplemented_code(uint16_t opcode=0xFFF) {
	std::cerr << "Unimplemented opcode: 0x" << std::hex << opcode << std::endl;
	exit(1);
}

// Block 0: Lot's of different OPs 16-bit and imm 8-bit loads, misc, Math
void NOP() {
	// No Operation
	//unimplemented_code(0x00);
	// Just go one clock cycle
} 
void STOP() {
	unimplemented_code(0x10);
}

// 16-bit loads
void LD_r16_nn(CPU& cpu, uint8_t dst, uint16_t nn) {
	// Load 16-bit immediate value into register
	cpu.setR16(dst, nn);
}

void LD_nn_SP(CPU& cpu, uint16_t SP, uint16_t val) {
	// Load SP into 16-bit immediate value
	cpu.writeByte(SP & 0xFF, val); // could use getSP but this feels cleaner
	cpu.writeByte((SP & 0xFF00) >> 8, val + 1);
}

// 8-bit immediate and memory addr loads
void LD_a16_A(CPU& cpu, uint16_t dst) { // Also useful in Block 3
	// Load A into memory address at value in reg dst
	cpu.writeByte(cpu.getA(), dst);
}

void LD_A_a16(CPU& cpu, uint16_t src) {
	// Load value at memory address in src into A
	cpu.setA(cpu.readAddr(src));
}

void LD_r16mem_A(CPU& cpu, uint8_t dst) {
	// Load A into memory address at value in reg dst
	LD_a16_A(cpu, cpu.getR16(std::min(dst, (uint8_t)2)));
	if (dst == 2) cpu.setR16(2, cpu.getR16(2) + 1); // HL+
	else if (dst == 3) cpu.setR16(2, cpu.getR16(2) - 1); // HL-
}

void LD_A_r16mem(CPU& cpu, uint8_t src) {
	// Load value at memory address in reg src into A
	LD_A_a16(cpu, cpu.getR16(std::min(src, (uint8_t)2)));
	if (src == 2) cpu.setR16(2, cpu.getR16(2) + 1);
	else if (src == 3) cpu.setR16(2, cpu.getR16(2) - 1);
}

// Note: some of these functions are redundant like this one
void LD_r8_n(uint8_t* dst, int n) {
	// Load 8-bit immediate value into register
	*dst = n;
}


// 16-bit arithmetic
void INC_r16(CPU& cpu, uint8_t dst) {
	// Increment 16-bit register
	cpu.setR16(dst, cpu.getR16(dst) + 1);
}
void DEC_r16(CPU& cpu, uint8_t dst) {
	// Decrement 16-bit register
	cpu.setR16(dst, cpu.getR16(dst) - 1);
}
void ADD_HL_r16(CPU& cpu, uint8_t src) {
	// Add 16-bit value to HL
	uint16_t HL = cpu.getR16(2);
	uint16_t r16 = cpu.getR16(src);
	uint32_t res = HL + r16;

	cpu.setFlags(cpu.getZ(), 0, ((r16&0xFFF) + (HL & 0xFFF)) > 0x0FFF, res & 0x10000);
	cpu.setR16(2, res);
}

// 8-bit arithmetic
void INC_r8(CPU& cpu, uint8_t dst) {
	// Increment 8-bit register
	uint8_t val = cpu.getR8(dst);
	uint8_t res = val + 1;
	cpu.setR8(dst, res);
	cpu.setFlags((res & 0xFF) == 0, 0, ((val & 0xF) + 1) > 0x0F, cpu.getC());
}

void DEC_r8(CPU& cpu, uint8_t dst) {
	// Decrement 8-bit register
	uint8_t val = cpu.getR8(dst);
	uint8_t res = val - 1;
	cpu.setR8(dst, res);
	cpu.setFlags(res == 0, 1, (val & 0x0F) == 0, cpu.getC());
}

void DAA(CPU& cpu) {
	// Decial Adjust Accumulator (convert Acc result to BCD)
	uint8_t offset = 0;
	uint8_t A = cpu.getA();

	bool N = cpu.getN();
	bool H = cpu.getH();
	bool C = cpu.getC();
	
	if ((!N && (A & 0x0F) > 0x09) || H) offset = 0x06;
	if ((!N && A > 0x99) || C) {
		offset |= 0x60;
		C = true;
	}
	
	A = N ? A - offset : A + offset;
	cpu.setA(A);
	cpu.setFlags(A == 0, N, 0, C);
}
void SCF(CPU& cpu) {
	// Set carry flag
	cpu.setFlags(cpu.getZ(), 0, 0, 1);
}
void CCF(CPU& cpu) {
	//Complement Carry Flag
	cpu.setFlags(cpu.getZ(), 0, 0, !cpu.getC());
}
void CPL(CPU& cpu) {
	// Completment Accumulator
	cpu.setA(~cpu.getA());
	cpu.setN(1);
	cpu.setH(1);
}

// Rotate and shift functions
// Later make more defaul versions but for now
void RLCA(CPU& cpu) {
	// Rotate A left 
	uint8_t A = cpu.getA();
	uint8_t C = A >> 7;
	A = (A << 1) | C;
	cpu.setA(A);
	cpu.setFlags(0, 0, 0, C);
}
void RLA(CPU& cpu) {
	// Rotate A left through carry
	uint8_t A = cpu.getA();
	uint8_t C = A >> 7;
	uint8_t oldC = cpu.getC();
	A = (A << 1) | oldC;
	cpu.setA(A);
	cpu.setFlags(0, 0, 0, C);
}
void RRCA(CPU& cpu) {
	// Rotate A right 
	uint8_t A = cpu.getA();
	uint8_t C = A & 0x1;
	A = (C << 7) | (A >> 1);
	cpu.setA(A);
	cpu.setFlags(0, 0, 0, C);
}
void RRA(CPU& cpu) {
	// Rotate A right through carry
	uint8_t A = cpu.getA();
	uint8_t C = A & 0x1;
	A = (cpu.getC() << 7) | (A >> 1);
	cpu.setA(A);
	cpu.setFlags(0, 0, 0, C);
}


// Jump functions
void JR_n(CPU& cpu, int8_t dst) {
	// Jump relative by signed immediate value
	cpu.setPC(cpu.getPC() + dst);
}

bool JR_cc_n(CPU& cpu, uint8_t cc, int8_t dst) {
	// Conditional jump by signed immediate value
	bool cond = cpu.getCond(cc);
	if (cond) JR_n(cpu, dst);
	return cond;
}

// Block 1: 8-bit loads and HALT (0x40-0x7F)
void HALT() {
	std::cerr << "HALT instruction not implemented" << std::endl;
	exit(1);
}

void LD_r8_r8(CPU& cpu, uint8_t* src, uint8_t* dst) {
	*dst = *src;
}


// Block 2: 8-bit arithmetic (0x80-0xBF)
// Note all normally 1 cycle, but 2 if src is [HL]
void ADD_A_r8(CPU& cpu, uint8_t* src) {
	uint32_t res = cpu.getA() + *src;
	cpu.setFlags(res == 0, // Z if zero
				 0,		   // N is 0
		         ((cpu.getA() & 0x0F) + (*src & 0x0F)) > 0x0F, // H if overflow from bit 3
				 res > 0xFF // C set if overflow from bit 7
		);
	cpu.setA(res);
}

void ADC_A_r8(CPU& cpu, uint8_t* src) {
	uint32_t res = cpu.getA() + *src + cpu.getC();
	cpu.setFlags(!res, 0, ((cpu.getA() & 0x0F) + (*src & 0x0F) + cpu.getC()) & 0x10, res & 0x100);
	cpu.setA(res);
}

void SUB_A_r8(CPU& cpu, uint8_t* src) {
	uint8_t A = cpu.getA();
	uint8_t r8 = *src;
	uint32_t res = cpu.getA() - *src;
	
	cpu.setFlags(!res, 1, (r8 & 0x0F) > (A & 0x0F), r8 > A); // looks cleaner than spamming getA() and *src
	cpu.setA(res);
}

// Maybe just remove pointer and pass directly from call
void SBC_A_r8(CPU& cpu, uint8_t* src) {
	uint8_t A = cpu.getA();
	uint8_t r8 = *src;
	uint8_t C = cpu.getC();
	uint32_t res = A - r8 - cpu.getC();

	cpu.setFlags(!res, 1, C + (r8 & 0x0F) > (A & 0x0F), r8 + C > A); 
	cpu.setA(res);
}
// Experiment with alternative function since I can reuse with immediates
void AND_A_r8(CPU& cpu, uint8_t* r8) {
	uint8_t A = cpu.getA();
	uint8_t res = A & *r8;
	cpu.setFlags(!res, 0, 1, 0);
	cpu.setA(res);
}
void XOR_A_r8(CPU& cpu, uint8_t* src) {
	uint8_t res = cpu.getA() ^ *src;
	cpu.setFlags(!res, 0, 0, 0);
	cpu.setA(res);
}
void OR_A_r8(CPU& cpu, uint8_t* src) {
	uint8_t res = cpu.getA() | *src;
	cpu.setFlags(!res, 0, 0, 0);
	cpu.setA(res);
}

// Could have potentially called this from within SUB_A_r8
void CP_A_r8(CPU& cpu, uint8_t* src) {
	uint8_t A = cpu.getA();
	uint8_t r8 = *src;
	uint32_t res = A - r8;
	cpu.setFlags(!res, 1, (A & 0x0F) < (r8 & 0x0F), A < r8);
}

// Function pointers for above calls
using FunctionPtr = void (*)(CPU&, uint8_t*);
FunctionPtr r8_ArithTable[] = {
	ADD_A_r8,
	ADC_A_r8,
	SUB_A_r8,
	SBC_A_r8,
	AND_A_r8,
	XOR_A_r8,
	OR_A_r8,
	CP_A_r8
};

//using CondPtr = void (*)(CPU&, uint8_t*);
// Block 3: Misc part 2
// DI and EI
void DI() {
	// Disable interrupts
	unimplemented_code(0xF3);
	//cpu.setIME(false);
}

void EI() { // Fix RETI After
	// Enable interrupts
	unimplemented_code(0xFB);
	//cpu.setIME(true);
}

// Immediate math (reused ArithTable)

// Returns, jumps, and calls
void RET(CPU& cpu) {
	// Return from subroutine
	uint16_t SP = cpu.getSP();
	uint8_t LSB = cpu.readAddr(SP++);
	uint8_t MSB = cpu.readAddr(SP++);
	cpu.setSP(SP);
	cpu.setPC(U16(MSB, LSB));
}

bool RET_cc(CPU& cpu, uint8_t cc) {
	// Conditional return from subroutine
	bool cond = cpu.getCond(cc);
	if (cond) RET(cpu);
	return cond;
}

void RETI(CPU& cpu) {
	// Return from interrupt
	EI();
	RET(cpu);
	//cpu.setIME(true);
}

void JP_nn(CPU& cpu, uint16_t n16) {
	// Jump to 16-bit immediate value
	cpu.setPC(n16);
}

// Immediate loads from and to memory
// Note: LD_A16_A and LD_A_A16 are already defined earlier
// Could technically get rid of all these but I think it's slightly cleaner to have them
void LDH_a8_A(CPU& cpu, uint8_t a8) {
	// Load A into memory address 0xFF00 + a8
	LD_a16_A(cpu, 0xFF00 + a8);
}
void LDH_A_a8(CPU& cpu, uint8_t a8) {
	// Load value at memory address 0xFF00 + a8 into A
	LD_A_a16(cpu, 0xFF00 + a8);
}
void LDH_C_A(CPU& cpu) {
	// Load A into memory address 0xFF00 + C
	LD_a16_A(cpu, 0xFF00 + cpu.getC());
}
void LDH_A_C(CPU& cpu) {
	// Load value at memory address 0xFF00 + C into A
	LD_A_a16(cpu, 0xFF00 + cpu.getC());
}

// Push and Pop
void PUSH_r16(CPU& cpu, uint8_t reg) {
	// Push 16-bit register onto stack
	// Reg = 3 is indexing AF in this case confusing but it's how it is
	if (reg == 3) reg = 4; // AF Case
	uint16_t val = cpu.getR16(reg);

	DEC_r16(cpu, 3); // DEC SP
	cpu.writeByte((val >> 8) & 0xFF, cpu.getSP()); // Write MSB
	DEC_r16(cpu, 3); // DEC SP
	cpu.writeByte(val & 0xFF, cpu.getSP()); // Write LSB
}

void POP_r16(CPU& cpu, uint8_t reg) {
	// Pop 16-bit register from stack
	if (reg == 3) reg = 4; // AF Case
	uint16_t SP = cpu.getSP();
	uint8_t LSB = cpu.readAddr(SP++); // Read LSB
	uint8_t MSB = cpu.readAddr(SP++); // Read MSB
	cpu.setSP(SP);

	if (reg == 4) LSB &= 0xF0; // Assure last 4 bits of F are 0
	cpu.setR16(reg, U16(MSB, LSB));
}

// SP related add and loads
void ADD_SP_n(CPU& cpu, int8_t n) {
	// Add signed immediate value to SP
	uint16_t SP = cpu.getSP();
	uint32_t res = SP + n;
	cpu.setSP(res);
	cpu.setFlags(0, 0, ((SP & 0x0F) + (n & 0x0F)) > 0x0F, ((SP & 0xFF) + n) > 0xFF);
}

void LD_SP_HL(CPU& cpu) {
	// Load HL into SP
	cpu.setSP(cpu.getR16(2)); // Could have made a reg:: enum
}

void LD_HL_SP_n(CPU& cpu, int8_t n) {
	// Load SP + signed immediate value into HL
	uint16_t SP = cpu.getSP();
	uint32_t res = SP + n;
	cpu.setR16(2, res);
	cpu.setFlags(0, 0, ((SP & 0x0F) + (n & 0x0F)) > 0x0F, ((SP & 0xFF) + n) > 0xFF);
}

// The 256 0xCB prefixed opcodes