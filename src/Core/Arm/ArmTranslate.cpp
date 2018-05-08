#include "Common/Log.hpp"
#include "Common/Bits.hpp"
#include "Core/Arm/ArmTranslate.hpp"
#include "Core/Arm/Arm.hpp"
#include "Core/Memory.hpp"
#include <cstring>

namespace Arm {
static uint32_t ThumbExpandImm_C(uint32_t imm12, bool carryIn, bool& carryOut) {
	uint32_t imm32;
	uint32_t imm8;
	if (BITS_SHIFT(imm12, 11, 10) == 0b00) {
		switch (BITS_SHIFT(imm12, 9, 8)) {
			case 0b00:
				imm32 = SIGN_EXTEND(BITS_SHIFT(imm12, 7, 0), 8);
				break;
			case 0b01:
				imm8 = SIGN_EXTEND(BITS_SHIFT(imm12, 7, 0), 8);
				imm32 = imm8 | (imm8 << 24);
				LOG_INFO(Arm, "Untested path.");
				break;
			case 0b10:
				LOG_CRITICAL(Arm, "Unhandled case.");
				break;
			case 0b11:
				LOG_CRITICAL(Arm, "Unhandled case.");
				break;
			default:
				LOG_CRITICAL(Arm, "Unhandled case.");
				break;
			carryOut = carryIn;
		}
	}

	else {
		uint32_t rotatedValue;

	}
	return imm32;
}

static uint32_t DecodeImmShift(uint32_t opcode, uint32_t imm5, ShiftRegisterType& type) {
	switch (opcode) {
	case 0:
		type = ShiftRegisterType::LSL;
		return imm5;
		break;
	case 1:
		type = ShiftRegisterType::LSR;
		return (imm5 == 0 ? 32 : imm5);
		break;
	case 2:
		type = ShiftRegisterType::ASR;
		return (imm5 == 0 ? 32 : imm5);
		break;
	case 3:
		if (imm5 == 0) {
			type = ShiftRegisterType::RRX;
			return 1;
		}
		else
		{
			type = ShiftRegisterType::ROR;
			return imm5;
		}
		break;
	default:
		return UINT32_MAX;
		break;
	}
	return UINT32_MAX;
}

struct DecodeElement {
	const char* name;
	uint32_t size;
	TranslationHandler handler;
	uint32_t mask;
	uint32_t maskResult;
};

#define ARM_TRANSLATE(x) TranslationHandler_##x
#define ARM_TRANSLATE_FUN(x) \
InstructionBase* ARM_TRANSLATE(x) ( \
	State& state, \
	void* allocData, \
	TranslationAllocator allocate, \
	uint32_t instruction \
)

ARM_TRANSLATE_FUN(b_t1);
ARM_TRANSLATE_FUN(bx_t1);
ARM_TRANSLATE_FUN(bl_t1);
ARM_TRANSLATE_FUN(blx_t1);

ARM_TRANSLATE_FUN(push_t1);
ARM_TRANSLATE_FUN(stmdb_t1);

ARM_TRANSLATE_FUN(mov_r_t1);
ARM_TRANSLATE_FUN(mov_r_t2);

ARM_TRANSLATE_FUN(movs_imm_t1);
ARM_TRANSLATE_FUN(movw_imm_t2);
ARM_TRANSLATE_FUN(movw_imm_t3);
ARM_TRANSLATE_FUN(movt_imm_t1);

ARM_TRANSLATE_FUN(str_imm_t2);

ARM_TRANSLATE_FUN(cmp_r_t2);
ARM_TRANSLATE_FUN(cmp_imm_t1);

ARM_TRANSLATE_FUN(lsl_imm_t1);

ARM_TRANSLATE_FUN(addw_r_t3);
ARM_TRANSLATE_FUN(add_sp_r_t1);

ARM_TRANSLATE_FUN(addw_imm_t3);
ARM_TRANSLATE_FUN(add_sp_imm_t1);

ARM_TRANSLATE_FUN(sub_sp_imm_t1);


const DecodeElement ThumbDecodeTable[] = {
	{ "b [t1]", 2, ARM_TRANSLATE(b_t1),   0b1111000000000000, 0b1101000000000000 },
	{ "bx [t1]", 2, ARM_TRANSLATE(bx_t1), 0b1111111110000111, 0b0100011100000000 },
	{ "bl [t1]", 4, ARM_TRANSLATE(bl_t1), 0b11010000000000001111100000000000, 0b11010000000000001111000000000000 },
	{ "blx [t1]", 4, ARM_TRANSLATE(blx_t1), 0b11010000000000001111100000000000, 0b11000000000000001111000000000000 },
	
	{ "stmdb [t1]", 4, ARM_TRANSLATE(stmdb_t1), 0b00000000000000001111111111010000, 0b00000000000000001110100100000000 },
	{ "push [t1]", 2, ARM_TRANSLATE(push_t1), 0b1111111000000000, 0b1011010000000000 },

	{ "mov (register) [t1]", 2, ARM_TRANSLATE(mov_r_t1), 0b1111111100000000, 0b0100011000000000 }, 
	
	{ "movs (immediate) [t1]", 2, ARM_TRANSLATE(movs_imm_t1), 0b1111100000000000, 0b0010000000000000 },
	{ "movw (immediate) [t2]", 4, ARM_TRANSLATE(movw_imm_t2), 0b10000000000000001111101111101111, 0b00000000000000001111000001001111 },
	{ "movw (immediate) [t3]", 4, ARM_TRANSLATE(movw_imm_t3), 0b10000000000000001111101111110000, 0b00000000000000001111001001000000 },
	
	{ "movt (immediate) [t1]", 4, ARM_TRANSLATE(movt_imm_t1), 0b10000000000000001111101111110000, 0b00000000000000001111001011000000 },


	{ "addw (register) [t3]", 4, ARM_TRANSLATE(addw_r_t3), 0b00000000000000001111111111100000, 0b00000000000000001110101100000000 },
	{ "add sp (register) [t1]", 2, ARM_TRANSLATE(add_sp_r_t1), 0b1111111101111000, 0b0100010001101000 },

	{ "addw (immediate) [t3]", 4, ARM_TRANSLATE(addw_imm_t3), 0b10000000000000001111101111100000, 0b00000000000000001111000100000000 },
	{ "add sp (immediate) [t1]", 2, ARM_TRANSLATE(add_sp_imm_t1), 0b1111100000000000, 0b1010100000000000 },

	{ "sub sp (immediate) [t1]", 2, ARM_TRANSLATE(sub_sp_imm_t1), 0b1111111110000000, 0b1011000010000000 },
	
	{ "str (immediate) [t2]", 2, ARM_TRANSLATE(str_imm_t2), 0b1111100000000000, 0b1001000000000000 },
	
	{ "cmp (register) [t2]", 2, ARM_TRANSLATE(cmp_r_t2), 0b1111111100000000, 0b0100010100000000 },

	{ "cmp (immediate) [t1]", 2, ARM_TRANSLATE(cmp_imm_t1), 0b1111100000000000, 0b0010100000000000 },

	{ "lsl (immediate) [t1]", 2, ARM_TRANSLATE(lsl_imm_t1), 0b1111100000000000, 0b0000000000000000 }
};

enum class DecodeResult {
	Success,
	Invalid,
	Unknown
};

/*
 * Matches the instruction to a DecodeMold and calls its respective handler.
 */
static DecodeResult Decode(
	uint32_t instruction,
	const DecodeElement* elements,
	size_t size,
	const DecodeElement*& match
	) {
	DecodeResult result = DecodeResult::Unknown;

	for(size_t i = 0; i < size; i++) {
		auto& e = elements[i];
		if ((instruction & e.mask) == e.maskResult) {
			match = &e;
			result = DecodeResult::Success;
			break;
		}
	}

	if (result == DecodeResult::Unknown) {
		uint16_t* half = (uint16_t*)&instruction;
		LOG_ERROR(Arm, "Unknown CPU instruction. [[1] 0x%04X [2] 0x%04X]", half[0], half[1]);
	}

	return result;
}

TranslationCache::TranslationCache() :
	top(&cache[0])
{

}

TranslationCache::~TranslationCache() {

}

void* TranslationCache::TranslationAllocatorWrapper(void* userdata, size_t size) {
	return static_cast<TranslationCache*>(userdata)->Allocate(size);
}

void* TranslationCache::Allocate(size_t size) {
	uint8_t* base = top;
	if ((top + size) > &cache[0] + CacheSize)
		LOG_CRITICAL(Arm, "Translation cache is full!");
	top += size;
	return base;
}

const TranslationCache::TranslationBlock* TranslationCache::GetTranslationBlock(const State& state) {
	for (auto& b : blocks) {
		if (b.range.begin <= state.pc && b.range.end > state.pc)
			return &b;
	}
	return nullptr;
}

bool IsBranchInstruction(uOp uop) {
	switch (uop) {
		case uOp::BranchExchange:
		case uOp::BranchImm:
		case uOp::BranchLinkImm:
			return true;
			break;
	}
	return false;
}

const TranslationCache::TranslationBlock* TranslationCache::TranslateBlock(State& state) {
	DecodeResult r;
	const DecodeElement* decodeTable = nullptr;
	size_t tableSize = 0;
	uint32_t fakePc = state.pc; // Used to step through memory

	// Initially check to see if there is translation block at the
	// current PC
	auto b = GetTranslationBlock(state);
	if (b)
		return b;

	// Allocate another block
	TranslationBlock* nb = nullptr;
	blocks.push_back({});
	nb = &blocks[blocks.size() - 1];
	nb->range.begin = state.pc;
	nb->cacheLocation = top;
	nb->thumb = (bool)state.cpsr.t;

	if (state.cpsr.t) {
		decodeTable = ThumbDecodeTable;
		tableSize = sizeof(ThumbDecodeTable) / sizeof(DecodeElement);
	}
	else {
		LOG_ERROR(Arm, "Arm mode currently not yet supported.");
	}

	size_t iterations = 0;

	do {
		const DecodeElement* match = nullptr;
		uint32_t currentInstruction = 0xDEADBEEF;
		uint32_t mask = (uint32_t)~0;
		Memory::Read32(currentInstruction, fakePc);

		if (!currentInstruction)
			LOG_CRITICAL(Arm, "Cannot fetch instruction at PC address: 0x%08X", fakePc);
		r = Decode(currentInstruction, decodeTable, tableSize, match);
		if (r != DecodeResult::Success)
			LOG_CRITICAL(Arm, "Failure decoding instruction at PC address: 0x%08X", fakePc);
		if (match->size == 2)
			mask = 0xFFFF;
		auto base = match->handler(
			state,
			this,
			TranslationAllocatorWrapper,
			currentInstruction & mask
			);
		LOG_INFO(Arm, "Instruction: \"%s\" decoded at PC: 0x%08X", match->name, fakePc);
		assert(match->size == 2 || match->size == 4);
		fakePc += match->size;
		iterations++;
		
		if (IsBranchInstruction(base->opcode))
			break;

	}while ((r == DecodeResult::Success) && (iterations < MaxTranslationBlockSize));

	nb->range.end = fakePc;

	return nb;
}

ARM_TRANSLATE_FUN(b_t1) {
	uint32_t cond = BITS_SHIFT(instruction, 11, 8);
	uint32_t imm8 = BITS_SHIFT(instruction, 7, 0);
	uint32_t imm32 = SIGN_EXTEND(imm8 << 1, 9);
	if (cond == 0b1111) // SVC
		abort();

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct b));
	auto b = (struct b*)&base[1];

	base->opcode = uOp::BranchImm;
	base->cond = (Condition)cond;
	base->flagCond = FlagCondition::InITBlock;
	base->size = sizeof(struct b);
	base->pcOffset = 2;
	
	b->target = imm32;
	b->relative = true;

	return base;
}

