#include "Chunk.h"

#include <cassert>

namespace ash
{
	void Chunk::WriteAB(uint8_t op, uint8_t A, uint8_t B, int line)
	{

		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));

		opcode.push_back(op);
		opcode.push_back(A);
		opcode.push_back(B);
	}

	void Chunk::WriteABC(uint8_t op, uint8_t A, uint8_t B, uint8_t C, int line)
	{
		
		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));

		opcode.push_back(op);
		opcode.push_back(A);
		opcode.push_back(B);
		opcode.push_back(C);
	}

	void Chunk::WriteI8(uint8_t A, int8_t constant)
	{
		WriteI64(A, (int64_t)constant);
	}
	void Chunk::WriteI16(uint8_t A, int16_t constant)
	{
		WriteI64(A, (int64_t)constant);
	}
	void Chunk::WriteI32(uint8_t A, int32_t constant)
	{
		WriteI64(A, (int64_t)constant);
	}
	void Chunk::WriteFloat(uint8_t A, float constant)
	{
		WriteU32(A, *reinterpret_cast<uint32_t*>(&constant));
	}
	void Chunk::WriteI64(uint8_t A, int64_t constant)
	{
		WriteU64(A, static_cast<uint64_t>(constant));
	}
	void Chunk::WriteDouble(uint8_t A, double constant)
	{
		WriteU64(A, *reinterpret_cast<uint64_t*>(&constant));
	}
	void Chunk::WriteU8(uint8_t A, uint8_t constant)
	{
		WriteU32(A, static_cast<uint32_t>(constant));
	}
	void Chunk::WriteU16(uint8_t A, uint16_t constant)
	{
		WriteU32(A, static_cast<uint32_t>(constant));
	}
	void Chunk::WriteU32(uint8_t A, uint32_t constant)
	{
		uint8_t temp = (uint8_t)(constant >> 24);
		opcode.push_back(OP_CONST);
		opcode.push_back(A);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 16);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 8);
		opcode.push_back(temp);
		temp = (uint8_t)constant;
		opcode.push_back(temp);
	}
	void Chunk::WriteU64(uint8_t A, uint64_t constant)
	{
		uint8_t temp = (uint8_t)(constant >> 56);
		opcode.push_back(OP_CONST_LONG);
		opcode.push_back(A);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 48);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 40);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 32);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 24);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 16);
		opcode.push_back(temp);
		temp = (uint8_t)(constant >> 8);
		opcode.push_back(temp);
		temp = (uint8_t)constant;
		opcode.push_back(temp);
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