#include "ArmUnicorn.hpp"
#include "Common/Log.hpp"
#include <cassert>

namespace Arm {
UnicornInterface::UnicornInterface(bool thumb) {
	uc_err error;
	uint64_t regval = 0;
	uint64_t fpexc = 0x40000000;
	
	auto mode = thumb ? UC_MODE_THUMB : UC_MODE_ARM;
	error = uc_open(UC_ARCH_ARM, mode, &engine);

	// Enable VFP
	uc_reg_read(engine, UC_ARM_REG_C1_C0_2, &regval);
	regval |= (0xF << 20);
	uc_reg_write(engine, UC_ARM_REG_C1_C0_2, &regval);
	uc_reg_write(engine, UC_ARM_REG_FPEXC, &fpexc);

	// Set to user mode
	uint32_t newCpsr = ReadCpsr();
	newCpsr &= ((uint32_t)~0b11111);
	newCpsr |= 0b10000;
	WriteCpsr(newCpsr);
}

UnicornInterface::~UnicornInterface() {
	if (engine)
		uc_close(engine);
}

void UnicornInterface::LoadState(const ThreadState& state) {
	int fregs[32];
	void* fvals[32];
	uc_err err;
	for (size_t i = 0; i < 16; i++)
		WriteRegister(i, state.r[i]);
	for (size_t i = 0; i < 32; i++) {
		fregs[i] = UC_ARM_REG_D0 + i;
		fvals[i] = (void*)(&state.d[i]);
	}
	WriteCpsr(state.psr);
	err = uc_reg_write_batch(engine, fregs, fvals, 32);
	assert(err == UC_ERR_OK);
}

void UnicornInterface::SaveState(ThreadState& state) {
	int fregs[32];
	void* fvals[32];
	uc_err err;
	for (size_t i = 0; i < 16; i++)
		state.r[i] = ReadRegister(i);
	for (size_t i = 0; i < 32; i++) {
		fregs[i] = UC_ARM_REG_D0 + i;
		fvals[i] = (void*)(&state.d[i]);
	}
	err = uc_reg_read_batch(engine, fregs, fvals, 32);
	state.psr = ReadCpsr();
	assert(err == UC_ERR_OK);
}

uint32_t UnicornInterface::ReadCpsr() {
	uc_err err;
	uint64_t value;
	err = uc_reg_read(engine, UC_ARM_REG_CPSR, &value);
	assert(err == UC_ERR_OK);
	return static_cast<uint32_t>(value);
}

void UnicornInterface::WriteCpsr(uint32_t cpsr) {
	uc_err err;
	uint64_t value = static_cast<uint64_t>(cpsr);
	err = uc_reg_write(engine, UC_ARM_REG_CPSR, &value);
	assert(err == UC_ERR_OK);
}

uint32_t UnicornInterface::ReadRegister(uint32_t idx) {
	uint64_t value = 0;
	uc_arm_reg reg;
	uc_err err;
	assert(idx < 16);

	switch (idx) {
		case 13:
			reg = UC_ARM_REG_SP;
			break;
		case 14:
			reg = UC_ARM_REG_LR;
			break;
		case 15:
			reg = UC_ARM_REG_PC;
			break;
		default:
			assert(idx < 13);
			reg = (uc_arm_reg)((int)UC_ARM_REG_R0 + idx);
			break;
	}
	err = uc_reg_read(engine, reg, &value);
	assert(err == UC_ERR_OK);
	return static_cast<uint32_t>(value);
}

void UnicornInterface::WriteRegister(uint32_t idx, uint32_t value) {
	uint64_t value64 = static_cast<uint32_t>(value);
	uc_arm_reg reg;
	uc_err err;
	assert(idx < 16);

	switch (idx) {
		case 13:
			reg = UC_ARM_REG_SP;
			break;
		case 14:
			reg = UC_ARM_REG_LR;
			break;
		case 15:
			reg = UC_ARM_REG_PC;
			break;
		default:
			assert(idx < 13);
			reg = (uc_arm_reg)((int)UC_ARM_REG_R0 + idx);
			break;
	}
	
	err = uc_reg_write(engine, reg, &value64);
	assert(err == UC_ERR_OK);
}

bool UnicornInterface::Execute(uint32_t numInstructions) {
	uint32_t pc = ReadRegister(15);
	uint32_t cpsr = ReadCpsr();
	uint32_t isThumb = (cpsr >> 5) & (~(uint32_t)0xFFFFFFE); // T [thumb] bit is located in bit 5
	uc_err error;

	if (isThumb)
		pc |= 1;

	error = uc_emu_start(engine, pc, ((uint32_t)1) << 31, 0, numInstructions);

	if (error != UC_ERR_OK) {
		uint32_t instructionAddress = pc & (~(uint32_t)1);
		uint32_t instr = 0;
		pc = ReadRegister(15);
		Memory::Read32(instr, instructionAddress);
		LOG_INFO(Arm, "Break in emulation at 0x%08X. Instr: 0x%08X", pc, instr);
		return false;
	}
	
	return true;
}

void UnicornInterface::HaltExecution() {
	uc_err error = uc_emu_stop(engine);
	assert(error == UC_ERR_OK);
}

void UnicornInterface::MapHostMemory(uint32_t address, size_t size, void* buffer, Memory::Protection protection) {
	LOG_INFO(Arm, "Mapping address 0x%08X to 0x%08X with protection 0x%02X", address, (address + (uint32_t)size), (int)protection);
	uc_err err;
	uint32_t prot = 0;
	if (((int)protection & (int)Memory::Protection::Read) == (int)Memory::Protection::Read)
		prot |= UC_PROT_READ;

	if (((int)protection & (int)Memory::Protection::Write) == (int)Memory::Protection::Write)
		prot |= UC_PROT_WRITE;

	if (((int)protection & (int)Memory::Protection::Execute) == (int)Memory::Protection::Execute)
		prot |= UC_PROT_EXEC;
	err = uc_mem_map_ptr(engine, address, size, prot, buffer);
	assert(err == UC_ERR_OK);
}

void UnicornInterface::UnmapHostMemory(uint32_t address, size_t size) {
	uc_err err;
	err = uc_mem_unmap(engine, address, size);
	assert(err == UC_ERR_OK);
}

static void CallbackHook(uc_engine* engine, uint64_t address, uint32_t size, void* userData) {
	const uint32_t BufferSize = 256;
	uint8_t buffer[BufferSize];

	while (size) {
		uint32_t readSize = static_cast<uint32_t>(size % BufferSize);
		if (size < readSize)
			readSize = size;
		Memory::Write(buffer, static_cast<uint32_t>(address), readSize);
		size -= readSize;
	}
}

void UnicornInterface::RegisterMemoryCallback(Memory::Callback* callback, uint32_t address, uint32_t size) {
	hooks.push_back(uc_hook{});
	auto& hook = hooks[hooks.size() - 1];
	uc_err err;
	err = uc_hook_add(engine, &hook, UC_HOOK_CODE, (void*)&CallbackHook, callback, address, address + size);
}

void UnicornInterface::ReadMemory(void* buffer, uint32_t address, size_t size) {
	uc_err err;
	err = uc_mem_read(engine, address, buffer, size);
	assert(err == UC_ERR_OK);
}

void UnicornInterface::WriteMemory(const void* buffer, uint32_t address, size_t size) {
	uc_err err;
	err = uc_mem_write(engine, address, buffer, size);
	assert(err == UC_ERR_OK);
}

}