ARM_TRANSLATE_FUN(bx_t1) {
	uint32_t Rm = BITS_SHIFT(instruction, 6, 3);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct b));
	auto b = (struct b*)&base[1];

	base->opcode = uOp::BranchExchange;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct b);
	base->pcOffset = 2;

	b->Rm = Rm;
	b->relative = false;
	
	return base;
}

ARM_TRANSLATE_FUN(bl_t1) {
	uint32_t s = BIT_SHIFT(instruction, 10);
	uint32_t i1 = !((BIT_SHIFT(instruction, 29) != 0) ^ s);
	uint32_t i2 = !((BIT_SHIFT(instruction, 27) != 0) ^ s);

	uint32_t imm25 = 0;
	imm25 |= (s << 24);
	imm25 |= (i2 << 23);
	imm25 |= (i1 << 22);
	imm25 |= (BITS_SHIFT(instruction, 9, 0) << 12);
	imm25 |= (BITS_SHIFT(instruction, 26, 16) << 1);

	uint32_t imm32 = SIGN_EXTEND(imm25, 25);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct b));
	auto b = (struct b*)&base[1];

	base->opcode = uOp::BranchLinkImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct b);
	base->pcOffset = 4;

	b->target = imm32;
	b->relative = true;

	return base;
}

