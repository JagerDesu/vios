#pragma once

#include "ArmUop.hpp"
#include <vector>
#include <cstddef>
#include <array>

namespace Arm {

struct State;

struct InstructionBase;

static const size_t MaxTranslationBlockSize = 1;

using TranslationAllocator = void* (*) (void* userdata, size_t size);
using TranslationHandler = InstructionBase* (*) (
	State& state,
	void* allocData,
	TranslationAllocator allocate,
	uint32_t instruction
	);

class TranslationCache {
public:
	struct Range {
		uint32_t begin;
		uint32_t end;
		inline bool operator==(const Range& right) const {
			return ((begin == right.begin) && (end == right.end));
		}
	};

	struct TranslationBlock {
		Range range;
		uint8_t* cacheLocation;
		bool thumb;
	};

	TranslationCache();
	~TranslationCache();

	void* Allocate(size_t size);
	const TranslationBlock* GetTranslationBlock(const State& state);
	const TranslationBlock* TranslateBlock(State& state);

	static void* TranslationAllocatorWrapper(void* userdata, size_t size);

private:
	static const size_t CacheSize = 1024 * 1024 * 64;
	std::array<uint8_t, CacheSize> cache;
	uint8_t* top;
	std::vector<TranslationBlock> blocks;
};

}