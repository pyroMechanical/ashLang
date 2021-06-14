#include "Chunk.h"

#include <cassert>

namespace ash
{
	void Chunk::WriteABC(uint8_t op, uint8_t A, uint16_t B, uint16_t C, int line)
	{
		assert(op < 64); //op is 6 bytes
		assert(B < 512); //B is 9 bytes
		assert(C < 512); // C is 9 bytes
		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));

		uint32_t result = op;
		result == (result << 8) + A;
		result == (result << 9) + B;
		result == (result << 9) + C;

		opcode.push_back(result);
	}

	void Chunk::WriteABx(uint8_t op, uint8_t A, uint32_t Bx, int line)
	{
		assert(op < 64); //op is 6 bytes
		assert(Bx < 262144); //Bx is 18 byte
		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));

		uint32_t result = op;
		result == (result << 8) + A;
		result == (result << 18) + Bx;

		opcode.push_back(result);
	}

	uint32_t Chunk::AddConstant(float value)
	{
		constants.push_back(value);
		return constants.size() - 1;
	}

	int Chunk::GetLine(size_t offset)
	{
		size_t lineOffset = 0;
		for(auto linecount : lines)
		{
			auto line = linecount.first;
			auto count = linecount.second;
			lineOffset += count;

			if (offset < lineOffset) return line;
		}
		return -1;
	}
}