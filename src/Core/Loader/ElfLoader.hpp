#pragma once

#include <vector>

#include "Core/Loader/ElfInfo.hpp"
#include "Core/HLE/Kernel.hpp"
#include "Core/Memory.hpp"

/*
 * - Loads a static ARM ELF with no relocations
 **/
bool LoadArmElf(ElfInfo& elfInfo, HLE::Program& program);

/*
 * - Loads a SCE Vita flavored ELF with SCE Relocations
 **/
bool LoadVitaElf(ElfInfo& elfInfo, HLE::Program& program);

bool LoadSelf(std::vector<uint8_t> buffer, HLE::Program& program);