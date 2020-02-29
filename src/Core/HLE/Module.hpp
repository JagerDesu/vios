#pragma once
#include "Core/Arm/Arm.hpp"
#include "Common/Types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace HLE {
using SceUID = int32_t;
using Nid = uint32_t;
typedef void (*CallbackFunctionType)(Arm::Interface* arm);

struct Function {
	const char* name;
	uint32_t nid;
	CallbackFunctionType function;
};

using FunctionEntry = Function;

struct Variable {
	uint32_t nid;
	uint8_t* data;
	size_t size;
};

void ReturnValue(Arm::Interface* arm, uint32_t value);

uint32_t FetchArgument32W(Arm::Interface* arm, size_t num);


/*
	* Bind_x_x - Wraps a function for consumption by the HLE
	* environment.
	* 
	* Arugment/Return type chart:
	* 
	* - 'u' 32 bit Unsigned integer
	* - 'i' 32 bit Integer
	* - 'p' Pointer
	* - 'f' Float
	* - 'v' Void
*/
/*
void Bind_I_UUIU(Arm::Interface* arm, void* function) {
	using EmulatedFunctionType = int (*)(uint32_t, uint32_t, uint32_t, uint32_t);
	EmulatedFunctionType raw = (EmulatedFunctionType)function;
	uint32_t arg[4];
	for (size_t i = 0; i < 4; i++)
		arg[i] = FetchArgument32W(arm, i);
	ReturnValue(arm, raw(arg[0], arg[1], arg[2], arg[3]));
}*/
#define HLE_BIND_FUNC(func, signature) static_cast<CallbackFunctionType>([](Arm::Interface* arm) {\
	Bind_##signature(arm, &func); \
})

template <int EmulatedFunction(uint32_t, uint32_t, int32_t, uint32_t)>
void Bind_I_UUIU(Arm::Interface* arm) {
	uint32_t arg[4];
	for (size_t i = 0; i < 4; i++)
		arg[i] = FetchArgument32W(arm, i);
	ReturnValue(arm, EmulatedFunction(arg[0], arg[1], arg[2], arg[3]));
}

template <int EmulatedFunction(uint32_t, uint32_t, int32_t, int32_t, uint32_t)>
void Bind_I_UUIIU(Arm::Interface* arm) {
	uint32_t arg[5];
	uint32_t sp = arm->ReadRegister(13);
	for (size_t i = 0; i < 5; i++)
		arg[i] = FetchArgument32W(arm, i);
	ReturnValue(arm, EmulatedFunction(arg[0], arg[1], arg[2], arg[3], arg[4]));
}


template <uint32_t EmulatedFunction(int, int)>
void Bind_U_II(Arm::Interface* arm) {
	int32_t arg[2];
	for (size_t i = 0; i < 2; i++)
		arg[i] = FetchArgument32W(arm, i);
	ReturnValue(arm, EmulatedFunction(arg[0], arg[1]));
}

#define REGISTER_HLE_FUNC(lib, name, nid, sig) \
//{g_##lib##_library.RegisterFunction(nid, FunctionEntry{#name, Bind_##sig<&name>});}

struct NidTable {
	struct Nid {
		uint32_t value;
	};

	struct Entry {
		Nid nid;
		uint32_t address;
	};

	struct Bucket {
		uint32_t value;
		uint32_t next;
	};

	Entry* entries;
	Bucket* bucket;
	size_t size;
	size_t capacity;
};

struct Library {
	inline Library(const char* name) :
		name(name)
	{

	}

	inline const FunctionEntry* GetFunction(Nid nid) const {
		/*auto it = functions.find(nid);
		if (it == functions.end())
			return nullptr;
		return &it->second;

		return nullptr;*/
		return nullptr;
	}

	inline const uint32_t* GetVariable(Nid nid) const {
		/*auto it = variables.find(nid);
		if (it == variables.end())
			return nullptr;
		return &it->second;*/
		return nullptr;
	}

	void RegisterFunction(Nid nid, FunctionEntry entry) {
		//functions[nid] = entry;
	}

	void RegisterVariable(Nid nid, uint32_t address) {
		//variables[nid] = address;
	}
	const char* name;
	std::vector<Function> functions;
	std::vector<Library> libraries;
};


struct Module {
	inline Module(const char* name, uint32_t nid) :
		name(name),
		nid(nid)
	{

	}

	inline const Library* GetLibrary(Nid nid) const {
		/*auto it = libraries.find(nid);
		if (it == libraries.end())
			return nullptr;
		return it->second;*/
		return nullptr;
	}

	void RegisterLibrary(Nid nid, const Library& library) {
		//libraries[nid] = &library;
	}

	const char* name;
	Nid nid;
	std::vector<Library> libraries;
};
}