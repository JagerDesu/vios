#pragma once
#include "Common/Types.hpp"
#include "Common/Swap.hpp"

namespace Arm {
	struct Interface;
}

namespace Memory {
	enum class Protection : uint32_t {
		Read    = 0b001,
		Write   = 0b010,
		Execute = 0b100,

		ReadWrite = (Read | Write),
		ReadWriteExecute = (Read | Write | Execute),
		ReadExecute = (Read | Execute),
		WriteExecute = (Write | Execute)
	};

	struct Callback {
		virtual void Read(void* buffer, uint32_t address, uint32_t size) = 0;
		virtual void Write(const void* buffer, uint32_t address, uint32_t size) = 0;

		// Probably should properly handle big endian, but who cares right now
		inline void Read32(Common::uint32_le_t& buffer, uint32_t address) {
			Read(&buffer, address, sizeof(buffer));
		}

		inline void Read16(Common::uint16_le_t& buffer, uint32_t address) {
			Read(&buffer, address, sizeof(buffer));
		}

		inline void Read8(Common::uint8_le_t& buffer, uint32_t address) {
			Read(&buffer, address, sizeof(buffer));
		}

		inline void Write32(Common::uint32_le_t buffer, uint32_t address) {
			Write(&buffer, address, sizeof(buffer));
		}

		inline void Write16(Common::uint16_le_t buffer, uint32_t address) {
			Write(&buffer, address, sizeof(buffer));
		}
		inline void Write8(Common::uint8_le_t buffer, uint32_t address) {
			Write(&buffer, address, sizeof(buffer));
		}
	};

	void RegisterCallback(Callback* callback, uint32_t address, uint32_t size);

	void MapHostMemory(uint32_t address, uint32_t size, void* buffer, Protection protection);
	void UnmapHostMemory(uint32_t address, uint32_t size);

	void Read(void* buffer, uint32_t address, uint32_t size);
	void Write(const void* buffer, uint32_t address, uint32_t size);
	
	void RegisterArm(Arm::Interface* arm);
	
	// Probably should properly handle big endian, but who cares right now
	inline void Read32(Common::uint32_le_t& buffer, uint32_t address) {
		Read(&buffer, address, sizeof(buffer));
	}

	inline void Read16(Common::uint16_le_t& buffer, uint32_t address) {
		Read(&buffer, address, sizeof(buffer));
	}

	inline void Read8(Common::uint8_le_t& buffer, uint32_t address) {
		Read(&buffer, address, sizeof(buffer));
	}

	inline void Write32(Common::uint32_le_t buffer, uint32_t address) {
		Write(&buffer, address, sizeof(buffer));
	}

	inline void Write16(Common::uint16_le_t buffer, uint32_t address) {
		Write(&buffer, address, sizeof(buffer));
	}
	inline void Write8(Common::uint8_le_t buffer, uint32_t address) {
		Write(&buffer, address, sizeof(buffer));
	}
	inline uint32_t VirtualToPhysicalAddress(uint32_t addr) { return addr; }
}