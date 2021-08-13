#pragma once
#include "Memory.h"
#include "Chunk.h"

#include <array>
#include <list>
#include <unordered_map>

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
	};

	class VM
	{
	private:
		std::array<uint64_t, 256> R;
		std::array<uint8_t, 256> rFlags;
		std::vector<uint64_t> stack;
		std::vector<size_t> stackPointers;
		Allocation* allocationList = nullptr;
		friend class Memory;

		Chunk* chunk;
		uint8_t* ip;
	public:
		VM();
		~VM();

		InterpretResult interpret(std::string source);

		InterpretResult interpret(Chunk* chunk);

		InterpretResult run();

		InterpretResult error(const char* msg);

		void* allocate(void* pointer, size_t oldSize, size_t newSize);

		void freeAllocation(Allocation* alloc);

		void freeAllocations();

		void collectGarbage();
	};
}