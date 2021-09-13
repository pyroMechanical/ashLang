#pragma once

#include <vector>
#include <string>
#include <stdint.h>
namespace ash
{

	class Chunk
	{
	private:
		std::vector<uint32_t> opcode;
		std::vector<std::pair<int, int>> lines;
	public:
		Chunk() = default;
		~Chunk() = default;
		void WriteA(uint8_t op, uint8_t A, int line);
		void WriteAB(uint8_t op, uint8_t A, uint8_t B, int line);
		void WriteABC(uint8_t op, uint8_t A, uint8_t B, uint8_t C, int line);
		void WriteOp(uint8_t op) { opcode.push_back(op << 24); }
		void WriteU8(uint8_t A, uint8_t constant);
		void WriteU16(uint8_t A, uint16_t contant);
		void WriteU32(uint8_t A, uint32_t contant);
		void WriteU64(uint8_t A, uint64_t constant);
		void WriteI8(uint8_t A, int8_t constant);
		void WriteI16(uint8_t A, int16_t constant);
		void WriteI32(uint8_t A, int32_t constant);
		void WriteI64(uint8_t A, int64_t constant);
		void WriteFloat(uint8_t A, float constant);
		void WriteDouble(uint8_t A, double constant);

		void WriteRelativeJump(uint8_t op, int32_t jump, int line);

		uint32_t* code() { return opcode.data(); }
		size_t size() { return opcode.size(); }
		uint32_t at(size_t offset) { return opcode[offset]; }

		int GetLine(size_t offset);

		
	};

