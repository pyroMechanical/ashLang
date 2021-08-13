#pragma once

#include <vector>
#include <stdint.h>
namespace ash
{
	class Chunk
	{
	private:
		std::vector<uint8_t> opcode;
		std::vector<std::pair<int, int>> lines;
	public:
		Chunk() = default;
		~Chunk() = default;
		void WriteAB(uint8_t op, uint8_t A, uint8_t B, int line);
		void WriteABC(uint8_t op, uint8_t A, uint8_t B, uint8_t C, int line);
		void WriteOp(uint8_t op) { opcode.push_back(op); }
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

		uint8_t* code() { return opcode.data(); }
		size_t size() { return opcode.size(); }
		uint8_t at(size_t offset) { return opcode[offset]; }

		int GetLine(size_t offset);

	};

	enum OpCodes : uint8_t
	{
		OP_MOVE, // A, B
		OP_ALLOC, // A, B; allocate R[A] bytes of memory on the heap, store address in R[B]
		OP_CONST, // A, 4 byte constant
		OP_CONST_LONG, // A, 8 byte constant
		OP_STORE8, // A, B; store value from R[A] to memory address R[B]
		OP_STORE16, // A, B; store value from R[A] to memory address R[B]
		OP_STORE32, // A, B; store value from R[A] to memory address R[B]
		OP_STORE64, // A, B; store value from R[A] to memory address R[B]
		OP_LOAD8, // A, B; load value from address R[B] to R[A]
		OP_LOAD16, // A, B; load value from address R[B] to R[A]
		OP_LOAD32, // A, B; load value from address R[B] to R[A]
		OP_LOAD64, // A, B; load value from address R[B] to R[A]
		OP_STORE8_OFFSET, // A, B, C; store value from R[A] to memory address R[B] + R[C]
		OP_STORE16_OFFSET, // A, B, C; store value from R[A] to memory address R[B] + R[C]
		OP_STORE32_OFFSET, // A, B, C; store value from R[A] to memory address R[B] + R[C]
		OP_STORE64_OFFSET, // A, B, C; store value from R[A] to memory address R[B] + R[C]
		OP_LOAD8_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_LOAD16_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_LOAD32_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_LOAD64_OFFSET, // A, B, C; load value from address R[B] + R[C] to R[A]
		OP_PUSH, //A; push value from R[A] onto stack
		OP_POP, //A; pop value from stack into R[A]
			/*signed integers are sign-extended, so 
				one addition/subtraction operation works for both*/
		OP_INT_EQUAL, //A, B, C; R[C]: R[A] = R[B]
		OP_INT_ADD, //A, B, C; R[C]: R[A] + R[B]
		OP_INT_SUB, //A, B, C; R[C]: R[A] - R[B]
			//arithmetic and comparison operations on unsigned integers
		OP_UNSIGN_MUL, //A, B, C; R[C]: R[A] * R[B]
		OP_UNSIGN_DIV, //A, B, C; R[C]: R[A] / R[B]
		OP_UNSIGN_LESS, //A, B, C; R[C]: R[A] < R[B]
		OP_UNSIGN_GREATER, //A, B, C; R[C]: R[A] > R[B]
			//arithmetic and comparison operations on unsigned integers
		OP_SIGN_MUL, //A, B, C; R[C]: R[A] * R[B]
		OP_SIGN_DIV, //A, B, C; R[C]: R[A] / R[B]
		OP_SIGN_LESS, //A, B, C; R[C]: R[A] < R[B]
		OP_SIGN_GREATER, //A, B, C; R[C]: R[A] > R[B]
			//arithmetic and comparison operations on single-precision floats
		OP_FLOAT_ADD,
		OP_FLOAT_SUB,
		OP_FLOAT_MUL,
		OP_FLOAT_DIV,
		OP_FLOAT_LESS,
		OP_FLOAT_GREATER,
		OP_FLOAT_EQUAL,
			//arithmetic and comparison operators on double-precision floats
		OP_DOUBLE_ADD,
		OP_DOUBLE_SUB,
		OP_DOUBLE_MUL,
		OP_DOUBLE_DIV,
		OP_DOUBLE_LESS,
		OP_DOUBLE_GREATER,
		OP_DOUBLE_EQUAL,
			//conversion operations
		OP_INT_TO_FLOAT, // A, B; R[B]: (float)R[A]
		OP_FLOAT_TO_INT, // A, B; R[B]: (int)R[A]
		OP_FLOAT_TO_DOUBLE, // A, B; R[B]: (double)R[A]
		OP_DOUBLE_TO_FLOAT, // A, B; R[B]: (float)R[A]
		OP_INT_TO_DOUBLE, // A, B; R[B]: (double)R[A]
		OP_DOUBLE_TO_INT, // A, B; R[B]: (int)R[A]
		OP_AND, //A, B, C; R[C]: R[A] and R[B]
		OP_OR, //A, B, C; R[C]: R[A] or R[B]
		OP_CALL, 
		OP_RETURN,
	};
}