ARM_TRANSLATE_FUN(blx_t1) {
	LOG_CRITICAL(Arm, "Unhandled decode.");
	return nullptr;
}

ARM_TRANSLATE_FUN(stmdb_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct lstm));
	auto lstm = (struct lstm*)&base[1];

	uint32_t W = BIT_SHIFT(instruction, 5);
	uint32_t Rn = BITS_SHIFT(instruction, 3, 0);
	uint32_t M = BIT_SHIFT(instruction, 14);

	base->opcode = uOp::Stmdb;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(lstm);
	base->pcOffset = 4;

	lstm->writeBack = (bool)W;
	lstm->Rn = Rn;
	lstm->list = BITS_SHIFT(instruction, 12, 0) | (M << 14);

	return base;
}

ARM_TRANSLATE_FUN(addw_r_t3) {
	const uint32_t S = BIT_SHIFT(instruction, 4);
	const uint32_t Rn = BITS_SHIFT(instruction, 3, 0);
	const uint32_t Rm = BITS_SHIFT(instruction, 19, 16);
	const uint32_t type = BITS_SHIFT(instruction, 21, 20);
	const uint32_t imm2 = BITS_SHIFT(instruction, 23, 22);
	const uint32_t Rd = BITS_SHIFT(instruction, 27, 24);
	const uint32_t imm3 = BITS_SHIFT(instruction, 30, 28);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	base->opcode = uOp::Add;
	base->cond = Condition::AL;
	base->flagCond = S ? FlagCondition::Always : FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 4;

	dp->Rd = Rd;
	dp->Rm = Rm;
	dp->Rn = Rn;
	dp->shiftValue = DecodeImmShift(type, (imm3 << 2) | imm2, dp->shiftType);
	return base;
}

