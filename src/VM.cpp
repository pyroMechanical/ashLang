#include "VM.h"
#include "Compiler.h"

#include <iostream>
#include <bitset>

#define STRESSTEST_GC
//#def LOG_GC
#define ARRAY_SIZE_OFFSET 8
#define ARRAY_BEGIN_OFFSET 10

namespace ash
{
	namespace util
	{
		inline static uint32_t fetch_instruction(uint32_t*& ip)
		{
			return *ip++;
		}

		inline static uint8_t RegisterA(uint32_t instruction)
		{
			uint8_t A = instruction >> 16;
			return A;
		}

		inline static uint8_t RegisterB(uint32_t instruction)
		{
			uint8_t B = instruction >> 8;
			return B;
		}

		inline static uint8_t RegisterC(uint32_t instruction)
		{
			uint8_t C = instruction;
			return C;
		}

		inline static uint16_t Value(uint32_t instruction)
		{
			uint16_t value = instruction;
			return value;
		}

		inline static uint32_t JumpOffset(uint32_t instruction)
		{
			uint32_t offset = instruction & 0x00FFFFFF;
			offset += (!!(offset & (1 << (24 - 1))) * 0xFF000000);
			return offset;
		}

		template<typename T>
		inline static T r_cast(void* value)
		{
			return *reinterpret_cast<T*>(value);
		}

		template<typename T>
		inline static T r_cast(uint64_t value)
		{
			return *reinterpret_cast<T*>(value);
		}
	}

	VM::VM()
	{
		R.fill(0);
		rFlags.fill(0);
	}

	VM::~VM()
	{
		freeAllocations();
	}

	bool VM::isTruthy(uint8_t _register)
	{
		bool result = false;
		if (rFlags[_register] & REGISTER_HOLDS_FLOAT)
		{
			result = ( 0x7FFFFFFF & R[_register]);
		}
		else if (rFlags[_register] & REGISTER_HOLDS_DOUBLE)
		{
			result = (0x7FFFFFFFFFFFFFFF & R[_register]);
		}
		else
		{
			result = R[_register];
		}
		return result;
	}

	InterpretResult VM::error(const char* msg)
	{
		std::cout << msg << std::endl;
		return InterpretResult::INTERPRET_RUNTIME_ERROR;
	}

	InterpretResult VM::interpret(std::string source)
	{
		Compiler compiler;

		bool compileSuccess = compiler.compile(source.c_str());

		if (!compileSuccess) return InterpretResult::INTERPRET_COMPILE_ERROR;
		//InterpretResult result = run();

		//return result;

		return InterpretResult::INTERPRET_OK;
	}

	InterpretResult VM::interpret(Chunk* chunk)
	{
		this->chunk = chunk;
		ip = chunk->code();
		//this->types = chunk->types;
		return run();
	}

