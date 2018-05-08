#include "Core/Memory.hpp"
#include "Core/Arm/Arm.hpp"
#include <vector>

namespace Memory {
struct Region {
	uint32_t start;
	uint32_t size;
	union {
		Callback* callback;
		void* backing;
	};
};
Arm::Interface* arm = nullptr;
std::vector<Region> callbackRegions;
std::vector<Region> backedRegions;
void RegisterCallback(Callback* callback, uint32_t address, uint32_t size) {
	callbackRegions.push_back(Region{ address, size, callback });
}

static Region* GetCallbackRegion(uint32_t address) {
	for(auto& r : callbackRegions) {
		if(address >= r.start && address < (r.start + r.size))
			return &r;
	}
	return nullptr;
}

void Read(void* buffer, uint32_t address, uint32_t size) {
	auto r = GetCallbackRegion(address);
	if (r)
		r->callback->Read(buffer, address, size);
	else
		arm->ReadMemory(buffer, address, size);
}

void Write(const void* buffer, uint32_t address, uint32_t size) {
	auto r = GetCallbackRegion(address);
	if (r)
		r->callback->Write(buffer, address, size);
	else
		arm->WriteMemory(buffer, address, size);
}

void MapHostMemory(uint32_t address, uint32_t size, void* buffer, Protection protection) {
	Region region;
	region.start = address;
	region.size = size;
	region.backing = buffer;
	backedRegions.push_back(region);
	arm->MapHostMemory(address, size, buffer, protection);
}

void UnmapHostMemory(uint32_t address, uint32_t size) {
	arm->UnmapHostMemory(address, size);
}

void RegisterArm(Arm::Interface* interface) {
	arm = interface;
}
}