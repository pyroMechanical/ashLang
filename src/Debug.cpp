#include "Debug.h"

#include <iostream>
#include <iomanip>
#include <bitset>
namespace ash
{
	void Disassembler::disassembleChunk(Chunk* chunk, const char* name)
	{
		std::cout << std::setfill('0') << "==" << name << "==\n";
		this->chunk = chunk;
		for(size_t offset = 0; offset < chunk->size();)
		{
			offset = disassembleInstruction(offset);
		}
	}

	size_t Disassembler::disassembleInstruction(size_t offset)
	{
		std::cout << std::setw(4) << offset << " ";
		/*if (offset > 0 && chunk->GetLine(offset) == chunk->GetLine(offset - 1))
			std::cout << "    | ";
		else
			std::cout << std::setfill(' ') << std::setw(4) << chunk->GetLine(offset);*/

		uint8_t instruction = chunk->at(offset);
		uint8_t byteLength = 0;
		switch (instruction)
		{
		case OP_MOVE: return ABInstruction("OP_MOVE", offset);
		case OP_CONST: return ConstantInstruction("OP_CONST", offset, 4);
		case OP_CONST_LONG: return ConstantInstruction("OP_CONST_LONG", offset, 8);
		case OP_INT_EQUAL: return ABCInstruction("OP_INT_EQUAL", offset);
		case OP_INT_ADD: return ABCInstruction("OP_INT_ADD", offset);
		case OP_INT_SUB: return ABCInstruction("OP_INT_SUB", offset);
		case OP_UNSIGN_MUL: return ABCInstruction("OP_UNSIGN_MUL", offset);
		case OP_UNSIGN_DIV: return ABCInstruction("OP_UNSIGN_DIV", offset);
		case OP_UNSIGN_LESS: return ABCInstruction("OP_UNSIGN_LESS", offset);
		case OP_UNSIGN_GREATER: return ABCInstruction("OP_UNSIGN_GREATER", offset);
		case OP_SIGN_MUL: return ABCInstruction("OP_SIGN_MUL", offset);
		case OP_SIGN_DIV: return ABCInstruction("OP_SIGN_DIV", offset);
		case OP_SIGN_LESS: return ABCInstruction("OP_SIGN_LESS", offset);
		case OP_SIGN_GREATER: return ABCInstruction("OP_SIGN_GREATER", offset);
		case OP_FLOAT_ADD: return ABCInstruction("OP_FLOAT_ADD", offset);
		case OP_FLOAT_SUB: return ABCInstruction("OP_FLOAT_SUB", offset);
		case OP_FLOAT_MUL: return ABCInstruction("OP_FLOAT_MUL", offset);
		case OP_FLOAT_DIV: return ABCInstruction("OP_FLOAT_DIV", offset);
		case OP_FLOAT_LESS: return ABCInstruction("OP_FLOAT_LESS", offset);
		case OP_FLOAT_GREATER: return ABCInstruction("OP_FLOAT_GREATER", offset);
		case OP_FLOAT_EQUAL: return ABCInstruction("OP_FLOAT_EQUAL", offset);
		case OP_DOUBLE_ADD: return ABCInstruction("OP_DOUBLE_ADD", offset);
		case OP_DOUBLE_SUB: return ABCInstruction("OP_DOUBLE_SUB", offset);
		case OP_DOUBLE_MUL: return ABCInstruction("OP_DOUBLE_MUL", offset);
		case OP_DOUBLE_DIV: return ABCInstruction("OP_DOUBLE_DIV", offset);
		case OP_DOUBLE_LESS: return ABCInstruction("OP_DOUBLE_LESS", offset);
		case OP_DOUBLE_GREATER: return ABCInstruction("OP_DOUBLE_GREATER", offset);
		case OP_DOUBLE_EQUAL: return ABCInstruction("OP_DOUBLE_EQUAL", offset);
		case OP_INT_TO_FLOAT: return ABInstruction("OP_INT_TO_FLOAT", offset);
		case OP_INT_TO_DOUBLE: return ABInstruction("OP_INT_TO_DOUBLE", offset);
		case OP_FLOAT_TO_INT: return ABInstruction("OP_FLOAT_TO_INT", offset);
		case OP_FLOAT_TO_DOUBLE: return ABInstruction("OP_FLOAT_TO_DOUBLE", offset);
		case OP_DOUBLE_TO_FLOAT: return ABInstruction("OP_DOUBLE_TO_FLOAT", offset);
		case OP_DOUBLE_TO_INT: return ABInstruction("OP_DOUBE_TO_INT", offset);

		case OP_RETURN: return SimpleInstruction("OP_RETURN", offset);
		}
	}

	size_t Disassembler::SimpleInstruction(const char* name, size_t offset)
	{
		std::cout << name << std::endl;
		return offset + 1;
	}

	size_t Disassembler::ABInstruction(const char* name, size_t offset)
	{
		uint8_t A = chunk->at(offset + 1);
		uint8_t B = chunk->at(offset + 2);

		std::cout << std::setfill('0') << name  << " " << std::setw(3) << +A << " " << std::setw(3) << +B << std::endl;

		return offset + 3;
	}

	size_t Disassembler::ABCInstruction(const char* name, size_t offset)
	{
		uint8_t A = chunk->at(offset + 1);
		uint8_t B = chunk->at(offset + 2);
		uint8_t C = chunk->at(offset + 3);

		std::cout << std::setfill('0') << name << " "<< std::setw(3) << +A << " " << std::setw(3) << +B << " " << std::setw(3) << +C << std::endl;

		return offset + 4;
	}

	size_t Disassembler::ConstantInstruction(const char* name, size_t offset, uint8_t bytes)
	{
		uint64_t constant = 0;
		uint8_t A = chunk->at(offset + 1);
		for(uint8_t i = 1; i <= bytes; i++)
		{
			constant = constant << 8;
			constant += chunk->at(offset + 1 + i);
		}

		std::cout << std::setfill('0') << name << " " << std::setw(3) << +A << " " << std::bitset<64>(constant) << std::endl;

		return offset + 2 + bytes;
	}
}