	InterpretResult VM::run()
	{
		using namespace util;

		while(true)
		{
			uint32_t instruction = fetch_instruction(ip);
			uint8_t opcode = instruction >> 24;

			switch (opcode)
			{
				case OP_MOVE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					R[B] = R[A];
					rFlags[B] = rFlags[A];
					break;
				}
				case OP_ALLOC:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					size_t toAllocate = R[A];
					void* alloc = allocate(nullptr, 0, toAllocate);
					R[B] = reinterpret_cast<uint64_t>(alloc);
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_POINTER;
					break;
				}
				case OP_ALLOC_ARRAY:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					size_t count = R[A];
					uint8_t span = static_cast<uint8_t>(R[B]);
					void* alloc = allocateArray(nullptr, 0, count, span);
					R[C] = reinterpret_cast<uint64_t>(alloc);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_POINTER;
					rFlags[C] |= REGISTER_HOLDS_ARRAY;
					break;
				}
				case OP_CONST_LOW:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					R[A] = value;
					rFlags[A] = rFlags[A] & (~REGISTER_HOLDS_POINTER);
					break;
				}
				case OP_CONST_MID_LOW:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					R[A] = (R[A] & 0xFFFFFFFF0000FFFF) + (((uint64_t)value) <<16);
					rFlags[A] = rFlags[A] & (~REGISTER_HOLDS_POINTER);
					break;
				}
				case OP_CONST_MID_HIGH:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					R[A] = (R[A] & 0xFFFF0000FFFFFFFF) + (((uint64_t)value) << 32);
					rFlags[A] = rFlags[A] & (~REGISTER_HOLDS_POINTER);
					break;
				}
				case OP_CONST_HIGH:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					R[A] = (R[A] & 0x0000FFFFFFFFFFFF) + (((uint64_t)value) << 48);
					rFlags[A] = rFlags[A] & (~REGISTER_HOLDS_POINTER);
					break;
				}
				case OP_STORE8:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory);
					*addr = static_cast<uint8_t>(R[A]);
					break;
				}
				case OP_STORE16:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory);
					*addr = static_cast<uint16_t>(R[A]);
					break;
				}
				case OP_STORE32:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory);
					*addr = static_cast<uint32_t>(R[A]);
					break;
				}
				case OP_STORE64:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory);
					*addr = R[A];
					break;
				}
				case OP_LOAD8:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_LOAD16:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_LOAD32:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_LOAD64:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_STORE8_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory + R[C]);
					*addr = static_cast<uint8_t>(R[A]);
					break;
				}
				case OP_STORE16_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory + R[C]);
					*addr = static_cast<uint16_t>(R[A]);
					break;
				}
				case OP_STORE32_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory + R[C]);
					*addr = static_cast<uint32_t>(R[A]);
					break;
				}
				case OP_STORE64_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory + R[C]);
					*addr = R[A];
					break;
				}
				case OP_LOAD8_OFFSET:
				{
					uint8_t A = fetch_instruction(ip);
					uint8_t B = fetch_instruction(ip);
					uint8_t C = fetch_instruction(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_LOAD16_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_LOAD32_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_LOAD64_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_ARRAY_STORE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					if ((rFlags[B] & REGISTER_HOLDS_ARRAY) == 0) return error("pointer held in register is not an array!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					uint8_t span = (*alloc->memory) & 0x7F;
					uint64_t arrayCount = *reinterpret_cast<uint64_t*>(alloc->memory + ARRAY_SIZE_OFFSET);
					if (R[C] >= arrayCount) return error("array index out of bounds!");
					uint64_t offset = span * R[C];
					switch (span)
					{
					case 1:
					{
						auto addr = reinterpret_cast<uint8_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						*addr = static_cast<uint8_t>(R[A]);
						break;
					}
					case 2:
					{
						auto addr = reinterpret_cast<uint16_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						*addr = static_cast<uint16_t>(R[A]);
						break;
					}
					case 4:
					{
						auto addr = reinterpret_cast<uint32_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						*addr = static_cast<uint32_t>(R[A]);
						break;
					}
					case 8:
					{
						auto addr = reinterpret_cast<uint64_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						*addr = R[A];
						break;
					}
					}
					break;
				}
				case OP_ARRAY_LOAD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					if ((rFlags[B] & REGISTER_HOLDS_ARRAY) == 0) return error("pointer held in register is not an array!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					uint8_t span = (*alloc->memory) & 0x7F;
					uint64_t arrayCount = *reinterpret_cast<uint64_t*>(alloc->memory + ARRAY_SIZE_OFFSET);
					if (R[C] >= arrayCount) return error("array index out of bounds!");
					uint64_t offset = span * R[C];
					switch (span)
					{
					case 1:
					{
						auto addr = reinterpret_cast<uint8_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						R[A] = *addr;
						break;
					}
					case 2:
					{
						auto addr = reinterpret_cast<uint16_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						R[A] = *addr;
						break;
					}
					case 4:
					{
						auto addr = reinterpret_cast<uint32_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						R[A] = *addr;
						break;
					}
					case 8:
					{
						auto addr = reinterpret_cast<uint64_t*>(alloc->memory + ARRAY_BEGIN_OFFSET + offset);
						R[A] = *addr;
						break;
					}
					}
					break;
				}
				case OP_PUSH:
				{
					uint8_t A = RegisterA(instruction);
					stack.push_back(R[A]);
					if ((rFlags[A] & REGISTER_HOLDS_POINTER) != 0) stackPointers.push_back(stack.size() - 1);
					rFlags[A] = 0;
					break;
				}
				case OP_POP:
				{
					uint8_t A = RegisterA(instruction);
					R[A] = stack.back();
					if (stackPointers.back() == stack.size() - 1)
					{
						rFlags[A] = REGISTER_HOLDS_POINTER;
						stackPointers.pop_back();
					}
					stack.pop_back();
					break;
				}
				case OP_INT_ADD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = R[A] + R[B];
					rFlags[C] &= (REGISTER_HIGH_BITS | REGISTER_HOLDS_SIGNED);
					break;
				}
				case OP_INT_SUB:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = R[A] - R[B];
					rFlags[C] &= (REGISTER_HIGH_BITS | REGISTER_HOLDS_SIGNED);
					break;
				}
				case OP_INT_NEGATE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					R[B] = -static_cast<int64_t>(R[A]);
					rFlags[B] &= (REGISTER_HIGH_BITS | REGISTER_HOLDS_SIGNED);
				}
				case OP_UNSIGN_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = R[A] * R[B];
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_UNSIGN_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = R[A] / R[B];
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_UNSIGN_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = R[A] < R[B];
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_INT_EQUAL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = R[A] == R[B];
					rFlags[C] &= (REGISTER_HIGH_BITS | REGISTER_HOLDS_SIGNED);
					break;
				}
				case OP_UNSIGN_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = R[A] > R[B];
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_SIGN_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = static_cast<uint64_t>((static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_SIGNED;
					break;
				}
				case OP_SIGN_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = static_cast<uint64_t>((static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_SIGNED;
					break;
				}
				case OP_SIGN_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					
					R[C] = comparisonRegister = (static_cast<int64_t>(R[A]) < static_cast<int64_t>(R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_SIGNED;
					break;
				}
				case OP_SIGN_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (static_cast<int64_t>(R[A]) > static_cast<int64_t>(R[B]));
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_ADD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					float temp = (r_cast<float>(&R[A]) + r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_SUB:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					float temp = (r_cast<float>(&R[A]) - r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					float temp = (r_cast<float>(&R[A]) * r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					float temp = (r_cast<float>(&R[A]) / r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_NEGATE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					R[B] = R[A] ^ 0x0000000080000000;
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (r_cast<float>(&R[A]) < r_cast<float>(&R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (r_cast<float>(&R[A]) > r_cast<float>(&R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_EQUAL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (r_cast<float>(&R[A]) == r_cast<float>(&R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_DOUBLE_ADD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					double temp = (r_cast<double>(&R[A]) + r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_SUB:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					double temp = (r_cast<double>(&R[A]) - r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					double temp = (r_cast<double>(&R[A]) * r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					double temp = (r_cast<double>(&R[A]) / r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_NEGATE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					R[B] = R[A] ^ 0x8000000000000000;
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (r_cast<double>(&R[A]) < r_cast<double>(&R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (r_cast<double>(&R[A]) > r_cast<double>(&R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_EQUAL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = comparisonRegister = (r_cast<double>(&R[A]) == r_cast<double>(&R[B]));
					rFlags[C] &= REGISTER_HIGH_BITS;
					rFlags[C] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_INT_TO_FLOAT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					float temp = static_cast<float>(static_cast<int64_t>(R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] = 0;
					break;
				}
				case OP_FLOAT_TO_INT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					R[B] = static_cast<uint64_t>(static_cast<int64_t>(static_cast<float>(R[A])));
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_FLOAT_TO_DOUBLE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					double temp = static_cast<double>(r_cast<float>(&R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_TO_FLOAT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					float temp = static_cast<float>(r_cast<double>(&R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_FLOAT;
					break;
				}
				case OP_INT_TO_DOUBLE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					double temp = static_cast<double>(static_cast<int64_t>(R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_DOUBLE;
					break;
				}
				case OP_DOUBLE_TO_INT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					R[B] = static_cast<uint64_t>(static_cast<int64_t>(static_cast<double>(R[A])));
					rFlags[B] &= REGISTER_HIGH_BITS;
					rFlags[B] |= REGISTER_HOLDS_SIGNED;
					break;
				}
				case OP_BITWISE_AND:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = R[A] & R[B];
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_BITWISE_OR:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					R[C] = R[A] | R[B];
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_LOGICAL_AND:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					bool isATruthy = isTruthy(A);
					bool isBTruthy = isTruthy(B);

					R[C] = comparisonRegister = isATruthy && isBTruthy;
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_LOGICAL_OR:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					bool isATruthy = isTruthy(A);
					bool isBTruthy = isTruthy(B);

					R[C] = comparisonRegister = isATruthy || isBTruthy;
					rFlags[C] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_LOGICAL_NOT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					bool isATruthy = isTruthy(A);

					R[B] = comparisonRegister = !isATruthy;
					rFlags[B] &= REGISTER_HIGH_BITS;
					break;
				}
				case OP_STORE_IP_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					R[A] = ip - chunk->code();
					break;
				}
				case OP_RELATIVE_JUMP:
				{
					int32_t jump = (int32_t)JumpOffset(instruction);
					if((ip - chunk->code()) + jump - 1 > static_cast<int64_t>(chunk->size()) || (ip - chunk->code()) + jump - 1 < 0) return error("attempted jump beyond code bounds!");
					ip += jump - 1;
					break;
				}
				case OP_RELATIVE_JUMP_IF_TRUE:
				{
					if (comparisonRegister)
					{
						comparisonRegister = false;
						int32_t jump = (int32_t)JumpOffset(instruction);
						if (((ip - chunk->code()) + jump - 1) > static_cast<int64_t>(chunk->size()) || ((ip - chunk->code()) + jump - 1) < 0) return error("attempted jump beyond code bounds!");
						ip += jump - 1;
					}
					break;
				}
				case OP_REGISTER_JUMP:
				{
					uint8_t A = RegisterA(instruction);
					if (R[A] > chunk->size()) return error("attempted jump beyond code bounds!");
					ip = chunk->code() + R[A];
					break;
				}
				case OP_REGISTER_JUMP_IF_TRUE:
				{
					if (comparisonRegister)
					{
						comparisonRegister = false;
						uint8_t A = RegisterA(instruction);
						if (R[A] > chunk->size()) return error("attempted jump beyond code bounds!");
						ip = chunk->code() + R[A];
					}
					break;
				}
				case OP_OUT:
				{
					uint8_t A = RegisterA(instruction);

					if ((rFlags[A] & REGISTER_HOLDS_SIGNED) != 0) std::cout << static_cast<int64_t>(R[A]) << std::endl;
					else if ((rFlags[A] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<float>(&R[A]) << std::endl;
					else if ((rFlags[A] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<double>(&R[A]) << std::endl;
					else std::cout << R[A] << std::endl;

					break;
				}
				case OP_RETURN: 
				{
					return InterpretResult::INTERPRET_OK;
				}
			}
		}
	}

	void* VM::allocate(uint64_t typeID)
	{
#ifdef STRESSTEST_GC
			collectGarbage();
#else
			//TODO: find a heuristic for calling the garbage collector
#endif
		TypeMetadata* typeInfo = types[typeID].get();
		
		size_t dataSize;
		size_t size;
		uint8_t spacing;
		spacing = (size_t)(typeInfo->fields[0] & 0x7F) - 2;
		if (typeInfo->fields[0] & 0x80) spacing = alignof(void*) - 2;
		if (spacing < 0) spacing = 0;
		for (const auto typeData : typeInfo->fields)
		{
			dataSize += (typeData & 0x7F);
		}

		size = 10 + spacing + dataSize; //8 bytes TypeMetadata*, 1 byte for spacing offset, 1 byte for refcount
		if (size % sizeof(void*))
		{
			size /= sizeof(void*);
			size += sizeof(void*);
		}
		void* result = malloc(size);
		if (result == nullptr) exit(1);
		memset(result, 0, size);
		auto typePtr = (TypeMetadata**)result;
		*typePtr = typeInfo;
		auto spaceInfo = (uint8_t*)result + 8;
		*spaceInfo = spacing;
		auto refCount = (uint8_t*)result + 9;
		*refCount = 1;
		Allocation* allocation = new Allocation();
		allocation->memory = static_cast<char*>(result);
		allocation->next = allocationList;
		allocationList = allocation;
		return (void*)allocation;
	}

	void* VM::allocateArray(void* pointer, size_t oldCount, size_t newCount, uint8_t span)
	{
		if (newCount > oldCount)
#ifdef STRESSTEST_GC
			collectGarbage();
#else
			//TODO: find a heuristic for calling the garbage collector
#endif
		int64_t padding = (span & 0x7F) - 2;
		if (span & 0x80) padding = alignof(void*) - 2;
		if (padding < 0) padding = 0;
		size_t oldSize = 0;
		if (pointer)
		{
		
			uint8_t* oldArray = (uint8_t*)pointer;
			span = *oldArray;
			oldSize = 10 + padding + (oldCount * (span & 0x7F)); //8 bytes for capacity, 1 byte for span, 1 byte for refcount
		}
		size_t newSize = 10 + padding + (newCount * (span & 0x7F)); // 8 bytes for capacity, 1 byte for span, 1 byte for refcount
		
		void* result = realloc(pointer, newSize);
		if (result == nullptr) exit(1);
		memset((void*)(((char*)result) + oldSize), 0, newSize - oldSize);
		uint64_t* count = reinterpret_cast<uint64_t*>(result);
		*count = newCount;
		uint8_t* arraySpan = ((uint8_t*)result) + 8;
		*arraySpan = span;
		uint8_t* refCount = ((uint8_t*)result + 9);
		*refCount = 1;
		Allocation* allocation = new Allocation();
		allocation->memory = (char*)result;
		allocation->next = allocationList;
		allocationList = allocation;
		return (void*)allocation;
	}


	void VM::freeAllocation(Allocation* alloc)
	{
		delete alloc->memory;
		delete alloc;
	}

	void VM::freeAllocations()
	{
		Allocation* allocation = allocationList;

		while (allocation != nullptr)
		{
			Allocation* next = allocation->next;
			freeAllocation(allocation);
			allocation = next;
		}
	}

	void VM::collectGarbage()
	{
#ifdef LOG_GC
		std::cout << "> begin gc" << std::endl;
#endif

#ifdef LOG_GC
		std::cout << "> marking registers" << std::endl;
#endif
		std::vector<Allocation*> greyset;
		for (int i = 0; i < R.size(); i++)
		{
			if ((rFlags[i] & REGISTER_HOLDS_POINTER) != 0)
			{
				auto alloc = reinterpret_cast<Allocation*>(R[i]);
				alloc->marked = true;
				greyset.push_back(alloc);
			}
		}
#ifdef LOG_GC
		std::cout << "> marking stack" << std::endl;
#endif
		for (int i = 0; i < stackPointers.size(); i++)
		{
			auto alloc = reinterpret_cast<Allocation*>(stack[stackPointers[i]]);
			alloc->marked = true;
			greyset.push_back(alloc);
		}
#ifdef LOG_GC
		std::cout << "> marking from roots" << std::endl;
#endif

		//TODO: implement marking pointers in object memory!

#ifdef LOG_GC
		std::cout << "> begin sweep" << std::endl;
#endif
		Allocation* previous = nullptr;
		Allocation* alloc = allocationList;
		while (alloc != nullptr)
		{
			if (alloc->marked)
			{
				alloc->marked = false;
				previous = alloc;
				alloc = alloc->next;
			}
			else
			{
				auto white = alloc;
				alloc = alloc->next;
				freeAllocation(white);
				if (previous == nullptr)
				{
					allocationList = alloc;
				}
				else
				{
					previous->next = alloc;
				}
			}
		}

#ifdef LOG_GC
		std::cout << "> end sweep" << std::endl;
#endif

#ifdef LOG_GC
		std::cout << "> end gc" << std::endl;
#endif
	}
}