#include "SceThreadmgr.hpp"
#include "Common/Log.hpp"

namespace HLE {
Module g_SceLibKernel_module("SceLibKernel", 0xF9C9C52F);
Library g_SceLibKernel_library("SceLibKernel");

struct SceKernelLwMutexWork {
	uint64_t data[4];
};

struct SceKernelLwMutexOptParam {
	uint32_t size;
};

/**
 * * SceUID sceKernelCreateLwMutex(SceKernelLwMutexWork* work, const char* name, int attr, int count, SceKernelLwMutexOptParam* opt)
 */
SceUID sceKernelCreateLwMutex(uint32_t work, uint32_t name, int attr, int count, uint32_t opt) {
	char nameBuffer[256] = {};
	Memory::Read(nameBuffer, name, 256); // Probably should do some bounds checking...
	LOG_INFO(HLE, "sceKernelCreateLwMutex called (\'%s\')", nameBuffer);
	return 0;
}

void RegisterSceLibKernel() {
	REGISTER_HLE_FUNC(SceLibKernel, sceKernelCreateLwMutex, 0xDA6EC8EF, I_UUIIU);
	g_SceLibKernel_module.RegisterLibrary(0xCAE9ACE6, g_SceLibKernel_library);
	g_kernel.RegisterModule(g_SceLibKernel_module);
}

}