#include "Core/Loader/ElfLoader.hpp"
#include "Core/Loader/relocate.h"
#include "Core/Loader/sce-elf.h"
#include "Core/Memory.hpp"
#include "Common/Log.hpp"
#include <vector>
#include <cstring>
#include <iostream>
#include <iomanip>

#include <unicorn/unicorn.h>

const uint32_t BaseAddress  = 0x08804000;
const uint32_t StackAddress = 0xC0000000;

struct Relocation {
	uint32_t offset;
	uint32_t size;
};

/********************************************//**
 *  \brief Finds SCE module info
 *  
 *  This finds the module_info_t for this SCE 
 *  ELF.
 *  \returns -1 on error, index of segment 
 *      containing module info on success.
 ***********************************************/
int 
uvl_elf_get_module_info (Elf32_Ehdr *elf_hdr, ///< ELF header
                 Elf32_Phdr *elf_phdrs,       ///< ELF program headers
                sce_module_info_raw *mod_info)       ///< Where to read information to
{
    uint32_t offset;
    uint32_t index;

    index = ((uint32_t)elf_hdr->e_entry & 0xC0000000) >> 30;
    offset = (uint32_t)elf_hdr->e_entry & 0x3FFFFFFF;

    if (elf_phdrs[index].p_vaddr == 0) {
        LOG_ERROR(Loader, "Invalid segment index %d\n", index);
        return -1;
    }

    //*mod_info = (sce_module_info_raw *)(Memory::GetPointer(elf_phdrs[index].p_vaddr + offset));
	Memory::Read(mod_info, elf_phdrs[index].p_vaddr + offset, sizeof(sce_module_info_raw));
    return index;
}

static void InitializeProgramMemory(uint32_t imageBase, uint32_t imageSize, uint32_t stackBase, uint32_t stackSize, HLE::Program& program) {
	program.image.resize(imageSize);
	program.stack.resize(stackSize);
	program.imageBase = imageBase;
	program.stackBase = stackBase;
	
	Memory::MapHostMemory(stackBase, stackSize, &program.stack[0], Memory::Protection::ReadWrite);
}

static void HandleRelocations(ElfInfo& elfInfo, const std::vector<Relocation>& relocations) {
	for(auto& r : relocations) {
		uvl_relocate(
			(uint8_t*)elfInfo.GetBase() + r.offset,
			r.size,
			elfInfo.GetSegments()
			);
	}
}

static void MapProgramSegment(ElfInfo& elfInfo, HLE::Program& program) {
	const char* name = nullptr;
	HLE::Segment* segment = nullptr;
	Memory::Protection protection;
	const uint32_t flags = p.p_flags & (PF_R | PF_W | PF_X);
	
	if (flags == (PF_R | PF_X)) {
		segment = &program.text;
		name = ".text";
		protection = Memory::Protection::ReadExecute;
	}
	else if (flags == (PF_R)) {
		segment = &program.rodata;
		name = ".rodata";
		protection = Memory::Protection::Read;
	}
	else if (flags == (PF_R | PF_W)) {
		segment = &program.data;
		name = ".data";
		protection = Memory::Protection::ReadWrite;
	}
	else {
		LOG_ERROR(
			Loader,
			"Unexpected ELF PT_LOAD segment id %u with flags %X",
			(unsigned int)i,
			p.p_flags
		);
		continue;
	}

	const uint32_t segmentAddress = BaseAddress + p.p_vaddr;
	const uint32_t alignedSize = (p.p_memsz + 0xFFF) & ~0xFFF;

	segment->offset = currentImageOffset;
	segment->address = p.p_vaddr = segmentAddress;
	segment->size = alignedSize;

	memcpy(&program.image[currentImageOffset], elfInfo.GetSegmentPointer(i), p.p_filesz);

	Memory::MapHostMemory(
		segment->address,
		segment->size,
		&program.image[currentImageOffset],
		protection
	);
	currentImageOffset += alignedSize;
}

