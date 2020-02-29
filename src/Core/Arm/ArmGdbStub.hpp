#pragma once

#include "Arm.hpp"

#include <cstdint>
#include <cstddef>
#include <thread>

#include "SDL2/SDL_thread.h"

#include <vector>

namespace Arm {
struct State;

struct GdbStub {
public:
	enum class Status {
		Uninitialized,
		Running,
		Disabled,
		Halted,

		NeedAck,
	};

	GdbStub(Arm::Interface* arm);
	~GdbStub();
	bool Init(uint16_t port);
	void Shutdown();

	void Run();

	void ConsumePacket(const uint8_t* buffer, size_t size);

	uint8_t ReadByte();
	void WriteByte(uint8_t c);
	
	inline void Write(const void* buffer, size_t size) {
		for (size_t i = 0; i < size; i++)
			WriteByte(((uint8_t*)buffer)[i]);
	}

	inline void Read(const void* buffer, size_t size) {
		for (size_t i = 0; i < size; i++)
			((uint8_t*)buffer)[i] = ReadByte();
	}

	int socketHandle;
	uint16_t port;
	Status status;
	Arm::Interface* arm;
	std::vector<uint8_t> lastPacket;
};

}