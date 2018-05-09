#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <memory>
#include <cstring>

#include "Common/Types.hpp"
#include "Common/Log.hpp"
#include "Core/Loader/ElfLoader.hpp"
#include "Core/Arm/Arm.hpp"
#include "Core/Arm/ArmUnicorn.hpp"
#include "Core/Memory.hpp"
#include "Core/HLE/Kernel.hpp"

#include "Core/HLE/SceSysmem.hpp"
#include "Core/HLE/SceThreadmgr.hpp"


static void LoadFile(const std::string& path, std::vector<uint8_t>& data) {
	std::ifstream f(path, std::ios::binary);
	f.seekg(0, std::ios::end);
	auto s = f.tellg();
	f.seekg(0, std::ios::beg);
	data.resize(s);
	f.read(reinterpret_cast<char*>(&data[0]), data.size());
}

static void DumpCpu(std::ostream& os, Arm::Interface* arm) {
	static int inum = 0;
	uint32_t i = 0;
	uint32_t r;
	Arm::ThreadState threadState;

	arm->SaveState(threadState);

	os << "============================================================================\n";
	os << "* [CPU DUMP] Dump count: " << std::dec << inum << "\n";
	for (i = 0; i < 13; i++)
		os << "r" << std::dec << i << ": 0x" << std::hex << std::setfill('0') << std::setw(8) << threadState.r[i] << std::endl;
	os << "sp: 0x" << std::hex << std::setfill('0') << std::setw(8) << threadState.r[13] << std::endl;
	os << "lr: 0x" << std::hex << std::setfill('0') << std::setw(8) << threadState.r[14] << std::endl;
	os << "pc: 0x" << std::hex << std::setfill('0') << std::setw(8) << threadState.r[15] << std::endl;
	os << "cpsr: 0x" << std::hex << std::setfill('0') << std::setw(8) << threadState.psr << std::endl;
	os << "mode: 0x" << std::hex << std::setfill('0') << std::setw(8) << (threadState.psr & 0b11111) << std::endl;
	os << "============================================================================\n\n";
	inum++;
};

struct Test {
	uint32_t entryPoint;
	uint32_t result;
	uint32_t resultSize;
	const void* data;
};

int main(int argc, char** argv) {
	std::vector<uint8_t> elfData;
	Arm::Interface* arm = new Arm::UnicornInterface;
	HLE::Program program;

	Memory::RegisterArm(arm);
	HLE::g_kernel.Init(arm);

	HLE::RegisterSceSysmem();
	HLE::RegisterSceThreadmgr();

	LoadFile("tests/hello-world/bin/hello-world.elf", elfData);
	ElfInfo elfInfo(&elfData[0], elfData.size());
	LoadArmElf(elfInfo, program);

	uint32_t spc = 0xDEADBEEF;

	HLE::g_kernel.LoadProgram(program);
	
	while (1) {
		Arm::ThreadState threadState = {};
		Arm::ThreadState newThreadState = {};

		// Execute an arbitrarily large number of instructions. No
		// guarantee that all will execute
		if (!arm->Execute(1)) {
			abort();
		}
		arm->HaltExecution();
		DumpCpu(std::cout, arm);
		
		// Save the thread state. If currently at an HLE address, execute the HLE function
		arm->SaveState(threadState);
		if (HLE::g_kernel.HandleFunctionCall(threadState, newThreadState))
			arm->LoadState(newThreadState);

	}
	return 0;
}