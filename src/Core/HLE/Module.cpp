#include "Core/Memory.hpp"
#include "Module.hpp"
#include <vector>
#include <cassert>

namespace HLE {
uint32_t FetchArgument32W(Arm::Interface* arm, size_t idx) {
	uint32_t value;
	
	uint32_t sp = arm->ReadRegister(13);
	if (idx < 4) {
		value = arm->ReadRegister(idx);
	}
	else {
		sp += sizeof (uint32_t) * (idx - 4);
		Memory::Read32(value, sp);
	}
	
	return value;
}

void ReturnValue(Arm::Interface* arm, uint32_t value) {
	arm->WriteRegister(0, value);
}
}