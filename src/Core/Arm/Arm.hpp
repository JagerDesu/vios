#pragma once

#include "Common/Types.hpp"
#include "Common/Bits.hpp"
#include "Core/Arm/ArmUop.hpp"
#include "Core/Memory.hpp"
#include <cstdlib>
#include <unordered_map>
#include <cassert>

namespace Arm {

enum class ProgramCounterOffset {
	Thumb = 4,
	Arm = 8
};

struct Psr {
	bool n;
	bool z;
	bool c;
	bool v;
	bool q;
	bool j;
	bool e;
	bool t;
};

struct State : public NonCopyable {
	State();
	~State();

	inline bool HandleN(uint32_t value) {
		bool signBit = (bool)BIT_SHIFT(value, 31);
		cpsr.n = signBit;
		return cpsr.n;
	}

	inline bool HandleZ(uint32_t value) {
		cpsr.z = value == 0;
		return cpsr.z;
	}

	inline bool HandleV(uint32_t oldValue, uint32_t newValue, uint32_t msb) {
		uint32_t a = BIT_SHIFT(oldValue, msb);
		uint32_t b = BIT_SHIFT(newValue, msb);
		bool result = (bool)(a ^ b);
		cpsr.v = result;
		return cpsr.v;
	}

	/*
	 * Sets bit 0 to either 1 or zero if the CPU is in thumb or arm mode, respectively.
	 */
	inline uint32_t MakeAddressJumpable(uint32_t address) {
		uint32_t bit0 = cpsr.t ? 1 : 0;
		CHANGE_BIT(address, 0, bit0);
		return address;
	}

	bool CheckCondition(Condition cond);

	bool CheckFlagCondition(FlagCondition cond);

	void ALUWritePC(uint32_t value);
	void BranchTo(uint32_t value);
	
	uint32_t r[13];
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;

	uint32_t isetstate;
	Psr cpsr;

	uint64_t iterations;

	bool itblock;
};


struct Dispatcher {
	virtual void Dispatch(State& state, const void* stream, size_t size) = 0;
};

// Represents CPU thread state
//
// uint32_t r[13] must contain sp, lr, and pc in that order after 
struct ThreadState {
	uint32_t r[13];
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;
	uint64_t d[32];
	uint32_t psr;
};

struct Interface {
~Interface() {}
virtual void MapHostMemory(uint32_t address, size_t size, void* buffer, Memory::Protection protection) = 0;
virtual void UnmapHostMemory(uint32_t address, size_t size) = 0;
virtual void RegisterMemoryCallback(Memory::Callback* callback, uint32_t address, uint32_t size) = 0;

virtual void ReadMemory(void* buffer, uint32_t address, size_t size) = 0;
virtual void WriteMemory(const void* buffer, uint32_t address, size_t size) = 0;

virtual void LoadState(const ThreadState& state) = 0;
virtual void SaveState(ThreadState& state) = 0;
virtual uint32_t ReadCpsr() = 0;
virtual void WriteCpsr(uint32_t cpsr) = 0;
virtual uint32_t ReadRegister(uint32_t idx) = 0;
virtual void WriteRegister(uint32_t idx, uint32_t value) = 0;
virtual bool Execute(uint32_t numInstructions) = 0;
virtual void HaltExecution() = 0;
};
}