ARM_TRANSLATE_FUN(add_sp_r_t1) {
	const uint32_t Dm = BIT_SHIFT(instruction, 7);
	const uint32_t Rdm = BITS_SHIFT(instruction, 2, 0);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	base->opcode = uOp::Add;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 2;

	dp->Rd = Rdm | (Dm << 3);
	dp->Rm = Rdm | (Dm << 3);
	dp->Rn = 13; // SP
	dp->shiftType = ShiftRegisterType::LSL;
	dp->shiftValue = 0;
	return base;
}


ARM_TRANSLATE_FUN(addw_imm_t3) {
	const uint32_t Rn = BITS_SHIFT(instruction, 3, 0);
	const uint32_t S = BIT_SHIFT(instruction, 4);
	const uint32_t i = BIT_SHIFT(instruction, 10);
	const uint32_t imm8 = BITS_SHIFT(instruction, 23, 16);
	const uint32_t Rd = BITS_SHIFT(instruction, 27, 24);
	const uint32_t imm3 = BITS_SHIFT(instruction, 30, 28);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	base->opcode = uOp::AddImm;
	base->cond = Condition::AL;
	base->flagCond = S ? FlagCondition::Always : FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 4;

	//uint32_t imm32 = 

	LOG_CRITICAL(Arm, "Unfinished decode implementation.");

	if (S && (Rd == 0b1111))
		LOG_INFO(Arm, "Unhandled decode case. Manual states \"SEE CMN (immediate)\"");
	return base;
}

