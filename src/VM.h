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

	class VM
	{
	private:
		Chunk* chunk;
		uint8_t* ip;
		std::array<uint64_t, 256> R;
		std::vector<uint64_t> stack;
		Memory mem;
	public:
		VM();
		~VM();

		InterpretResult interpret(std::string source);

		InterpretResult interpret(Chunk* chunk);

		InterpretResult run();

		void freeAllocations();

		void collectGarbage();
	};
}