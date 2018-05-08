#pragma once

#include <vector>

namespace Arm {
	struct State;

	struct InterpreterDispatcher : public Dispatcher {
		void Dispatch(State& state, const void* stream, size_t size);
	};
}