ARM_TRANSLATE_FUN(add_sp_imm_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	base->opcode = uOp::AddImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 2;

	dp->Rd = BITS_SHIFT(instruction, 10, 8);
	dp->Rn = 13; // SP
	dp->imm32 = SIGN_EXTEND(BITS_SHIFT(instruction, 7, 0) << 2, 10);
	dp->shiftType = ShiftRegisterType::LSL;
	dp->shiftValue = 0;

	return base;
}

ARM_TRANSLATE_FUN(sub_sp_imm_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	uint32_t imm7 = BITS_SHIFT(instruction, 7, 0);
	uint32_t imm32 = SIGN_EXTEND(imm7 << 2, 8);

	base->opcode = uOp::SubtractImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 2;

	dp->Rd = 13; // SP
	dp->Rn = 13;
	dp->imm32 = imm32;
	dp->shiftValue = 0;
	dp->shiftType = ShiftRegisterType::LSL;

	return base;
}

ARM_TRANSLATE_FUN(mov_r_t1) {
	uint32_t Rd = BITS_SHIFT(instruction, 2, 0);
	uint32_t D = BIT_SHIFT(instruction, 7);
	uint32_t Rm = BITS_SHIFT(instruction, 6, 3);

	Rd |= (D << 3);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	base->opcode = uOp::Move;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 2;

	dp->Rd = Rd;
	dp->Rm = Rm;
	dp->shiftValue = 0;
	dp->shiftType = ShiftRegisterType::LSL;

	return base;
}

ARM_TRANSLATE_FUN(mov_r_t2) {
	LOG_CRITICAL(Arm, "Unhandled decode.");
	return nullptr;
}

ARM_TRANSLATE_FUN(movs_imm_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	uint32_t Rd = BITS_SHIFT(instruction, 10, 8);
	uint32_t imm8 = BITS_SHIFT(instruction, 7, 0);
	uint32_t imm32 = SIGN_EXTEND(imm8, 8);

	base->opcode = uOp::MoveImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::NotInITBlock;
	base->size = sizeof(struct dp);
	base->pcOffset = 2;

	dp->Rd = Rd;
	dp->imm32 = imm32;
	dp->shiftValue = 0;
	dp->shiftType = ShiftRegisterType::LSL;

	return base;
}
ARM_TRANSLATE_FUN(movw_imm_t2) {
	LOG_CRITICAL(Arm, "Unhandled decode.");
}

ARM_TRANSLATE_FUN(movw_imm_t3) {
	const uint32_t imm3 = BITS_SHIFT(instruction, 30, 28);
	const uint32_t Rd = BITS_SHIFT(instruction, 27, 24);
	const uint32_t imm8 = BITS_SHIFT(instruction, 23, 16);
	const uint32_t i = BIT_SHIFT(instruction, 10);
	const uint32_t imm4 = BITS_SHIFT(instruction, 3, 0);
	const uint32_t imm = (imm8 | (imm3 << 8) | (i << 11) | (imm4 << 12));

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];
	
	base->opcode = uOp::MoveImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 4;

	dp->Rd = Rd;
	dp->imm32 = SIGN_EXTEND(imm, 16);
	dp->shiftValue = 0;
	dp->shiftType = ShiftRegisterType::LSL;

	return base;
}

