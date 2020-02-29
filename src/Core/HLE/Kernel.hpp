#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "Core/Arm/Arm.hpp"
#include "Core/Loader/elf.h"
#include "Core/Loader/sce-elf.h"
#include "Module.hpp"

namespace HLE {

enum class Access {
	None = 0b000,
	Read = 0b001,
	Write = 0b010,
	Execute = 0b100,
	ReadWrite = Read | Write,
	ReadWriteExecute = Read | Write | Execute
};

struct Segment {
	uint32_t offset = 0;
	uint32_t address = 0;
	uint32_t size = 0;
};

struct Program {
	Program();
	~Program();

	bool isVita; // CODE SMELL why should this structure need to know this?

	uint32_t stackBase;
	uint32_t imageBase;
	uint32_t entry;
	Segment text, rodata, data;
	sce_module_info_raw mod_info;

	std::vector<uint8_t> stack;
	std::vector<uint8_t> image;
	std::vector<uint8_t> patchedFunctionData;
};

struct PageEntry {
	Access access;
};

class Kernel {
public:
	Kernel();
	~Kernel();
	bool Init(Arm::Interface* arm);
	void LoadProgram(Program& program);
	void RegisterModule(const Module& module);
	bool ResolveImports(Program& program, const void* importBuffer, size_t size);
	bool HandleFunctionCall(const Arm::ThreadState& threadState, Arm::ThreadState& newThreadState);
	Arm::Interface* GetArm() const;
	
private:
	Arm::Interface* arm;
	PageEntry pageTable[0x100000];
	uint8_t* memoryBase;
	std::vector<const Module*> hleModules;

	void ResolveNids(Program& program);
};

extern Kernel g_kernel;

}