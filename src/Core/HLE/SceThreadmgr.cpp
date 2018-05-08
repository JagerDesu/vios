#include "SceThreadmgr.hpp"
#include "Common/Log.hpp"

namespace HLE {
extern Module g_SceSysmem_module;
Library g_SceThreadmgr_library("SceThreadmgr");

static uint32_t sceKernelGetThreadTLSAddr(SceUID thid, int key) {
	return 0;
}
SceUID sceKernelCreateLwMutex(uint32_t memory, uint32_t name, int attr, int count, uint32_t opt) {
	return -1;
}

void RegisterSceThreadmgr() {
	REGISTER_HLE_FUNC(SceThreadmgr, sceKernelGetThreadTLSAddr, 0xBACA6891, U_II);
	g_SceSysmem_module.RegisterLibrary(0x859A24B1, g_SceThreadmgr_library);
}

}