	enum OpCodes : uint8_t
	{
		OP_HALT, //stops program execution
			//one-register instructions: 8-bit opcode | 8-bit register A | 16 bits space
		OP_PUSH, //A; push value from R[A] onto stack
		OP_POP,  //A; pop value from stack into R[A]
		OP_RETURN, 
		OP_OUT,  // A, outputs value of R[A] in terminal
		OP_STORE_IP_OFFSET, // A; R[A] = instruction pointer - chunk beginning
			//two-register instructions: 8-bit opcode | 8-bit register A | 8-bit register B | 8 bits space
		OP_MOVE,    // A, B; move value from R[A] to R[B]
		OP_ALLOC,   // A, B; allocate R[A] bytes of memory on the heap, store address in R[B]
		OP_STORE8,  // A, B; store value from R[A] in memory address R[B]
		OP_STORE16, // A, B; store value from R[A] in memory address R[B]
		OP_STORE32, // A, B; store value from R[A] in memory address R[B]
		OP_STORE64, // A, B; store value from R[A] in memory address R[B]
		OP_LOAD8,  // A, B; load value from address R[B] to R[A]
		OP_LOAD16, // A, B; load value from address R[B] to R[A]
		OP_LOAD32, // A, B; load value from address R[B] to R[A]
		OP_LOAD64, // A, B; load value from address R[B] to R[A]
		OP_CONST_LOW, // A, clears R[A], then writes to bottom two bytes
		OP_CONST_LOW_NEGATIVE, // A, clears R[A] to all ones then overwrites the bottom two bytes
		OP_CONST_MID_LOW,  // A, writes to second lowest two bytes
		OP_CONST_MID_HIGH, // A, writes to second higest two bytes
		OP_CONST_HIGH, // A, writes to highest two bytes
			//three-register instructions: 8-bit opcode | 8-bit register A | 8-bit register B | 8-bit register C
		OP_STORE8_OFFSET,  // A, B, C; store value from R[A] in memory address R[B] + R[C]
		OP_STORE16_OFFSET, // A, B, C; store value from R[A] in memory address R[B] + R[C]
		OP_STORE32_OFFSET, // A, B, C; store value from R[A] in memory address R[B] + R[C]
		OP_STORE64_OFFSET, // A, B, C; store value from R[A] in memory address R[B] + R[C]
		OP_LOAD8_OFFSET,  // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_LOAD16_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_LOAD32_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_LOAD64_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_ALLOC_ARRAY, //A, B, C; allocate array with length R[A] and span R[B] and store pointer in R[C]
		OP_ARRAY_STORE, //A, B, C; write value from R[A] in array R[B] with index R[C]
		OP_ARRAY_LOAD, //A, B, C; load value from array R[B] with index R[C] to R[A]
			//signed integers are sign-extended, so one addition/subtraction operation works for both
		OP_INT_ADD, //A, B, C; R[C] = R[A] + R[B]
		OP_INT_SUB, //A, B, C; R[C] = R[A] - R[B]
		OP_INT_NEGATE, //A, B; R[B] = -R[A]
			//arithmetic and comparison operations on unsigned integers
		OP_UNSIGN_MUL, //A, B, C; R[C] = R[A] * R[B]
		OP_UNSIGN_DIV, //A, B, C; R[C] = R[A] / R[B]
			//arithmetic and comparison operations on unsigned integers
		OP_SIGN_MUL, //A, B, C; R[C] = R[A] * R[B]
		OP_SIGN_DIV, //A, B, C; R[C] = R[A] / R[B]
			//arithmetic and comparison operations on single-precision floats
		OP_FLOAT_ADD, //A, B, C; R[C] = R[A] + R[B]
		OP_FLOAT_SUB, //A, B, C; R[C] = R[A] - R[B]
		OP_FLOAT_MUL, //A, B, C; R[C] = R[A] * R[B]
		OP_FLOAT_DIV, //A, B, C; R[C] = R[A] / R[B]
		OP_FLOAT_NEGATE, //A, B; R[B] = -R[A]
			//arithmetic and comparison operators on double-precision floats
		OP_DOUBLE_ADD, //A, B, C; R[C] = R[A] + R[B]
		OP_DOUBLE_SUB, //A, B, C; R[C] = R[A] - R[B]
		OP_DOUBLE_MUL, //A, B, C; R[C] = R[A] * R[B]
		OP_DOUBLE_DIV, //A, B, C; R[C] = R[A] / R[B]
		OP_DOUBLE_NEGATE, //A, B; R[B] = -R[A]
			//comparison operations set the value of the comparison register with their output value
		OP_UNSIGN_LESS, //A, B, C; R[C] = R[A] < R[B]
		OP_UNSIGN_GREATER, //A, B, C; R[C] = R[A] > R[B]
		OP_SIGN_LESS, //A, B, C; R[C] = R[A] < R[B]
		OP_SIGN_GREATER, //A, B, C; R[C] = R[A] > R[B]
		OP_INT_EQUAL, //A, B, C; R[C] = R[A] == R[B]
		OP_FLOAT_LESS, //A, B, C; R[C] = R[A] < R[B]
		OP_FLOAT_GREATER, //A, B, C; R[C] = R[A] > R[B]
		OP_FLOAT_EQUAL, //A, B, C; R[C] = R[A] == R[B]
		OP_DOUBLE_LESS, //A, B, C; R[C] = R[A] < R[B]
		OP_DOUBLE_GREATER, //A, B, C; R[C] = R[A] > R[B]
		OP_DOUBLE_EQUAL, //A, B, C; R[C] = R[A] == R[B]
			//conversion operations
		OP_INT_TO_FLOAT, // A, B; R[B] = (float)R[A]
		OP_FLOAT_TO_INT, // A, B; R[B] = (int)R[A]
		OP_FLOAT_TO_DOUBLE, // A, B; R[B] = (double)R[A]
		OP_DOUBLE_TO_FLOAT, // A, B; R[B] = (float)R[A]
		OP_INT_TO_DOUBLE, // A, B; R[B] = (double)R[A]
		OP_DOUBLE_TO_INT, // A, B; R[B] = (int)R[A]
		OP_BITWISE_AND, //A, B, C; R[C] = R[A] & R[B]
		OP_BITWISE_OR, //A, B, C; R[C] = R[A] | R[B]
		OP_LOGICAL_AND, //A, B, C; R[C] = R[A] && R[B]
		OP_LOGICAL_OR, //A, B, C; R[C] = R[A] || R[B]
		OP_LOGICAL_NOT, //A, B; R[B] = !R[A]
			//jump instructions
			//relative jump instruction: 8-bit opcode | 24-bit signed integer offset
		OP_RELATIVE_JUMP, // instruction pointer += signed integer offset
		OP_RELATIVE_JUMP_IF_TRUE, // if(comparison register), instruction pointer += signed integer offset
				//absolute jump instruction: 8-bit opcode | 8-bit register A | 16 bits space
		OP_REGISTER_JUMP, // instruction pointer = chunk beginning + R[A] 
		OP_REGISTER_JUMP_IF_TRUE, // if(comparison register) instruciton pointer = chunk beginning + R[A]
	};