ARM_TRANSLATE_FUN(movt_imm_t1) {
	const uint32_t imm3 = BITS_SHIFT(instruction, 30, 28);
	const uint32_t Rd = BITS_SHIFT(instruction, 27, 24);
	const uint32_t imm8 = BITS_SHIFT(instruction, 23, 16);
	const uint32_t i = BIT_SHIFT(instruction, 10);
	const uint32_t imm4 = BITS_SHIFT(instruction, 3, 0);
	const uint32_t imm16 = (imm8 | (imm3 << 8) | (i << 11) | (imm4 << 12));

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	base->opcode = uOp::MoveTImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct dp);
	base->pcOffset = 4;

	dp->Rd = Rd;
	dp->imm32 = imm16;
	dp->shiftValue = 0;
	dp->shiftType = ShiftRegisterType::LSL;

	return base;
}

ARM_TRANSLATE_FUN(push_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct lstm));
	auto lstm = (struct lstm*)&base[1];

	uint32_t list = BITS_SHIFT(instruction, 7, 0);
	uint32_t M = BIT_SHIFT(instruction, 8);
	list |= M << 14;

	base->opcode = uOp::Stmdb;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(lstm);
	base->pcOffset = 2;

	lstm->writeBack = true;
	lstm->Rn = 13;
	lstm->list = list;

	return base;
}

ARM_TRANSLATE_FUN(str_imm_t2) {
	uint32_t imm8 = BITS_SHIFT(instruction, 7, 0);
	uint32_t imm32 = SIGN_EXTEND(imm8 << 2, 10);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct stri));
	auto stri = (struct stri*)&base[1];

	base->opcode = uOp::StoreRegisterImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Never;
	base->size = sizeof(struct stri);
	base->pcOffset = 2;

	stri->Rn = 13;
	stri->Rt = BITS_SHIFT(instruction, 10, 8);
	stri->imm32 = imm32;
	stri->index = true;
	stri->add = true;
	stri->writeBack = false;

	return base;
}

ARM_TRANSLATE_FUN(cmp_imm_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct cmpi));
	auto cmpi = (struct cmpi*)&base[1];

	uint32_t imm8 = BITS_SHIFT(instruction, 7, 0);

	base->opcode = uOp::CompareImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Always;
	base->size = sizeof(struct cmpi);
	base->pcOffset = 2;

	cmpi->Rn = BITS_SHIFT(instruction, 10, 8);
	cmpi->imm32 = SIGN_EXTEND(imm8, 8);
	return base;
}

ARM_TRANSLATE_FUN(cmp_r_t2) {
	uint32_t N = BIT_SHIFT(instruction, 7);

	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct cmpr));
	auto cmpr = (struct cmpr*)&base[1];

	base->opcode = uOp::Compare;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::Always;
	base->size = sizeof(struct cmpr);
	base->pcOffset = 2;

	cmpr->Rm = BITS_SHIFT(instruction, 6, 3);
	cmpr->Rn = BITS_SHIFT(instruction, 2, 0) | (N << 3);
	cmpr->shiftValue = 0;
	cmpr->shiftType = ShiftRegisterType::LSL;

	return base;
}

ARM_TRANSLATE_FUN(lsl_imm_t1) {
	auto base = (InstructionBase*)allocate(allocData, sizeof(struct InstructionBase) + sizeof(struct dp));
	auto dp = (struct dp*)&base[1];

	ShiftRegisterType type;
	uint32_t imm5 = BITS_SHIFT(instruction, 10, 6);
	uint32_t Rm = BITS_SHIFT(instruction, 5, 3);
	uint32_t Rd = BITS_SHIFT(instruction, 2, 0);
	base->opcode = uOp::LslImm;
	base->cond = Condition::AL;
	base->flagCond = FlagCondition::NotInITBlock;
	base->size = sizeof(struct dp);
	base->pcOffset = 2;

	dp->Rd = Rd;
	dp->Rm = Rm;
	dp->shiftValue = DecodeImmShift(0b00, imm5, type);
	dp->shiftType = type;

	return base;
}

}
