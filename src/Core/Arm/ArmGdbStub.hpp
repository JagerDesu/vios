#pragma once

#include <cstdint>

namespace Arm {
struct State;

struct GdbStub {
public:
	enum class Status {
		Uninitialized,
		Running,
		Disabled,
		Halted
	};

	GdbStub();
	~GdbStub();

	inline Status GetStatus() const { return status; }
	inline void SetStatus(Status status) { this->status = status; }

	bool Init(uint16_t port);
	void Shutdown();

private:
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
};

}