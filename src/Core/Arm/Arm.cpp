#include "Core/Arm/Arm.hpp"
#include "Core/Arm/ArmTranslate.hpp"
#include "Common/Bits.hpp"
#include "Common/Log.hpp"
#include <cstdlib>
#include <cstring>

namespace Arm {

State::State() :
	r({}),
	sp(0),
	lr(0),
	pc(0),
	isetstate(0),
	itblock(false),
	iterations(0)
{
	memset(&cpsr, 0, sizeof(Psr));
}

State::~State() {

}

bool State::CheckCondition(Condition cond) {
	bool r = false;
	switch (cond) {
		case Condition::EQ:
			r = cpsr.z;
			break;
		case Condition::NE:
			r = !cpsr.z;
			break;
		case Condition::CS:
			r = cpsr.c;
			break;
		case Condition::CC:
			r = !cpsr.c;
			break;
		case Condition::MI:
			r = cpsr.n;
			break;
		case Condition::PL:
			r = !cpsr.n;
			break;
		case Condition::VS:
			r = cpsr.v;
			break;
		case Condition::VC:
			r = !cpsr.v;
			break;
		case Condition::HI:
			r = (cpsr.c) && (!cpsr.z);
			break;
		case Condition::LS:
			r = ((!cpsr.c) || cpsr.z);
			break;
		case Condition::GE:
			r = (cpsr.n == cpsr.v);
			break;
		case Condition::LT:
			r = (cpsr.n != cpsr.v);
			break;
		case Condition::GT:
			r = ((!cpsr.z) && ((cpsr.n) == cpsr.v));
			break;
		case Condition::LE:
			r = (cpsr.z || (cpsr.n != cpsr.v));
			break;
		case Condition::AL:
			r = true;
			break;
		default:
			abort();
			break;
	}

	return r;
}

bool State::CheckFlagCondition(FlagCondition cond) {
	if (cond == FlagCondition::Always)
		return true;
	if (cond == FlagCondition::Never)
		return false;
	if (cond == FlagCondition::InITBlock)
		return itblock;
	if (cond == FlagCondition::NotInITBlock)
		return !itblock;
	return false; // Should never reach this
}

void State::ALUWritePC(uint32_t value) {
	cpsr.t = BIT_SHIFT(value, 0);
	pc = value & (~1);
}

void State::BranchTo(uint32_t value) {
	pc = value;
}

}