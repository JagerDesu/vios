#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <memory>
#include <cstring>
#include <iostream>

#include "Common/Types.hpp"
#include "Common/Log.hpp"
#include "Core/Loader/ElfLoader.hpp"
#include "Core/Arm/Arm.hpp"
#include "Core/Arm/ArmGdbStub.hpp"
#include "Core/Arm/ArmUnicorn.hpp"
#include "Core/Memory.hpp"
#include "Core/HLE/Kernel.hpp"

#include "Core/HLE/SceSysmem.hpp"
#include "Core/HLE/SceThreadmgr.hpp"
#include "Core/HLE/SceLibKernel.hpp"

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

struct DebugIoCallback : public Memory::Callback {
	DebugIoCallback() :
		registers({})
	{
		registers.ready = 1;
	}

	struct Registers {
		uint8_t buffer[0x1000];
		uint32_t ready;
		uint8_t pad[0x1000 - 4];
	};

	virtual void Read(void* buffer, uint32_t address, uint32_t size) {
		abort();
	}
	virtual void Write(const void* buffer, uint32_t address, uint32_t size) {
		uint32_t offset = address % sizeof(Registers);
		
		memcpy(&registers, (const uint8_t*)buffer + offset, size);
	}

	void Step() {
		if (!registers.ready) {
			LOG_INFO(HLE, "Serial output: %s", (const char*)registers.buffer);
			memset(registers.buffer, 0, sizeof(registers.buffer));
			registers.ready = 1;
		}
	}
	Registers registers;
};

struct SystemParameters {
	int argc;
	char** argv;
};

struct TestSystem {
public:
	TestSystem(const SystemParameters& params) {
		arm = new Arm::UnicornInterface(false);

		Memory::RegisterArm(arm);
		HLE::g_kernel.Init(arm);

		//stub = new Arm::GdbStub(arm);
		
		std::vector<uint8_t> executable;
		LoadFile(params.argv[2], executable);

		ElfInfo elfInfo(executable.data(), executable.size());

		LoadArmElf(elfInfo, program);

		ioCallback = new DebugIoCallback;

		// Map the "IO Hardware"
		Memory::MapHostMemory(
			0xF0100000,
			sizeof(DebugIoCallback::Registers),
			&ioCallback->registers,
			Memory::Protection::ReadWrite
		);

		program.stackBase = 0x1000;

		HLE::g_kernel.LoadProgram(program);

		//stub->Init(4336);
	}

	~TestSystem () {
		delete stub;
		delete arm;
	}
	int RunLoop() {
		while (1) {
			Arm::ThreadState threadState = {};
			Arm::ThreadState newThreadState = {};

			// Execute an arbitrarily large number of instructions. No
			// guarantee that all will execute
			if (!arm->Execute(1)) {
				LOG_ERROR(Arm, "Failed to execute ARM code");
			}
			arm->HaltExecution();
			DumpCpu(std::cout, arm);
			ioCallback->Step();
			
			// Save the thread state. If currently at an HLE address, execute the HLE function
			arm->SaveState(threadState);
			if (HLE::g_kernel.HandleFunctionCall(threadState, newThreadState))
				arm->LoadState(newThreadState);
			//stub->Run();
		}
		return 0;
	}

	Arm::Interface* arm;
	Arm::GdbStub* stub;
	HLE::Program program;
	DebugIoCallback* ioCallback;
};

struct EmulatorSystem {
public:
	EmulatorSystem(const SystemParameters& params) {

	}

	~EmulatorSystem() {

	}

	int RunLoop() {
		return 0;
	}

};

static void ParseArguments(int argc, char** argv) {
	for (size_t i = 1; i < argc; i++) {
		
	}
}

int main(int argc, char** argv) {
	SystemParameters params;

	params.argc = argc;
	params.argv = argv;

	TestSystem system(params);

	system.RunLoop();
	
	return 0;
}