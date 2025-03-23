#include "GB.h"

GB::GB(std::vector<uint8_t>& romData) {
    // Add a load ROM function
	cpu = new CPU(romData);
	// Could choose to store romData in GB class
}

GB::~GB() {
	delete cpu;
}

void GB::run() {
	cpu->debugPrint();
    cpu->step();
}