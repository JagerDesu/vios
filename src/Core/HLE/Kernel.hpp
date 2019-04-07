#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "Core/Arm/Arm.hpp"
#include "Core/Loader/elf.h"
#include "Core/Loader/sce-elf.h"
#include "Module.hpp"

namespace HLE {

struct Segment {
	uint32_t offset = 0;
	uint32_t address = 0;
	uint32_t size = 0;
};

struct Program {
	Program();
	~Program();

	bool isVita;

	uint32_t stackBase;
	uint32_t imageBase;
	uint32_t entry;
	Segment text, rodata, data;
	sce_module_info_raw mod_info;

	std::vector<uint8_t> stack;
	std::vector<uint8_t> image;
	std::vector<uint8_t> patchedFunctionData;
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
	std::vector<const Module*> hleModules;

	void ResolveNids(Program& program);
};

extern Kernel g_kernel;

}