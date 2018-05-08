#include "SceSysmem.hpp"
#include "Common/Log.hpp"

namespace HLE {
Module g_SceSysmem_module("SceSysmem", 0x3380B323);
Library g_SceSysmem_library("SceSysmem");

static void CopyString(char* dst, uint32_t src, uint32_t len) {
	uint8_t c = 1;
	for (size_t i = 0; i < len; i++) {
		Memory::Read8(c, src);
		dst[i] = c;
		if (c == 0)
			break;
	}
}

//SceUID sceKernelAllocMemBlock(const char *name, SceKernelMemBlockType type, int size, SceKernelAllocMemBlockOpt *optp);
static SceUID sceKernelAllocMemBlock(uint32_t name, uint32_t type, int32_t size, uint32_t optp) {
	char nameBuffer[256];
	uint8_t c;
	
	LOG_INFO(HLE, "sceKernelAllocMemBlock called!");
	return 0;
}

void RegisterSceSysmem() {
	REGISTER_HLE_FUNC(SceSysmem, sceKernelAllocMemBlock, 0xB9D5EBDE, I_UUIU);
	g_SceSysmem_module.RegisterLibrary(0x37FE725A, g_SceSysmem_library);
	g_kernel.RegisterModule(g_SceSysmem_module);
}

}