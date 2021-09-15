#include "VM.h"
#include "Compiler.h"

#include <iostream>
#include <queue>
#include <bitset>
#include <typeinfo>
#include <typeindex>

#define STRESSTEST_GC
//#def LOG_GC
#define ARRAY_TYPE_OFFSET 8
#define STRUCT_SPACING_OFFSET 8
#define REFCOUNT_OFFSET 9
#define OBJECT_BEGIN_OFFSET 10

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
			result = (*reinterpret_cast<float*>(&R[_register]) == 0.0f || *reinterpret_cast<float*>(&R[_register]) == -0.0f);
		}
		else if (rFlags[_register] & REGISTER_HOLDS_DOUBLE)
		{
			result = (*reinterpret_cast<double*>(&R[_register]) == 0.0 || *reinterpret_cast<double*>(&R[_register]) == -0.0);
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
					if (rFlags[B] & REGISTER_HOLDS_POINTER) refDecrement(reinterpret_cast<Allocation*>(R[B]));
					setRegister(B, R[A]);
					break;
				}
				case OP_ALLOC:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint64_t typeID = R[A];
					Allocation* alloc = allocate(typeID);
					setRegister(B, alloc);
					break;
				}
				case OP_ALLOC_ARRAY:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					size_t count = R[A];
					uint8_t span = static_cast<uint8_t>(R[B]);
					Allocation* alloc = allocateArray(nullptr, 0, count, span);
					setRegister(C, alloc);
					break;
				}
				case OP_CONST_LOW:
				{
					uint8_t A = RegisterA(instruction);
					uint64_t value = Value(instruction);
					setRegister(A, value);
					break;
				}
				case OP_CONST_LOW_NEGATIVE:
				{
					uint8_t A = RegisterA(instruction);
					uint64_t value = Value(instruction) | 0xFFFFFFFFFFFF0000;
					setRegister(A, value);
				}
				case OP_CONST_MID_LOW:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					setRegister(A, (R[A] & 0xFFFFFFFF0000FFFF) + (((uint64_t)value) <<16));
					break;
				}
				case OP_CONST_MID_HIGH:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					setRegister(A, (R[A] & 0xFFFF0000FFFFFFFF) + (((uint64_t)value) << 32));
					break;
				}
				case OP_CONST_HIGH:
				{
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					setRegister(A,(R[A] & 0x0000FFFFFFFFFFFF) + (((uint64_t)value) << 48));
					break;
				}
				case OP_STORE_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto spacing = *(alloc->memory + STRUCT_SPACING_OFFSET);
					TypeMetadata* metadata = (TypeMetadata*)alloc->memory;
					if (R[C] >= metadata->fields.size()) return error("field out of bounds!");
					size_t offset = metadata->fields[R[C]].offset;
					FieldType type = metadata->fields[R[C]].type;
					if (type == FieldType::Array || type == FieldType::Struct)
					{
						Allocation* ref = *reinterpret_cast<Allocation**>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						if(ref)
							refDecrement(ref);
					}
					if (rFlags[A] & REGISTER_HOLDS_POINTER)
					{
						Allocation* ref = *reinterpret_cast<Allocation**>(&R[A]);
						refIncrement(ref);
					}
					switch (fieldSize(type))
					{
					case 1:
					{
						auto addr = reinterpret_cast<uint8_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint8_t>(R[A]);
						break;
					}
					case 2:
					{
						auto addr = reinterpret_cast<uint16_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint16_t>(R[A]);
						break;
					}
					case 4:
					{
						auto addr = reinterpret_cast<uint32_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint32_t>(R[A]);
						break;
					}
					case 8:
					{
						auto addr = reinterpret_cast<uint64_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint64_t>(R[A]);
						break;
					}
					}
					break;
				}
				case OP_LOAD_OFFSET:
				{
					uint8_t A = fetch_instruction(ip);
					uint8_t B = fetch_instruction(ip);
					uint8_t C = fetch_instruction(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto spacing = *(alloc->memory + STRUCT_SPACING_OFFSET);
					TypeMetadata* metadata = (TypeMetadata*)alloc->memory;
					if (R[C] >= metadata->fields.size()) return error("field out of bounds!");
					size_t offset = metadata->fields[R[C]].offset;
					FieldType type = metadata->fields[R[C]].type;
					switch (type)
					{
						case FieldType::Bool:
						case FieldType::UByte:
						{
							auto addr = reinterpret_cast<uint8_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, (uint64_t)*addr);
							break;
						}
						case FieldType::UShort:
						{
							auto addr = reinterpret_cast<uint16_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, (uint64_t)*addr);
							break;
						}

						case FieldType::UInt:
						{
							auto addr = reinterpret_cast<uint32_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, (uint64_t)*addr);
							break;
						}
						case FieldType::ULong:
						{
							auto addr = reinterpret_cast<uint64_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Byte:
						{
							auto addr = reinterpret_cast<int8_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, (int64_t)*addr);
							break;
						}
						case FieldType::Short:
						{
							auto addr = reinterpret_cast<int16_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, (int64_t)*addr);
							break;
						}
						case FieldType::Int:
						{
							auto addr = reinterpret_cast<int32_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, (int64_t)*addr);
							break;
						}
						case FieldType::Long:
						{
							auto addr = reinterpret_cast<int64_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Float:
						{
							auto addr = reinterpret_cast<float*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Double:
						{
							auto addr = reinterpret_cast<double*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Struct:
						case FieldType::Array:
						{
							auto addr = reinterpret_cast<Allocation**>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + R[C]);
							setRegister(A, *addr);
							break;
						}
					}
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
					uint8_t span = (*((char*)alloc->memory + ARRAY_TYPE_OFFSET)) & 0x7F;
					uint8_t spacing = (span - 2) * ((span - 2) > 0);
					uint64_t arrayCount = *reinterpret_cast<uint64_t*>(alloc->memory);
					if (R[C] >= arrayCount) return error("array index out of bounds!");
					uint64_t offset = span * R[C];
					switch (span)
					{
					case 1:
					{
						auto addr = reinterpret_cast<uint8_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint8_t>(R[A]);
						break;
					}
					case 2:
					{
						auto addr = reinterpret_cast<uint16_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint16_t>(R[A]);
						break;
					}
					case 4:
					{
						auto addr = reinterpret_cast<uint32_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						*addr = static_cast<uint32_t>(R[A]);
						break;
					}
					case 8:
					{
						auto addr = reinterpret_cast<uint64_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
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
					if (rFlags[A] & REGISTER_HOLDS_POINTER) refDecrement(reinterpret_cast<Allocation*>(R[A]));
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					uint8_t span = (*((char*)alloc->memory + ARRAY_TYPE_OFFSET)) & 0x7F;
					uint8_t spacing = (span - 2) * ((span - 2) > 0);
					bool isPtr = (*((char*)alloc->memory + ARRAY_TYPE_OFFSET)) & 0x80;
					uint64_t arrayCount = *reinterpret_cast<uint64_t*>(alloc->memory);
					if (R[C] >= arrayCount) return error("array index out of bounds!");
					uint64_t offset = span * R[C];
					switch (span)
					{
					case 1:
					{
						auto addr = reinterpret_cast<uint8_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						R[A] = *addr;
						break;
					}
					case 2:
					{
						auto addr = reinterpret_cast<uint16_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						R[A] = *addr;
						break;
					}
					case 4:
					{
						auto addr = reinterpret_cast<uint32_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						R[A] = *addr;
						break;
					}
					case 8:
					{
						auto addr = reinterpret_cast<uint64_t*>(alloc->memory + OBJECT_BEGIN_OFFSET + spacing + offset);
						R[A] = *addr;
						break;
					}
					}
					if (isPtr)
					{
						auto objectAddress = reinterpret_cast<Allocation*>(R[A]);
						rFlags[A] &= REGISTER_HIGH_BITS;
						switch (objectAddress->type())
						{
						case AllocationType::Array:
							rFlags[A] |= REGISTER_HOLDS_ARRAY;
						case AllocationType::Type:
							rFlags[A] |= REGISTER_HOLDS_POINTER;
						}
						refIncrement(objectAddress);
					}
					break;
				}
				case OP_PUSH:
				{
					uint8_t A = RegisterA(instruction);
					stack.push_back(R[A]);
					stackFlags.push_back(rFlags[A]);
					if ((rFlags[A] & REGISTER_HOLDS_POINTER) != 0)
					{
						stackPointers.push_back(stack.size() - 1);
						refIncrement(*reinterpret_cast<Allocation**>(&R[A]));
					}
					rFlags[A] = 0;
					break;
				}
				case OP_POP:
				{
					uint8_t A = RegisterA(instruction);
					R[A] =  stack.back();
					rFlags[A] = stackFlags.back();
					if (stackPointers.back() == stack.size() - 1)
					{
						stackPointers.pop_back();
						//no need to decrement: register holds value, no net change in refcount
					}
					stack.pop_back();
					stackFlags.pop_back();
					break;
				}
				case OP_INT_ADD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C,static_cast<int64_t>(R[A] + R[B]));
					break;
				}
				case OP_INT_SUB:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, static_cast<int64_t>(R[A] - R[B]));
					break;
				}
				case OP_INT_NEGATE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					setRegister(B, -static_cast<int64_t>(R[A]));
				}
				case OP_UNSIGN_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, R[A] * R[B]);
					break;
				}
				case OP_UNSIGN_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, R[A] / R[B]);
					break;
				}
				case OP_UNSIGN_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = R[A] < R[B]);
					break;
				}
				case OP_INT_EQUAL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = R[A] == R[B]);
					break;
				}
				case OP_UNSIGN_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = R[A] > R[B]);
					break;
				}
				case OP_SIGN_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (static_cast<int64_t>(R[A]) / static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					
					setRegister(C, comparisonRegister = (static_cast<int64_t>(R[A]) < static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (static_cast<int64_t>(R[A]) > static_cast<int64_t>(R[B])));
					break;
				}
				case OP_FLOAT_ADD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) + *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_SUB:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) - *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) * *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) / *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_NEGATE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					setRegister(B, -*reinterpret_cast<float*>(&R[A]));
					break;
				}
				case OP_FLOAT_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (*reinterpret_cast<float*>(&R[A]) < *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (*reinterpret_cast<float*>(&R[A]) > *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_EQUAL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (*reinterpret_cast<float*>(&R[A]) == *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_DOUBLE_ADD:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) + *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_SUB:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) - *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_MUL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) * *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_DIV:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) / *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_NEGATE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					setRegister(B, *reinterpret_cast<double*>(&R[A]));
					break;
				}
				case OP_DOUBLE_LESS:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (r_cast<double>(&R[A]) < r_cast<double>(&R[B])));
					break;
				}
				case OP_DOUBLE_GREATER:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (r_cast<double>(&R[A]) > r_cast<double>(&R[B])));
					break;
				}
				case OP_DOUBLE_EQUAL:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (r_cast<double>(&R[A]) == r_cast<double>(&R[B])));
					break;
				}
				case OP_INT_TO_FLOAT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, (static_cast<float>(static_cast<int64_t>(R[A]))));
					break;
				}
				case OP_FLOAT_TO_INT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<uint64_t>(static_cast<int64_t>(static_cast<float>(R[A]))));
					break;
				}
				case OP_FLOAT_TO_DOUBLE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<double>(*reinterpret_cast<float*>(&R[A])));
					break;
				}
				case OP_DOUBLE_TO_FLOAT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<float>(*reinterpret_cast<double*>(&R[A])));
					break;
				}
				case OP_INT_TO_DOUBLE:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<double>(static_cast<int64_t>(R[A])));
					break;
				}
				case OP_DOUBLE_TO_INT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<uint64_t>(static_cast<int64_t>(static_cast<double>(R[A]))));
					break;
				}
				case OP_BITWISE_AND:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					uint8_t flags = rFlags[C];
					setRegister(C, R[A] & R[B]);
					rFlags[C] = flags;
					break;
				}
				case OP_BITWISE_OR:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					uint8_t flags = rFlags[C];
					setRegister(C, R[A] | R[B]);
					rFlags[C] = flags;
					break;
				}
				case OP_LOGICAL_AND:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					bool isATruthy = isTruthy(A);
					bool isBTruthy = isTruthy(B);

					setRegister(C, comparisonRegister = isATruthy && isBTruthy);
					break;
				}
				case OP_LOGICAL_OR:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					bool isATruthy = isTruthy(A);
					bool isBTruthy = isTruthy(B);

					setRegister(C, comparisonRegister = isATruthy || isBTruthy);
					break;
				}
				case OP_LOGICAL_NOT:
				{
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					bool isATruthy = isTruthy(A);

					setRegister(B, comparisonRegister = !isATruthy);
					break;
				}
				case OP_STORE_IP_OFFSET:
				{
					uint8_t A = RegisterA(instruction);
					uint64_t temp = ip - chunk->code();
					setRegister(A, temp);
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

	Allocation* VM::allocate(uint64_t typeID)
	{
#ifdef STRESSTEST_GC
			collectGarbage();
#else
			//TODO: find a heuristic for calling the garbage collector
#endif
		TypeMetadata* typeInfo = types[typeID].get();
		
		size_t dataSize = typeInfo->fields.back().offset + util::fieldSize(typeInfo->fields.back().type);
		size_t size = 0;
		size = 10 + (size_t)typeInfo->spacing + dataSize; //8 bytes TypeMetadata*, 1 byte for spacing offset, 1 byte for refcount
		if (size % sizeof(void*))
		{
			size /= sizeof(void*);
			size += 1;
			size *= sizeof(void*);
		}
		void* result = malloc(size);
		if (result == nullptr) exit(1);
		memset(result, 0, size);
		auto typePtr = (TypeMetadata**)result;
		*typePtr = typeInfo;
		auto spaceInfo = (uint8_t*)result + 8;
		*spaceInfo = typeInfo->spacing;
		auto refCount = (uint8_t*)result + 9;
		*refCount = 1;
		Allocation* allocation = new TypeAllocation();
		allocation->memory = static_cast<char*>(result);
		allocation->next = allocationList;
		if(allocationList) allocationList->previous = allocation;
		allocation->size = size;
		allocationList = allocation;
		return allocation;
	}

	Allocation* VM::allocateArray(ArrayAllocation* pointer, size_t oldCount, size_t newCount, uint8_t fieldType)
	{
		if (newCount > oldCount)
#ifdef STRESSTEST_GC
			collectGarbage();
#else
			//TODO: find a heuristic for calling the garbage collector
#endif
		int64_t padding = (int64_t)(fieldType & 0x7F) - 2;
		if (fieldType & 0x80) padding = alignof(void*) - 2;
		if (padding < 0) padding = 0;
		size_t oldSize = 0;
		if (pointer)
		{
		
			uint8_t* oldArray = (uint8_t*)pointer;
			fieldType = *oldArray;
			oldSize = OBJECT_BEGIN_OFFSET + padding + (oldCount * (fieldType & 0x7F)); //8 bytes for capacity, 1 byte for span, 1 byte for refcount
		}
		size_t newSize = OBJECT_BEGIN_OFFSET + padding + (newCount * (fieldType & 0x7F)); // 8 bytes for capacity, 1 byte for span, 1 byte for refcount
		
		if (newSize % sizeof(void*))
		{
			newSize /= sizeof(void*);
			newSize += 1;
			newSize *= sizeof(void*);
		}

		void* result = realloc(pointer, newSize);
		if (result == nullptr) exit(1);
		memset((void*)(((char*)result) + oldSize), 0, newSize - oldSize);
		uint64_t* count = reinterpret_cast<uint64_t*>(result);
		*count = newCount;
		uint8_t* arraySpan = ((uint8_t*)result) + 8;
		*arraySpan = fieldType;
		uint8_t* refCount = ((uint8_t*)result + 9);
		*refCount = 1;
		Allocation* allocation = new ArrayAllocation();
		allocation->memory = (char*)result;
		allocation->next = allocationList;
		if(allocationList) allocationList->previous = allocation;
		allocation->size = newSize;
		allocationList = allocation;
		return allocation;
	}


	void VM::freeAllocation(Allocation* alloc)
	{
		switch (alloc->type())
		{
			case AllocationType::Type:
			{
				TypeAllocation* typeAlloc = (TypeAllocation*)alloc;
				char* mem = typeAlloc->memory;
				TypeMetadata* metadata = (TypeMetadata*)typeAlloc->memory;
				size_t currentOffset = OBJECT_BEGIN_OFFSET + (size_t)metadata->spacing;
				for (const auto field : metadata->fields)
				{
					if (field.type == FieldType::Struct || field.type == FieldType::Array)
					{
						Allocation* fieldObject = (Allocation*)(mem + currentOffset);
						if (fieldObject != nullptr)
							refDecrement(fieldObject);
					}
					currentOffset += (util::fieldSize(field.type));
				}
				break;
			}
			case AllocationType::Array:
			{
				ArrayAllocation* arrayAlloc = (ArrayAllocation*)alloc;
				uint8_t span = (*(arrayAlloc->memory + ARRAY_TYPE_OFFSET) & 0x7F);
				uint8_t spacing = (span - 2) * (span - 2 > 0);
				char* indexZero = arrayAlloc->memory + OBJECT_BEGIN_OFFSET + spacing;
				bool nonbasic = (*(arrayAlloc->memory + ARRAY_TYPE_OFFSET) & 0x80);
				if (nonbasic)
				{
					for (size_t i = 0; i < *(size_t*)arrayAlloc->memory; i++)
					{
						Allocation* ref = *(Allocation**)(indexZero + (span * i));
						if (ref != nullptr)
							refDecrement(ref);
					}
				}
			}
		}
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
		Allocation* ptr = allocationList;
		while (ptr)
		{
			uint8_t* refCount = (uint8_t*)(ptr->memory + REFCOUNT_OFFSET);
			*refCount = 0;
			ptr = ptr->next;
		}

#ifdef LOG_GC
		std::cout << "> marking registers" << std::endl;
#endif
		std::queue<Allocation*> greyset;
		for (int i = 0; i < R.size(); i++)
		{
			if ((rFlags[i] & REGISTER_HOLDS_POINTER) != 0)
			{
				auto alloc = reinterpret_cast<Allocation*>(R[i]);
				refIncrement(alloc);
				greyset.push(alloc);
			}
		}
#ifdef LOG_GC
		std::cout << "> marking stack" << std::endl;
#endif
		for (int i = 0; i < stackPointers.size(); i++)
		{
			auto alloc = reinterpret_cast<Allocation*>(stack[stackPointers[i]]);
			refIncrement(alloc);
			greyset.push(alloc);
		}
#ifdef LOG_GC
		std::cout << "> marking from roots" << std::endl;
#endif
		while(greyset.size())
		{
			auto current = greyset.front();
			switch (current->type())
			{
				case AllocationType::Array:
				{
					uint8_t span = *(uint8_t*)(current->memory + ARRAY_TYPE_OFFSET);
					if (span & 0x80)
					{
						size_t size = *(uint64_t*)current->memory;
						uint8_t spacing = sizeof(Allocation*) - 2;
						auto mem = current->memory + OBJECT_BEGIN_OFFSET + spacing;
						for (int i = 0; i < size; i++)
						{
							Allocation* ptr = *(Allocation**)(mem + sizeof(Allocation*) * i);
							if (ptr)
							{
								refIncrement(ptr);
								greyset.push(ptr);
							}
						}
					}
					break;
				}
				case AllocationType::Type:
				{
					TypeMetadata* metadata = (TypeMetadata*)current->memory;
					if (metadata == nullptr)
					{
						throw std::runtime_error("Invalid type object!");
					}
					size_t offset = 0;
					uint8_t spacing = *(uint8_t*)(current->memory + STRUCT_SPACING_OFFSET);
					auto mem = current->memory + OBJECT_BEGIN_OFFSET + spacing;
					for (const auto& field : metadata->fields)
					{
						if (field.type == FieldType::Struct || field.type == FieldType::Array)
						{
							Allocation* ptr = *(Allocation**)(mem + offset);
							if (ptr)
							{
								refIncrement(ptr);
								greyset.push(ptr);
							}
						}
						offset += util::fieldSize(field.type);
					}
					break;
				}
			}
			greyset.pop();
		}

#ifdef LOG_GC
		std::cout << "> begin sweep" << std::endl;
#endif
		Allocation* alloc = allocationList;
		while (alloc != nullptr)
		{
			if(refCount(alloc) == 0)
			{
				auto white = alloc;
				alloc = alloc->next;
				if (alloc) alloc->previous = white->previous;
				freeAllocation(white);
				if (white->previous == nullptr)
				{
					allocationList = alloc;
				}
				else
				{
					white->previous->next = alloc;
				}
			}
			alloc = alloc->next;
		}

#ifdef LOG_GC
		std::cout << "> end sweep" << std::endl;
#endif

#ifdef LOG_GC
		std::cout << "> end gc" << std::endl;
#endif
	}

	void VM::setRegister(uint8_t _register, bool value)
	{
		setRegister(_register, static_cast<uint64_t>(value));
	}

	void VM::setRegister(uint8_t _register, uint64_t value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(reinterpret_cast<Allocation*>(R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		R[_register] = value;
	}

	void VM::setRegister(uint8_t _register, int64_t value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(reinterpret_cast<Allocation*>(R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= ((REGISTER_HOLDS_SIGNED) * (value < 0));
		R[_register] = value;
	}

	void VM::setRegister(uint8_t _register, float value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(reinterpret_cast<Allocation*>(R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= REGISTER_HOLDS_FLOAT;
		R[_register] = *reinterpret_cast<uint64_t*>(&value);
	}
			
	void VM::setRegister(uint8_t _register, double value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(reinterpret_cast<Allocation*>(R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= REGISTER_HOLDS_DOUBLE;
		R[_register] = *reinterpret_cast<uint64_t*>(&value);
	}

	void VM::setRegister(uint8_t _register, Allocation* value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(reinterpret_cast<Allocation*>(R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= REGISTER_HOLDS_POINTER;
		if (((Allocation*)value)->type() == AllocationType::Array)
		{
			rFlags[_register] |= REGISTER_HOLDS_ARRAY;
		}
		R[_register] = *reinterpret_cast<uint64_t*>(&value);
	}

	void VM::refIncrement(Allocation* ref)
	{
		uint8_t* refCount = (uint8_t*)(ref->memory + REFCOUNT_OFFSET);
		if (*refCount == 255) return;
		(*refCount)++;
	}

	void VM::refDecrement(Allocation* ref)
	{
		uint8_t* refCount = (uint8_t*)(ref->memory + REFCOUNT_OFFSET);
		if (*refCount == 255) return;
		if (*refCount == 0 || *refCount == 1)
		{
			freeAllocation(ref);
			return;
		}
		(*refCount)--;
	}

	uint8_t VM::refCount(Allocation* ref)
	{
		return *(uint8_t*)(ref->memory + REFCOUNT_OFFSET);
	}
}