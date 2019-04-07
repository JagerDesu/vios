
#include "Common/Types.hpp"
#include "Common/Log.hpp"

#include "Core/HLE/Kernel.hpp"

#include <iostream>

namespace HLE {

Kernel g_kernel;

const uint32_t SystemCallAddress = 0xDE00BEE0;
	
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
	threadState.sp = program.stackBase + program.stack.size();
	threadState.lr = 0xDEADBEEF;
	threadState.pc = program.entry;

	arm->LoadState(threadState);

	if (program.isVita)
		ResolveNids(program);
}


void Kernel::RegisterModule(const Module& module) {
	hleModules.push_back(&module);
}

bool Kernel::ResolveImports(Program& program, const void* importBuffer, size_t size) {
	auto importBegin = (const uint8_t*)importBuffer;
	auto importEnd = importBegin + size;

	sce_module_imports_raw* import = (sce_module_imports_raw*)importBegin;

	size_t numImports = 0;

	// Read the import buffer
	while ((uintptr_t)import < (uintptr_t)importEnd) {
		char moduleName[256] = {0};
		std::vector<uint32_t> functionNIDs(import->num_syms_funcs);

		// Get module name
		if (import->module_name)
			Memory::Read(moduleName, import->module_name, 256); // Very very bad!!!
		moduleName[255] = 0;

		// Read NID table
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
						nid,              // This is not executed by the CPU; this is just so we know which NID we must call
						SystemCallAddress // We need to branch here if we want to call a HLE function
					};

					if (!functionEntry)
						continue;

					uint32_t entryAddress;
					
					Memory::Read32(entryAddress, import->func_entry_table + (sizeof(uint32_t) * i));
					Memory::Write(patch, entryAddress, sizeof(patch));
					LOG_INFO(HLE, "\"%s\" function resolved", functionEntry->name);
				}
			}
		}

		import = (sce_module_imports_raw*)(((uintptr_t)import) + import->size);
		numImports++;
	}

	return true;
}	

void Kernel::ResolveNids(Program& program) {
	const auto* mod_info = &program.mod_info;
	const uint32_t importLength = mod_info->import_end - mod_info->import_top;
	std::vector<uint8_t> importBuffer(importLength);

	// Copy the import data into a host buffer
	Memory::Read(&importBuffer[0], mod_info->import_top + program.imageBase, importLength);

	ResolveImports(program, importBuffer.data(), importBuffer.size());
}

bool Kernel::HandleFunctionCall(const Arm::ThreadState& threadState, Arm::ThreadState& newThreadState) {
	for (auto& p : hleFunctionTable) {
		if (threadState.r[15] == p.first) {
			p.second(arm);
			memcpy(&newThreadState, &threadState, sizeof(Arm::ThreadState));
			newThreadState.r[15] = threadState.r[14];
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