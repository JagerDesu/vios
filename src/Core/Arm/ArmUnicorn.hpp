#pragma once

#include "Arm.hpp"

#include <unicorn/unicorn.h>
#include <vector>

namespace Arm {
class UnicornInterface : public Interface {
public:
	UnicornInterface(bool thumb = true);
	~UnicornInterface();
	void LoadState(const ThreadState& state);
	void SaveState(ThreadState& state);
	uint32_t ReadCpsr();
	void WriteCpsr(uint32_t cpsr);
	uint32_t ReadRegister(uint32_t idx);
	void WriteRegister(uint32_t idx, uint32_t value);
	bool Execute(uint32_t numInstructions);
	void HaltExecution();

	void MapHostMemory(uint32_t address, size_t size, void* buffer, Memory::Protection protection);
	void UnmapHostMemory(uint32_t address, size_t size);
	void RegisterMemoryCallback(Memory::Callback* callback, uint32_t address, uint32_t size);
	
	void ReadMemory(void* buffer, uint32_t address, size_t size);
	void WriteMemory(const void* buffer, uint32_t address, size_t size);
private:
	uc_engine* engine;
	std::vector<uc_hook> hooks;
};
}