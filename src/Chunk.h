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
		OP_STORE, // A, B, C; store value from R[B] to memory address R[C], bytes A: 0x01 to 0x08
		OP_LOAD, // A, B, C; load value from address R[C] to R[B], bytes A: 0x01 to 0x08
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

	enum StackTransferFlags : uint8_t
	{
		FLAG_BYTE = 0x01,
		FLAG_SHORT = 0x02,
		FLAG_HALF = 0x04,
		FLAG_LONG = 0x08,
		FLAG_MAX_BYTES = 0x0F,
		FLAG_READ_SINT = 0x80
	};
}