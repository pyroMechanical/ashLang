#pragma once
#include "Memory.h"
#include "Chunk.h"

#include <array>
#include <list>
#include <unordered_map>
#include <memory>

namespace ash
{
	enum class InterpretResult
	{
		INTERPRET_OK,
		INTERPRET_COMPILE_ERROR,
		INTERPRET_RUNTIME_ERROR
	};

	enum registerFlags : uint8_t
	{
		REGISTER_HOLDS_POINTER = 0x01,
		REGISTER_HOLDS_SIGNED = 0x02,
		REGISTER_HOLDS_FLOAT = 0x04,
		REGISTER_HOLDS_DOUBLE = 0x08,
		REGISTER_HOLDS_ARRAY = 0x10,
		REGISTER_HIGH_BITS = 0xE0
	};

	class VM
	{
	private:
		bool comparisonRegister = false;
		std::array<uint64_t, 256> R;
		std::array<uint8_t, 256> rFlags;
		std::vector<uint64_t> stack;
		std::vector<size_t> stackPointers;
		std::vector<std::unique_ptr<TypeMetadata>> types;
		Allocation* allocationList = nullptr;
		friend class Memory;

		Chunk* chunk;
		uint32_t* ip;
	public:
		VM();
		~VM();

		InterpretResult interpret(std::string source);

		InterpretResult interpret(Chunk* chunk);

		InterpretResult run();

		InterpretResult error(const char* msg);

		void* allocate(uint64_t typeID);

		void* allocateArray(void* pointer, size_t oldCount, size_t newCount, uint8_t span);

		void freeAllocation(Allocation* alloc);

		void freeAllocations();

		void collectGarbage();

		void refIncrement(Allocation* ref);

		void refDecrement(Allocation* ref);

		bool isTruthy(uint8_t _register);
	};
}