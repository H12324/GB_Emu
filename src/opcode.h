#pragma once

#include "CPU.h"
#include <iostream>


// Block 0: Misc, 16-
void NOP();

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
	/* Alternative Approach
	* cpu.setFlags(0)
	if (!res) cpu.setZ(1);
	cpu.setN(0);
	if ((cpu.getA() & 0x0F) + (*src & 0x0F) > 0x0F) cpu.setH(1);
	if (res > 0xFF) cpu.setH(1);*/

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

// Block 3: Misc part 2
void LD_r8_n(uint8_t* dst);
void LD_r16_nn(uint16_t* dst);
void LD_r16_A(uint16_t* dst);
void LD_A_r16(uint16_t* src);

