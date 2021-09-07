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

		uint8_t instruction = static_cast<uint8_t>(chunk->at(offset) >> 24);
		switch (instruction)
		{
			case OP_HALT: return SimpleInstruction("OP_HALT", offset);
			case OP_PUSH: return AInstruction("OP_PUSH", offset);
			case OP_POP: return AInstruction("OP_POP", offset);
			case OP_RETURN: return SimpleInstruction("OP_RETURN", offset);
			case OP_OUT: return AInstruction("OP_OUT", offset);
			case OP_STORE_IP_OFFSET: return AInstruction("OP_STORE_IP_OFFSET", offset);
			case OP_MOVE: return ABInstruction("OP_MOVE", offset);
			case OP_ALLOC: return ABInstruction("OP_ALLOC", offset);
			case OP_STORE8:  return ABInstruction("OP_STORE8", offset);
			case OP_STORE16: return ABInstruction("OP_STORE16", offset);
			case OP_STORE32: return ABInstruction("OP_STORE32", offset);
			case OP_STORE64: return ABInstruction("OP_STORE64", offset);
			case OP_LOAD8:  return ABInstruction("OP_LOAD8", offset);
			case OP_LOAD16: return ABInstruction("OP_LOAD16", offset);
			case OP_LOAD32: return ABInstruction("OP_LOAD32", offset);
			case OP_LOAD64: return ABInstruction("OP_LOAD64", offset);
			case OP_CONST_LOW: return ConstantInstruction("OP_CONST_LOW", offset);
			case OP_CONST_MID_LOW:  return ConstantInstruction("OP_CONST_MID_LOW", offset);
			case OP_CONST_MID_HIGH: return ConstantInstruction("OP_CONST_MID_HIGH", offset);
			case OP_CONST_HIGH: return ConstantInstruction("OP_CONST_HIGH", offset);
			case OP_STORE8_OFFSET:  return ABCInstruction("OP_STORE8_OFFSET", offset);
			case OP_STORE16_OFFSET: return ABCInstruction("OP_STORE16_OFFSET", offset);
			case OP_STORE32_OFFSET: return ABCInstruction("OP_STORE32_OFFSET", offset);
			case OP_STORE64_OFFSET: return ABCInstruction("OP_STORE64_OFFSET", offset);
			case OP_LOAD8_OFFSET:  return ABCInstruction("OP_LOAD8_OFFSET", offset);
			case OP_LOAD16_OFFSET: return ABCInstruction("OP_LOAD16_OFFSET", offset);
			case OP_LOAD32_OFFSET: return ABCInstruction("OP_LOAD32_OFFSET", offset);
			case OP_LOAD64_OFFSET: return ABCInstruction("OP_LOAD64_OFFSET", offset);
			case OP_INT_ADD: return ABCInstruction("OP_INT_ADD", offset);
			case OP_INT_SUB: return ABCInstruction("OP_INT_SUB", offset);
			case OP_INT_NEGATE: return ABInstruction("OP_INT_NEGATE", offset);
			case OP_UNSIGN_MUL: return ABCInstruction("OP_USIGN_MUL", offset);
			case OP_UNSIGN_DIV: return ABCInstruction("OP_USIGN_DIV", offset);
			case OP_SIGN_MUL: return ABCInstruction("OP_SIGN_MUL", offset);
			case OP_SIGN_DIV: return ABCInstruction("OP_SIGN_DIV", offset);
			case OP_FLOAT_ADD: return ABCInstruction("OP_FLOAT_ADD", offset);
			case OP_FLOAT_SUB: return ABCInstruction("OP_FLOAT_SUB", offset);
			case OP_FLOAT_MUL: return ABCInstruction("OP_FLOAT_MUL", offset);
			case OP_FLOAT_DIV: return ABCInstruction("OP_FLOAT_DIV", offset);
			case OP_FLOAT_NEGATE: return ABInstruction("OP_FLOAT_NEGATE", offset);
			case OP_DOUBLE_ADD: return ABCInstruction("OP_DOUBLE_ADD", offset);
			case OP_DOUBLE_SUB: return ABCInstruction("OP_DOUBLE_SUB", offset);
			case OP_DOUBLE_MUL: return ABCInstruction("OP_DOUBLE_MUL", offset);
			case OP_DOUBLE_DIV: return ABCInstruction("OP_DOUBLE_DIV", offset);
			case OP_DOUBLE_NEGATE: return ABInstruction("OP_DOUBLE_NEGATE", offset);
			case OP_UNSIGN_LESS:    return ABCInstruction("OP_USIGN_LESS", offset);
			case OP_UNSIGN_GREATER: return ABCInstruction("OP_USIGN_LESS", offset);
			case OP_SIGN_LESS: 	    return ABCInstruction("OP_SIGN_LESS", offset);
			case OP_SIGN_GREATER:   return ABCInstruction("OP_SIGN_LESS", offset);
			case OP_INT_EQUAL:      return ABCInstruction("OP_INT_EQUAL", offset);
			case OP_FLOAT_LESS:    return ABCInstruction("OP_FLOAT_LESS", offset);
			case OP_FLOAT_GREATER: return ABCInstruction("OP_FLOAT_GREATER", offset);
			case OP_FLOAT_EQUAL:   return ABCInstruction("OP_FLOAT_EQUAL", offset);
			case OP_DOUBLE_LESS:    return ABCInstruction("OP_DOUBLE_LESS", offset);
			case OP_DOUBLE_GREATER:	return ABCInstruction("OP_DOUBLE_GREATER", offset);
			case OP_DOUBLE_EQUAL:	return ABCInstruction("OP_DOUBLE_EQUAL", offset);
			case OP_INT_TO_FLOAT:   return ABInstruction("OP_INT_TO_FLOAT", offset);
			case OP_FLOAT_TO_INT:   return ABInstruction("OP_FLOAT_TO_INT", offset);
			case OP_FLOAT_TO_DOUBLE:  return ABInstruction("OP_FLOAT_TO_DOUBLE", offset);
			case OP_DOUBLE_TO_FLOAT:  return ABInstruction("OP_DOUBLE_TO_FLOAT", offset);
			case OP_INT_TO_DOUBLE:   return ABInstruction("OP_INT_TO_DOUBLE", offset);
			case OP_DOUBLE_TO_INT:   return ABInstruction("OP_DOUBLE_TO_INT", offset);
			case OP_BITWISE_AND: return ABCInstruction("OP_BITWISE_AND", offset);
			case OP_BITWISE_OR:  return ABCInstruction("OP_BITWISE_OR", offset);
			case OP_LOGICAL_AND: return ABCInstruction("OP_LOGICAL_AND", offset);
			case OP_LOGICAL_OR:  return ABCInstruction("OP_LOGICAL_OR", offset);
			case OP_LOGICAL_NOT: return ABInstruction("OP_LOGICAL_NOT", offset);
			case OP_RELATIVE_JUMP: return JumpInstruction("OP_RELATIVE_JUMP", offset);
			case OP_RELATIVE_JUMP_IF_TRUE: return JumpInstruction("OP_RELATIVE_JUMP_IF_TRUE", offset);
			case OP_REGISTER_JUMP: return ABInstruction("OP_REGISTER_JUMP", offset);
			case OP_REGISTER_JUMP_IF_TRUE: return ABInstruction("OP_REGISTER_JUMP_IF_TRUE", offset);
		}
	}

	size_t Disassembler::SimpleInstruction(const char* name, size_t offset)
	{
		std::cout << name << std::endl;
		return offset + 1;
	}

	size_t Disassembler::ABInstruction(const char* name, size_t offset)
	{
		uint8_t A = static_cast<uint8_t>(chunk->at(offset) >> 16);
		uint8_t B = static_cast<uint8_t>(chunk->at(offset) >> 8);

		std::cout << std::setfill('0') << name  << " " << std::setw(3) << +A << " " << std::setw(3) << +B << std::endl;

		return offset + 1;
	}

	size_t Disassembler::AInstruction(const char* name, size_t offset)
	{
		
		uint8_t A = static_cast<uint8_t>(chunk->at(offset) >> 16);
		std::cout << std::setfill('0') << name << " " << std::setw(3) << +A << std::endl;   

		return offset + 1;
	}

	size_t Disassembler::ABCInstruction(const char* name, size_t offset)
	{
		uint8_t A = static_cast<uint8_t>(chunk->at(offset) >> 16);
		uint8_t B = static_cast<uint8_t>(chunk->at(offset) >> 8);
		uint8_t C = static_cast<uint8_t>(chunk->at(offset));

		std::cout << std::setfill('0') << name << " "<< std::setw(3) << +A << " " << std::setw(3) << +B << " " << std::setw(3) << +C << std::endl;

		return offset + 1;
	}

	size_t Disassembler::ConstantInstruction(const char* name, size_t offset)
	{
		uint8_t A = static_cast<uint8_t>(chunk->at(offset) >> 16);
		uint16_t constant = static_cast<uint16_t>(chunk->at(offset));

		std::cout << std::setfill('0') << name << " " << std::setw(3) << +A << " " << std::bitset<16>(constant) << std::endl;

		return offset + 1;
	}

	size_t Disassembler::JumpInstruction(const char* name, size_t offset)
	{
		uint32_t jumpSize = chunk->at(offset) & 0x00FFFFFF;
		jumpSize = jumpSize + (0xFF000000 * (jumpSize >> 23));

		std::cout << std::setfill('0') << name << " " << std::setw(3) << static_cast<int32_t>(jumpSize) << std::endl;
		
		return offset + 1;
	}
}