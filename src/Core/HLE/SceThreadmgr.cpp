#include "SceThreadmgr.hpp"
#include "Common/Log.hpp"

namespace HLE {
extern Module g_SceSysmem_module;
Library g_SceThreadmgr_library("SceThreadmgr");

struct SceKernelLwMutexWork {
	uint64_t data[4];
};

struct SceKernelLwMutexOptParam {
	uint32_t size;
};

/**
 * * void* sceKernelGetThreadTLSAddr(SceUID thid, int key)
 */

static uint32_t sceKernelGetThreadTLSAddr(SceUID thid, int key) {
	return 0;
}

void RegisterSceThreadmgr() {
	REGISTER_HLE_FUNC(SceThreadmgr, sceKernelGetThreadTLSAddr, 0xBACA6891, U_II);
	g_SceSysmem_module.RegisterLibrary(0x859A24B1, g_SceThreadmgr_library);
}

}