#pragma once

#include "Object.h"
#include "Value.h"
#include "Chunk.h"

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
		uint32_t* ip;
		std::vector<Value> registers;
		std::list<Object> objects;
		std::unordered_map<std::string, Value> globals;
		std::unordered_map<std::string, StringObject*> strings;

	public:
		VM();
		~VM();

		InterpretResult interpret(std::string source);

		InterpretResult run();
	};
}