#include "Chunk.h"

#include <cassert>

namespace ash
{
	void Chunk::WriteA(uint8_t op, uint8_t A, int line)
	{

		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));
		uint32_t result = 0;
		result = op;
		result = (result << 8) + A;
		result = (result << 16);
		opcode.push_back(result);
	}



	void Chunk::WriteRelativeJump(uint8_t op, int32_t jump, int line)
	{
#define INT24_MIN  (-8388608)
#define INT24_MAX 8388607
		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));
		uint32_t result = 0;
		if (jump > INT24_MAX) jump = INT24_MAX;
		if (jump < INT24_MIN) jump = INT24_MIN;
		result = op;
		result = (result << 24) + (jump & 0xFFFFFF);
		opcode.push_back(result);
	}

	void Chunk::WriteAB(uint8_t op, uint8_t A, uint8_t B, int line)
	{

		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));
		uint32_t result = 0;
		result = op;
		result = (result << 8) + A;
		result = (result << 8) + B;
		result = (result << 8);
		opcode.push_back(result);
	}

	void Chunk::WriteABC(uint8_t op, uint8_t A, uint8_t B, uint8_t C, int line)
	{
		
		if (lines.size() != 0 && line == lines.back().first)
			lines.back().second++;
		else
			lines.emplace_back(std::pair<int, int>(line, 1));

		uint32_t result = 0;
		result = op;
		result = (result << 8) + A;
		result = (result << 8) + B;
		result = (result << 8) + C;
		opcode.push_back(result);
	}

	void Chunk::WriteI8(uint8_t A, int8_t constant)
	{
		WriteI16(A, (int16_t)constant);
	}
	void Chunk::WriteI16(uint8_t A, int16_t constant)
	{
		uint32_t result = 0;
		result = constant >> 15 ? OP_CONST_LOW_NEGATIVE : OP_CONST_LOW;
		result = (result << 8) + A;
		result = (result << 16) + constant;

		opcode.push_back(result);
	}
	void Chunk::WriteI32(uint8_t A, int32_t constant)
	{
		uint16_t constant_high = static_cast<uint16_t>(constant >> 16);
		uint16_t constant_low = static_cast<uint16_t>(constant);

		uint32_t result_high = OP_CONST_MID_LOW;
		result_high = (result_high << 8) + A;
		result_high = (result_high << 16) + constant_high;

		uint32_t result_low = constant >> 31 ? OP_CONST_LOW_NEGATIVE : OP_CONST_LOW;
		result_low = (result_low << 8) + A;
		result_low = (result_low << 16) + constant_low;

		opcode.push_back(result_low);
		opcode.push_back(result_high);
	}
	void Chunk::WriteFloat(uint8_t A, float constant)
	{
		WriteU32(A, *reinterpret_cast<uint32_t*>(&constant));
	}
	void Chunk::WriteI64(uint8_t A, int64_t constant)
	{
		uint16_t constant_high = static_cast<uint16_t>(constant >> 48);
		uint16_t constant_mid_high = static_cast<uint16_t>(constant >> 32);
		uint16_t constant_mid_low = static_cast<uint16_t>(constant >> 16);
		uint16_t constant_low = static_cast<uint16_t>(constant);

		uint32_t result_high = OP_CONST_HIGH;
		result_high = (result_high << 8) + A;
		result_high = (result_high << 16) + constant_high;

		uint32_t result_mid_high = OP_CONST_MID_HIGH;
		result_mid_high = (result_mid_high << 8) + A;
		result_mid_high = (result_mid_high << 16) + constant_mid_high;

		uint32_t result_mid_low = OP_CONST_MID_LOW;
		result_mid_low = (result_mid_low << 8) + A;
		result_mid_low = (result_mid_low << 16) + constant_mid_low;

		uint32_t result_low = constant >> 63 ? OP_CONST_LOW_NEGATIVE : OP_CONST_LOW;
		result_low = (result_low << 8) + A;
		result_low = (result_low << 16) + constant_low;

		opcode.push_back(result_low);
		opcode.push_back(result_mid_low);
		opcode.push_back(result_mid_high);
		opcode.push_back(result_high);
	}
	void Chunk::WriteDouble(uint8_t A, double constant)
	{
		WriteU64(A, *reinterpret_cast<uint64_t*>(&constant));
	}
	void Chunk::WriteU8(uint8_t A, uint8_t constant)
	{
		WriteU16(A, (uint16_t)constant);
	}
	void Chunk::WriteU16(uint8_t A, uint16_t constant)
	{
		uint32_t result = 0;
		result = OP_CONST_LOW;
		result = (result << 8) + A;
		result = (result << 16) + constant;

		opcode.push_back(result);
	}
	void Chunk::WriteU32(uint8_t A, uint32_t constant)
	{
		uint16_t constant_high = static_cast<uint16_t>(constant >> 16);
		uint16_t constant_low = static_cast<uint16_t>(constant);

		uint32_t result_high = OP_CONST_MID_LOW;
		result_high = (result_high << 8) + A;
		result_high = (result_high << 16) + constant_high;

		uint32_t result_low = OP_CONST_LOW;
		result_low = (result_low << 8) + A;
		result_low = (result_low << 16) + constant_low;

		opcode.push_back(result_low);
		opcode.push_back(result_high);
	}
	void Chunk::WriteU64(uint8_t A, uint64_t constant)
	{
		uint16_t constant_high = static_cast<uint16_t>(constant >> 48);
		uint16_t constant_mid_high = static_cast<uint16_t>(constant >> 32);
		uint16_t constant_mid_low = static_cast<uint16_t>(constant >> 16);
		uint16_t constant_low = static_cast<uint16_t>(constant);

		uint32_t result_high = OP_CONST_HIGH;
		result_high = (result_high << 8) + A;
		result_high = (result_high << 16) + constant_high;

		uint32_t result_mid_high = OP_CONST_MID_HIGH;
		result_mid_high = (result_mid_high << 8) + A;
		result_mid_high = (result_mid_high << 16) + constant_mid_high;

		uint32_t result_mid_low = OP_CONST_MID_LOW;
		result_mid_low = (result_mid_low << 8) + A;
		result_mid_low = (result_mid_low << 16) + constant_mid_low;

		uint32_t result_low = OP_CONST_LOW;
		result_low = (result_low << 8) + A;
		result_low = (result_low << 16) + constant_low;

		opcode.push_back(result_low);
		opcode.push_back(result_mid_low);
		opcode.push_back(result_mid_high);
		opcode.push_back(result_high);
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