	static const std::vector<std::string> OpcodeNames = {
			"OP_HALT",
			"OP_PUSH",
			"OP_POP",
			"OP_RETURN",
			"OP_OUT",
			"OP_STORE_IP_OFFSET",
			"OP_MOVE",
			"OP_ALLOC",
			"OP_STORE8",
			"OP_STORE16",
			"OP_STORE32",
			"OP_STORE64",
			"OP_LOAD8",
			"OP_LOAD16",
			"OP_LOAD32",
			"OP_LOAD64",
			"OP_CONST_LOW",
			"OP_CONST_MID_LOW",
			"OP_CONST_MID_HIGH",
			"OP_CONST_HIGH",
			"OP_STORE8_OFFSET", 
			"OP_STORE16_OFFSET",
			"OP_STORE32_OFFSET", 
			"OP_STORE64_OFFSET", 
			"OP_LOAD8_OFFSET",
			"OP_LOAD16_OFFSET", 
			"OP_LOAD32_OFFSET",
			"OP_LOAD64_OFFSET", 
			"OP_INT_ADD", 
			"OP_INT_SUB", 
			"OP_INT_NEGATE", 
			"OP_UNSIGN_MUL", 
			"OP_UNSIGN_DIV", 
			"OP_SIGN_MUL", 
			"OP_SIGN_DIV", 
			"OP_FLOAT_ADD",
			"OP_FLOAT_SUB",
			"OP_FLOAT_MUL",
			"OP_FLOAT_DIV",
			"OP_FLOAT_NEGATE", 
			"OP_DOUBLE_ADD", 
			"OP_DOUBLE_SUB", 
			"OP_DOUBLE_MUL", 
			"OP_DOUBLE_DIV", 
			"OP_DOUBLE_NEGATE", 
			"OP_UNSIGN_LESS", 
			"OP_UNSIGN_GREATER",
			"OP_SIGN_LESS", 
			"OP_SIGN_GREATER",
			"OP_INT_EQUAL", 
			"OP_FLOAT_LESS",
			"OP_FLOAT_GREATER", 
			"OP_FLOAT_EQUAL", 
			"OP_DOUBLE_LESS", 
			"OP_DOUBLE_GREATER",
			"OP_DOUBLE_EQUAL", 
			"OP_INT_TO_FLOAT", 
			"OP_FLOAT_TO_INT", 
			"OP_FLOAT_TO_DOUBLE", 
			"OP_DOUBLE_TO_FLOAT", 
			"OP_INT_TO_DOUBLE",
			"OP_DOUBLE_TO_INT",
			"OP_BITWISE_AND",
			"OP_BITWISE_OR", 
			"OP_LOGICAL_AND",
			"OP_LOGICAL_OR", 
			"OP_LOGICAL_NOT",
			"OP_RELATIVE_JUMP", 
			"OP_RELATIVE_JUMP_IF_TRUE",
			"OP_REGISTER_JUMP",
			"OP_REGISTER_JUMP_IF_TRUE"
	};
}