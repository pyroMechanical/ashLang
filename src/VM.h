#pragma once

#include "Object.h"
#include "Value.h"
#include "Chunk.h"

#include <array>
#include <list>
#include <unordered_map>

namespace ash
{
	enum InterpretResult
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
		std::vector<uint8_t> stack;

	public:
		VM();
		~VM();

		InterpretResult interpret(std::string source);

		InterpretResult interpret(Chunk* chunk);

		InterpretResult run();
	};
}