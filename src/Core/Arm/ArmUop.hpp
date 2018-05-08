#pragma once

#include <cstdint>

enum class Condition : uint8_t {
	EQ,
	NE,
	CS,
	CC,
	MI,
	PL,
	VS,
	VC,
	HI,
	LS,
	GE,
	LT,
	GT,
	LE,
	AL
};

enum class FlagCondition : uint8_t {
	Never,
	Always,

	// Thumb only
	InITBlock,
	NotInITBlock
};


enum class ShiftRegisterType : uint8_t {
	LSL,
	LSR,
	ASR,
	RRX,
	ROR
};


namespace Arm {
enum class uOp : uint8_t {
	None,
	
	Add,
	AddImm,

	Subtract,
	SubtractImm,
	
	Multiply,
	MultiplyImm,

	Move,
	MoveImm,

	MoveTImm,
	
	BranchImm,
	BranchExchange,
	BranchLinkExchangeImm,
	BranchLinkImm,

	StoreRegisterImm,

	Compare,
	CompareImm,
	
	Stmdb,
	
	Lsl,
	LslImm,

	Count
};

struct InstructionBase {
	uOp opcode;
	Condition cond;
	FlagCondition flagCond;
	uint8_t size : 5;
	uint8_t pcOffset : 3;
};

// Load/Store multiple
struct lstm {
	uint8_t Rn; // Base register
	uint8_t writeBack;
	uint16_t list;
};

struct dp {
	union {
		uint32_t Rm;
		uint32_t imm32;
	};
	uint32_t Rn;
	uint32_t Rd;
	uint32_t shiftValue;
	ShiftRegisterType shiftType;
};

struct b {
	union {
		uint32_t target;
		uint32_t Rm;
	};
	bool relative;
};

struct stri {
	uint32_t imm32;
	uint8_t Rn;
	uint8_t Rt;
	bool add;
	bool writeBack;
	bool index;
};

struct cmpi {
	uint32_t Rn;
	uint32_t imm32;
};

struct cmpr {
	uint32_t Rn;
	uint32_t Rm;
	uint32_t shiftValue;
	ShiftRegisterType shiftType;
};

}