static void ReadVitaImports(ElfInfo& elfInfo, const sce_module_info_raw* mod_inf, Elf32_Segment* segment) {
    int idx;
    LOG_TRACE(Loader, "Getting module info.");
    if ((idx = uvl_elf_get_module_info(elfInfo.GetHeader(), elfInfo.GetSegments(), &mod_info)) < 0) {
        LOG_TRACE(Loader,"Cannot find module info section.");
        return;
    }
    LOG_TRACE(Loader, "Module name: %s, export table offset: 0x%08X, import table offset: 0x%08X", mod_info.modname, mod_info.ent_top, mod_info.stub_top);

	uint32_t importLength = mod_info.import_end - mod_info.import_top;
	std::vector<uint8_t> importBuffer(importLength);

	void* importBegin = &importBuffer[0];
	void* importEnd = (void*)((uintptr_t)importBegin + importLength);
	uint32_t importAddress = BaseAddress + elfInfo.GetSegment(idx).p_offset + (uintptr_t)mod_info.import_top;
	Memory::Read(importBegin, importAddress, importLength);
	
    sce_module_imports_raw* import = (sce_module_imports_raw*)importBegin;
    void* end = importEnd;

	size_t numImports = 0;
	while (import < end) {
		char moduleName[256] = {0};
		std::vector<uint32_t> functionNIDs(import->num_syms_funcs);

		Memory::Read(moduleName, import->module_name, 256); // Very very bad!!!
		moduleName[255] = 0;

		for (size_t i = 0; i < functionNIDs.size(); i++) {
			Memory::Read32(functionNIDs[i], import->func_nid_table + (i * sizeof(uint32_t)));
		}

		std::cout << "Import[" << numImports << "]: " << moduleName << std::endl;
		std::cout << "  Size: " << import->size << std::endl;
		for (size_t i = 0; i < functionNIDs.size(); i++) {
			std::cout << "  functionNIDs[" << i << "]: 0x" << std::hex << std::setfill('0')
			          << std::setw(8)<< functionNIDs[i] << std::dec << std::endl;
		}

		import = (sce_module_imports_raw*)(((uintptr_t)import) + import->size);
		numImports++;
	}
}

void LoadElf(ElfInfo& elfInfo, HLE::Program& program) {
	uint32_t currentImageOffset = 0;
	uint32_t totalImageSize = 0;
	std::vector<Relocation> relocations;

	for(size_t i = 0; i < elfInfo.GetHeader()->e_phnum; i++) {
        auto& p = elfInfo.GetSegment(i);
        if (p.p_type == PT_LOAD)
            totalImageSize += (p.p_memsz + 0xFFF) & ~0xFFF; // Do some aligning
    }

	InitializeProgramMemory(BaseAddress, totalImageSize, StackAddress, 0x4000, program);
	
	for(size_t i = 0; i < elfInfo.NumSegments(); i++) {
		auto& p = elfInfo.GetSegment(i);
		switch (p.p_type) {
		case PT_LOAD: {
			MapProgramSegment(elfInfo, program, &elfInfo.GetSegment(i));
			break;
		}

		case PT_SCE_RELA: {
			Relocation r = {
				elfInfo.GetSegment(i).p_offset,
				elfInfo.GetSegment(i).p_filesz
			};

			relocations.push_back(r);
			break;
		}
		default:
			LOG_ERROR(Loader, "Invalid segment type [%i]", p.p_type);
			break;
		}
	}

	for (size_t i = 0; i < elfInfo.NumSections() ; i++) {
		auto s = elfInfo.GetSection(i);
		s.sh_addr += BaseAddress;
	}

	// Load the string table first
	auto stringTable = elfInfo.GetSection(elfInfo.GetHeader()->e_shstrndx);
	for (size_t i = 0; i < elfInfo.NumSections() ; i++) {
		if(i == elfInfo.GetHeader()->e_shstrndx)
			continue;
		const size_t nameOffset = stringTable.sh_offset + elfInfo.GetSection(i).sh_name;
		auto name = (const char*)elfInfo.GetBase() + nameOffset;
		std::cout << "Section:" << name << std::endl;
	}

	HandleRelocations(elfInfo, relocations);

	sce_module_info_raw mod_info = {};
	ReadVitaImports(elfInfo, mod_info);

	program.entry = (elfInfo.GetSegment(idx).p_vaddr + mod_info.module_start);
	memcpy(&program.mod_info, &mod_info, sizeof(sce_module_info_raw));
}

