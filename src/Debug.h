#pragma once

#include "Chunk.h"

namespace ash
{
	class Disassembler
	{
	private:
		Chunk* chunk;
		size_t disassembleInstruction(size_t offset);
		size_t ABInstruction(const char* name, size_t offset);
		size_t ABCInstruction(const char* name, size_t offset);
		size_t ConstantInstruction(const char* name, size_t offset);
		size_t SimpleInstruction(const char* name, size_t offset);
		size_t AInstruction(const char* name, size_t offset);
		size_t JumpInstruction(const char* name, size_t offset);
	public:
		Disassembler() = default;
		~Disassembler() = default;
		void disassembleChunk(Chunk* chunk, const char* name);
	};
}