#pragma once

#include <vector>

#include "Core/Loader/ElfInfo.hpp"
#include "Core/HLE/Kernel.hpp"
#include "Core/Memory.hpp"

struct ILoader {
virtual bool Load(const uint8_t* buffer, size_t size, HLE::Program& program) = 0;
};

struct ElfLoader : public ILoader {
	bool Load(const uint8_t* buffer, size_t size, HLE::Program& program);
};

void LoadElf(ElfInfo& elfInfo, HLE::Program& program);