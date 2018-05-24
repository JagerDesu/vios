
#include "Common/Types.hpp"
#include "Common/Log.hpp"

#include "Core/HLE/Kernel.hpp"

#include <iostream>

namespace HLE {

Kernel g_kernel;
	
struct FunctionMemoryCallback : public Memory::Callback {
	void Read(void* buffer, uint32_t address, uint32_t size) {
		Arm::ThreadState oldThreadState {};
		Arm::ThreadState newThreadState = {};
		auto arm = g_kernel.GetArm();

		arm->SaveState(oldThreadState);
		if (g_kernel.HandleFunctionCall(oldThreadState, newThreadState))
			LOG_INFO(HLE, "HLE function called.");
	}

	void Write(const void* buffer, uint32_t address, uint32_t size) {
		
	}
};

Kernel::Kernel() :
	arm(nullptr)
{

}

Kernel::~Kernel() {

}

bool Kernel::Init(Arm::Interface* arm) {
	this->arm = arm;

	return true;
}

void Kernel::LoadProgram(Program& program) {
	Arm::ThreadState threadState = {};
	arm->SaveState(threadState);
 
	threadState.r[0]  = 0x00000000;
	threadState.r[1]  = 0x00000000;
	threadState.r[2]  = 0x00000000;
	threadState.r[3]  = 0x00000000;
	threadState.r[4]  = 0x00000000;
	threadState.r[5]  = 0x00000000;
	threadState.r[6]  = 0x00000000;
	threadState.r[7]  = 0x00000000;
	threadState.r[8]  = 0x00000000;
	threadState.r[9]  = 0x00000000;
	threadState.r[10] = 0x00000000;
	threadState.r[11] = 0x00000000;
	threadState.r[12] = 0x00000000;
	threadState.sp = program.stackBase;
	threadState.lr = 0xDEADBEEF;
	threadState.pc = program.entry;

	arm->LoadState(threadState);

	if (program.isVita)
		ResolveNids(program);
}


void Kernel::RegisterModule(const Module& module) {
	hleModules.push_back(&module);
}

void Kernel::ResolveNids(Program& program) {
	static uint8_t patchedFunctionData[8 * 1024];
	const auto* mod_info = &program.mod_info;
	const uint32_t importLength = mod_info->import_end - mod_info->import_top;
	std::vector<uint8_t> importBuffer(importLength);

	Memory::Read(&importBuffer[0], mod_info->import_top + program.imageBase, importLength);

	void* importBegin = &importBuffer[0];
	void* importEnd = (void*)((uintptr_t)importBegin + importLength);
	sce_module_imports_raw* import = (sce_module_imports_raw*)importBegin;

	size_t numImports = 0;
	const uint32_t functionAddressBase = 0xE0000000;
	uint32_t functionAddress = functionAddressBase;
	Memory::MapHostMemory(functionAddressBase, sizeof(patchedFunctionData), &patchedFunctionData[0], Memory::Protection::Read);
	while (import < importEnd) {
		char moduleName[256] = {0};
		std::vector<uint32_t> functionNIDs(import->num_syms_funcs);

		if (import->module_name)
			Memory::Read(moduleName, import->module_name, 256); // Very very bad!!!
		moduleName[255] = 0;

		if (import->func_nid_table) {
			for (size_t i = 0; i < functionNIDs.size(); i++) {
				Memory::Read32(functionNIDs[i], import->func_nid_table + (i * sizeof(uint32_t)));
			}
		}
		
		for (auto& m : hleModules) {
			for (auto& l : m->libraries) {
				if (import->module_nid != l.first)
					continue;
				LOG_INFO(HLE, "Found library match \"%s\"", l.second->name);
				for (size_t i = 0; i < functionNIDs.size(); i++) {
					auto& nid = functionNIDs[i];
					auto functionEntry = l.second->GetFunction(nid);
					uint32_t patch[] = {
						0xE59ff000,
						0x00000000,
						functionAddress
					};
					if (!functionEntry)
						continue;
					LOG_INFO(HLE, "Found function match \"%s\"", functionEntry->name);
					std::pair<uint32_t, CallbackFunctionType> entry(functionAddress, functionEntry->function);
					hleFunctionTable.push_back(entry);
					uint32_t entryAddress;
					
					Memory::Read32(entryAddress, import->func_entry_table + (sizeof(uint32_t) * i));
					Memory::Write(patch, entryAddress, sizeof(patch));
					functionAddress += 12;
				}
			}
		}

		import = (sce_module_imports_raw*)(((uintptr_t)import) + import->size);
		numImports++;
	}
}

bool Kernel::HandleFunctionCall(const Arm::ThreadState& threadState, Arm::ThreadState& newThreadState) {
	for (auto& p : hleFunctionTable) {
		if (threadState.r[15] == p.first) {
			p.second(arm);
			memcpy(&newThreadState, &threadState, sizeof(Arm::ThreadState));
			newThreadState.r[15] = newThreadState.r[14];
			return true;
		}
	}
	return false;
}

Arm::Interface* Kernel::GetArm() const {
	return arm;
}

Program::Program() :
	stackBase(0),
	imageBase(0),
	entry(0),
	isVita(false),
	mod_info({})
{

}

Program::~Program() {

}

}