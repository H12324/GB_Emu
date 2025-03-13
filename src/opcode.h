#pragma once

#include "CPU.h"
#include <iostream>

// Opcode functions will implement in opcode.cpp
void NOP();
void HALT() {
	std::cerr << "HALT instruction not implemented" << std::endl;
	exit(1);
}
void LD_r8_r8(CPU& cpu, uint8_t* src, uint8_t* dst) {
	*dst = *src;
}

void LD_r8_n(uint8_t* dst);
void LD_r16_nn(uint16_t* dst);
void LD_r16_A(uint16_t* dst);
void LD_A_r16(uint16_t* src);