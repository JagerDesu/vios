#pragma once

#include "Common/Types.hpp"
#include "Core/Loader/elf.h"

class ElfInfo {
public:
	ElfInfo(void* base, size_t size) :
		base(base),
		size(size),
		header((Elf32_Ehdr*)base),
		segments((Elf32_Phdr*)((uint8_t*)base + header->e_phoff)),
		sections((Elf32_Shdr*)((uint8_t*)base + header->e_shoff))
	{
	}

	inline size_t GetSize() const { return size; }
	inline void* GetBase() const { return base; }

	inline Elf32_Ehdr* GetHeader() const { return header; }

	inline uint8_t* GetSegmentPointer(size_t i) const { return (uint8_t*)GetBase() + GetSegment(i).p_offset; }
	inline Elf32_Phdr& GetSegment(size_t i) { return segments[i]; }
	inline Elf32_Shdr& GetSection(size_t i) { return sections[i]; }

	inline const Elf32_Phdr& GetSegment(size_t i) const { return segments[i]; }
	inline const Elf32_Shdr& GetSection(size_t i) const { return sections[i]; }

	inline Elf32_Phdr* GetSegments() const { return segments; }
	inline Elf32_Shdr* GetSections() const { return sections; }

	inline size_t NumSegments() const { return header->e_phnum; }
	inline size_t NumSections() const { return header->e_shnum; }

private:
	size_t size;
	void* base;
	Elf32_Ehdr* header;
	Elf32_Phdr* segments;
	Elf32_Shdr* sections;
};