#pragma once
#include "Value.h"

#include <vector>
#include <stdint.h>
namespace ash
{
	class Chunk
	{
	private:
		std::vector<uint32_t> opcode;
		std::vector<std::pair<int, int>> lines;
		std::vector<Value> constants;
	public:
		Chunk() = default;
		~Chunk() = default;

		void WriteABC(uint8_t op, uint8_t A, uint16_t B, uint16_t C, int line);
		void WriteABx(uint8_t op, uint8_t A, uint32_t Bx, int line);
		uint32_t AddConstant(float value);

		int GetLine(size_t offset);

	};

	enum class OpCodes : uint8_t
	{
		OP_MOVE,
		OP_LOADK, //load from constants
		OP_LOADG, //load from globals
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV,
		OP_POW,
		OP_NEGATE,
		OP_NOT,
		OP_JUMP,
		OP_EQUAL,
		OP_LESS,
		OP_GREATER,
		OP_LESS_EQUAL,
		OP_GREATER_EQUAL,
		OP_CALL,
		OP_RETURN,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_,
		//OP_
	};
}