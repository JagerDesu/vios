#include "Core/Arm/Arm.hpp"
#include "Core/Arm/ArmInterpreterDispatcher.hpp"
#include "Common/Log.hpp"
#include "Core/HLE/Module.hpp"
#include "Core/Memory.hpp"
#include <cassert>
#include <cstring>

namespace Arm {

// Add with carry, indicates if a carry-out or signed overflow occurred.
static uint32_t AddWithCarry(uint32_t left, uint32_t right, uint32_t carryIn, bool& carryOut,
                 bool& overflow) {
    uint64_t unsignedSum = (uint64_t)left + (uint64_t)right + (uint64_t)carryIn;
    int64_t signedSum = (int64_t)(int32_t)left + (int64_t)(int32_t)right + (int64_t)carryIn;
    uint64_t result = (unsignedSum & 0xFFFFFFFF);

	carryOut = (result != unsignedSum);
    overflow = ((int64_t)(int32_t)result != signedSum);

    return (uint32_t)result;
}

// Bitwise inversion
static uint32_t Not(uint32_t value) {
	return ~value;
}

static uint32_t Shift_C(uint32_t value, ShiftRegisterType type, uint32_t amount, bool carryIn, bool& carryOut) {
	uint32_t result;

	assert(!(type == ShiftRegisterType::RRX && amount != 1));
	if (amount == 0) {
		carryOut = carryIn;
		return value;
	}
	switch(type) {
	case ShiftRegisterType::LSL:
		result = LSL(value, amount);
        carryOut = (bool)BIT(value, 32 - amount);
		break;
	case ShiftRegisterType::LSR:
		result = LSR(value, amount);
		carryOut = (bool)BIT(value, amount - 1);
		break;
	case ShiftRegisterType::ASR:
		result = ASR(value, amount);
		carryOut = (bool)BIT(value, amount - 1);
		break;
	case ShiftRegisterType::RRX:
		abort();
		break;
	case ShiftRegisterType::ROR:
		result = ROR(value, amount);
		abort();
		break;
	default:
		abort();
		break;
	}
	return result;
}

static void ProcessMove(State& state, uint32_t Rd, uint32_t value, bool setFlags, bool carry) {
	if (setFlags) {
		state.HandleN(value);
		state.HandleZ(value);
		state.cpsr.c = carry;
	}
	if (Rd == 15)
		state.ALUWritePC(value);
	else
		state.r[Rd] = value;
}

static uint32_t ProcessAdd(State& state, uint32_t left, uint32_t right, uint32_t shiftValue, ShiftRegisterType shiftType, bool setFlags) {
	bool carry;
	bool carryOut;
	bool overflow;

	uint32_t shifted = Shift_C(right, shiftType, shiftValue, state.cpsr.c, carry);

	uint32_t result = AddWithCarry(left, right, 0, carryOut, overflow);
	if(setFlags) {
		state.HandleN(result);
		state.HandleZ(result);
		state.cpsr.c = carryOut;
		state.cpsr.v = overflow;
	}
	return result;
}

static void ProcessCompare(State& state, uint32_t left, uint32_t right, uint32_t shiftValue, ShiftRegisterType shiftType) {
	bool carry;
	bool overflow;

	uint32_t shifted = Shift_C(right, shiftType, shiftValue, state.cpsr.c, carry);

	uint32_t result = AddWithCarry(
		left,
		Not(shifted),
		1,
		carry,
		overflow
		);
	state.cpsr.n = state.HandleN(result);
	state.cpsr.z = state.HandleZ(result);
	state.cpsr.c = carry;
	state.cpsr.v = overflow;
}

void InterpreterDispatcher::Dispatch(State& state, const void* stream, size_t size) {
	const uint8_t* cursor = (const uint8_t*)stream;
	for (; cursor < (const uint8_t*)stream + size; cursor += sizeof(InstructionBase)) {
		auto uop = (const InstructionBase*)cursor;
		state.pc += uop->pcOffset;
		if (!state.CheckCondition(uop->cond))
			continue;
		switch (uop->opcode) {
		case uOp::Stmdb: {
			auto lstm = (struct lstm*)&uop[1];
			uint32_t stackBase = state.r[lstm->Rn];
			uint32_t count = 0;

			for (size_t bit = 0; bit < 16; bit++) {
				if (BIT_SHIFT(lstm->list, bit)) {
					stackBase -= 4;
					Memory::Write32(state.r[bit], stackBase);
					count++;
				}
			}
			if (lstm->writeBack)
				state.r[lstm->Rn] -= (count * sizeof(uint32_t));
			break;
		}

		case uOp::AddImm: {
			auto dp = (struct dp*)&uop[1];
			uint32_t result;
			bool carryOut;
			bool overflow;
			
			result = ProcessAdd(state,
				state.r[dp->Rn],
				dp->imm32,
				dp->shiftValue,
				dp->shiftType,
				state.CheckFlagCondition(uop->flagCond)
			);

			if(dp->Rd == 15)
				state.ALUWritePC(result);
			else
				state.r[dp->Rd] = result;
			
			break;
		}

		case uOp::SubtractImm: {
			auto dp = (struct dp*)&uop[1];
			uint32_t result;
			bool carryOut;
			bool overflow;
			uint32_t old = state.r[dp->Rd];

			result = AddWithCarry(state.r[dp->Rn], Not(dp->imm32), 1, carryOut, overflow);
			if(state.CheckFlagCondition(uop->flagCond)) {
				state.HandleN(result);
				state.HandleZ(result);
				state.cpsr.c = carryOut;
				state.cpsr.v = overflow;
			}
			
			if(dp->Rd == 15)
				state.ALUWritePC(result);
			else
				state.r[dp->Rd] = result;
			break;
		}

		case uOp::Move: {
			auto dp = (struct dp*)&uop[1];
			ProcessMove(
				state,
				dp->Rd,
				state.r[dp->Rm],
				state.CheckFlagCondition(uop->flagCond),
				state.cpsr.c
				);
			break;
		}

		case uOp::MoveImm: {
			auto dp = (struct dp*)&uop[1];
			ProcessMove(
				state,
				dp->Rd,
				dp->imm32,
				state.CheckFlagCondition(uop->flagCond),
				state.cpsr.c
				);
			break;
		}

		case uOp::MoveTImm: {
			auto dp = (struct dp*)&uop[1];
			uint32_t old = state.r[dp->Rd];
			state.r[dp->Rd] = (old & 0x0000FFFF) | (dp->imm32 << 16);
			break;
		}

		case uOp::LslImm: {
			auto dp = (struct dp*)&uop[1];
			uint32_t result;
			bool carry;

			result = Shift_C(state.r[dp->Rm], dp->shiftType, dp->shiftValue, state.cpsr.c, carry);
			if (dp->Rd == 15)
				state.ALUWritePC(dp->Rd);
			else
				state.r[dp->Rd] = result;
			if (state.CheckFlagCondition(uop->flagCond)) {
				state.HandleN(result);
				state.HandleZ(result);
				state.cpsr.c = carry;
			}
			break;
		}

		case uOp::StoreRegisterImm: {
			uint32_t offsetAddress;
			uint32_t address;
			auto stri = (struct stri*)&uop[1];
			if (stri->add)
				offsetAddress = state.r[stri->Rn] + stri->imm32;
			else
				offsetAddress = state.r[stri->Rn] - stri->imm32;
			if(stri->index)
				address = offsetAddress;
			else
				address = state.r[stri->Rn];
			Memory::Write32(address, state.r[stri->Rt]);
			if (stri->writeBack)
				state.r[stri->Rn] = address;
			break;
		}

		case uOp::CompareImm: {
			auto cmpi = (struct cmpi*)&uop[1];
			ProcessCompare(
				state,
				state.r[cmpi->Rn],
				cmpi->imm32,
				0,
				ShiftRegisterType::LSL
				);
			break;
		}

		case uOp::Compare: {
			auto cmpr = (struct cmpr*)&uop[1];
			ProcessCompare(
				state,
				state.r[cmpr->Rn],
				state.r[cmpr->Rm],
				cmpr->shiftValue,
				cmpr->shiftType
				);
			break;
		}

		case uOp::Add: {
			auto dp = (struct dp*)&uop[1];
			uint32_t shifted;
			uint32_t result;
			bool carry;
			bool overflow;

			shifted = Shift_C(
				state.r[dp->Rm],
				dp->shiftType,
				dp->shiftValue,
				state.cpsr.c,
				carry
				);

			result = AddWithCarry(
				state.r[dp->Rn],
				shifted,
				0,
				carry,
				overflow
				);

			if (dp->Rd == 15)
				state.ALUWritePC(result);
			else {
				state.r[dp->Rd] = result;
				if (state.CheckFlagCondition(uop->flagCond)) {
					state.HandleN(result);
					state.HandleZ(result);
					state.cpsr.c = carry;
					state.cpsr.v = overflow;
				}
			}
			
			break;
		}

		case uOp::BranchLinkImm: {
			auto b = (struct b*)&uop[1];
			uint32_t target = b->target;
			if (b->relative)
				target += state.pc;
			if (!HLE::CallFunction(state, target)) {
				state.lr = state.MakeAddressJumpable(state.pc - (state.cpsr.t ? 0 : 4));
				state.BranchTo(target);
			}
			break;
		}

		case uOp::BranchImm: {
			auto b = (struct b*)&uop[1];
			uint32_t target = b->target;
			if (b->relative)
				target += state.pc;
			if (!HLE::CallFunction(state, target))
				state.BranchTo(target);
			break;
		}

		case uOp::BranchExchange: {
			auto b = (struct b*)&uop[1];
			uint32_t target = state.r[b->Rm];
			if (b->relative)
				target += state.pc;
			if (!HLE::CallFunction(state, target)) {
				state.lr = state.MakeAddressJumpable(state.pc - (state.cpsr.t ? 0 : 4));
				state.cpsr.t = BIT(target, 0);
				state.BranchTo(target & ~1);
			}
			break;
		}
		
		default:
			LOG_INFO(Arm, "InterpreterDispatcher: Unimplemented instruction.");
			break;
		}
		cursor += uop->size;
	}
}

}