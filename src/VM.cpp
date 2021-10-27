#include "VM.h"
#include "Compiler.h"
#include "Debug.h"
#include "Timer.h"

#include <iostream>
#include <queue>
#include <bitset>
#include <typeinfo>
#include <typeindex>
#include <chrono>

//#define STRESSTEST_GC
//#define LOG_GC
//#define LOG_TIMES


namespace ash
{
	namespace util
	{
		inline static uint8_t ilog2(size_t i)
		{
#if defined(_MSC_VER)
			unsigned long index;
			_BitScanReverse64(&index, i);
			return (uint8_t)index;
#elif defined(__GNUC__) || defined(__GNUG__)
			return (uint8_t)31 - __builtin_clzll(i);
#else
			//i |= (i >> 1);
			//i |= (i >> 2);
			//i |= (i >> 4);
			//i |= (i >> 8);
			//i |= (i >> 16);
			//i |= (i >> 32);
			//return (i - (i >> 1));
#endif
		}

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
		std::shared_ptr<Chunk> chunk;
		{
			Compiler compiler;

			bool compileSuccess = compiler.compile(source.c_str());

			if (!compileSuccess || !compiler.getChunk()->size()) return InterpretResult::INTERPRET_COMPILE_ERROR;
			types = compiler.getTypes();
			chunk = compiler.getChunk();
		}

		//Disassembler debug;
		//debug.disassembleChunk(chunk.get(), "generated chunk");
		InterpretResult result = interpret(chunk.get());

		return result;
	}

	InterpretResult VM::interpret(Chunk* chunk)
	{
		this->chunk = chunk;
		ip = chunk->code();
		//this->types = chunk->types;
		auto t1 = std::chrono::high_resolution_clock::now();
		InterpretResult result = run();
		auto t2 = std::chrono::high_resolution_clock::now();

		std::cout << "Execution took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
		return result;
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
					#ifdef LOG_TIMES
					Timer t = { "OP_MOVE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					if (rFlags[B] & REGISTER_HOLDS_POINTER) refDecrement(reinterpret_cast<Allocation*>(R[B]));
					setRegister(B, R[A]);
					if (rFlags[A] & REGISTER_HOLDS_SIGNED)
					{
						setRegister(B, static_cast<int64_t>(R[A]));
					}
					else if (rFlags[A] & REGISTER_HOLDS_FLOAT)
					{
						setRegister(B, *reinterpret_cast<float*>(&R[A]));
					}
					else if (rFlags[A] & REGISTER_HOLDS_DOUBLE)
					{
						setRegister(B, *reinterpret_cast<double*>(&R[A]));
					}
					else if (rFlags[A] & REGISTER_HOLDS_POINTER)
					{
						setRegister(B, reinterpret_cast<Allocation*>(R[A]));
					}
					else
					{
						setRegister(B, R[A]);
					}
					break;
				}
				case OP_NEW_STACK_FRAME:
				{
#ifdef LOG_TIMES
					Timer t = { "OP_NEW_STACK_FRAME" };
#endif
					stack.push_back(R[FRAME_REGISTER]);
					stackFlags.push_back(0);
					R[FRAME_REGISTER] = stack.size();
					break;
				}
				case OP_ALLOC:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_ALLOC" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint64_t typeID = R[A];
					Allocation* alloc = allocate(typeID);
					setRegister(B, alloc);
					break;
				}
				case OP_ALLOC_ARRAY:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_ALLOC_ARRAY" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_CONST_LOW" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint64_t value = Value(instruction);
					setRegister(A, value);
					break;
				}
				case OP_CONST_LOW_NEGATIVE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_CONST_LOW_NEGATIVE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint64_t value = Value(instruction) | 0xFFFFFFFFFFFF0000;
					setRegister(A, value);
				}
				case OP_CONST_MID_LOW:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_CONST_MID_LOW" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					setRegister(A, (R[A] & 0xFFFFFFFF0000FFFF) + (((uint64_t)value) <<16));
					break;
				}
				case OP_CONST_MID_HIGH:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_CONST_MID_HIGH" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					setRegister(A, (R[A] & 0xFFFF0000FFFFFFFF) + (((uint64_t)value) << 32));
					break;
				}
				case OP_CONST_HIGH:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_CONST_HIGH" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint16_t value = Value(instruction);
					setRegister(A,(R[A] & 0x0000FFFFFFFFFFFF) + (((uint64_t)value) << 48));
					break;
				}
				case OP_STORE_OFFSET:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_STORE_OFFSET" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					TypeMetadata* metadata = alloc->typeInfo;
					if (R[C] >= metadata->fields.size()) return error("field out of bounds!");
					size_t offset = metadata->fields[R[C]].offset;
					FieldType type = metadata->fields[R[C]].type;
					if (rFlags[A] & REGISTER_HOLDS_POINTER)
					{
						Allocation* ref = *reinterpret_cast<Allocation**>(&R[A]);
						refIncrement(ref);
					}
					if (type == FieldType::Array || type == FieldType::Struct)
					{
						Allocation* ref = *reinterpret_cast<Allocation**>(alloc->memory + offset);
						if(ref)
							refDecrement(ref);
					}
					switch (fieldSize(type))
					{
						case 1:
						{
							auto addr = reinterpret_cast<uint8_t*>(alloc->memory + offset);
							*addr = static_cast<uint8_t>(R[A]);
							break;
						}
						case 2:
						{
							auto addr = reinterpret_cast<uint16_t*>(alloc->memory + offset);
							*addr = static_cast<uint16_t>(R[A]);
							break;
						}
						case 4:
						{
							auto addr = reinterpret_cast<uint32_t*>(alloc->memory + offset);
							*addr = static_cast<uint32_t>(R[A]);
							break;
						}
						case 8:
						{
							auto addr = reinterpret_cast<uint64_t*>(alloc->memory + offset);
							*addr = static_cast<uint64_t>(R[A]);
							break;
						}
					}
					break;
				}
				case OP_LOAD_OFFSET:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_LOAD_OFFSET" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto spacing = *(alloc->memory);
					TypeMetadata* metadata = alloc->typeInfo;
					if (R[C] >= metadata->fields.size()) return error("field out of bounds!");
					size_t offset = metadata->fields[R[C]].offset;
					FieldType type = metadata->fields[R[C]].type;
					switch (type)
					{
						case FieldType::Bool:
						case FieldType::UByte:
						{
							auto addr = reinterpret_cast<uint8_t*>(alloc->memory + offset);
							setRegister(A, (uint64_t)*addr);
							break;
						}
						case FieldType::UShort:
						{
							auto addr = reinterpret_cast<uint16_t*>(alloc->memory + offset);
							setRegister(A, (uint64_t)*addr);
							break;
						}

						case FieldType::UInt:
						{
							auto addr = reinterpret_cast<uint32_t*>(alloc->memory + offset);
							setRegister(A, (uint64_t)*addr);
							break;
						}
						case FieldType::ULong:
						{
							auto addr = reinterpret_cast<uint64_t*>(alloc->memory + offset);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Byte:
						{
							auto addr = reinterpret_cast<int8_t*>(alloc->memory + offset);
							setRegister(A, (int64_t)*addr);
							break;
						}
						case FieldType::Short:
						{
							auto addr = reinterpret_cast<int16_t*>(alloc->memory + offset);
							setRegister(A, (int64_t)*addr);
							break;
						}
						case FieldType::Int:
						{
							auto addr = reinterpret_cast<int32_t*>(alloc->memory + offset);
							setRegister(A, (int64_t)*addr);
							break;
						}
						case FieldType::Long:
						{
							auto addr = reinterpret_cast<int64_t*>(alloc->memory + offset);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Float:
						{
							auto addr = reinterpret_cast<float*>(alloc->memory + offset);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Double:
						{
							auto addr = reinterpret_cast<double*>(alloc->memory + offset);
							setRegister(A, *addr);
							break;
						}
						case FieldType::Array:
						{
							auto addr = reinterpret_cast<Allocation**>(alloc->memory + offset);
							setRegister(A, *addr);
							rFlags[A] &= REGISTER_HIGH_BITS;
							rFlags[A] |= REGISTER_HOLDS_POINTER | REGISTER_HOLDS_ARRAY;
							break;
						}
						case FieldType::Struct:
						{
							auto addr = reinterpret_cast<Allocation**>(alloc->memory + offset);
							setRegister(A, *addr);
							rFlags[A] &= REGISTER_HIGH_BITS;
							rFlags[A] |= REGISTER_HOLDS_POINTER;
							break;
						}
					}
					break;
				}
				case OP_ARRAY_STORE:
					{
					#ifdef LOG_TIMES
					Timer t = { "OP_ARRAY_STORE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					if ((rFlags[B] & REGISTER_HOLDS_ARRAY) == 0) return error("pointer held in register is not an array!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					uint8_t span = (*((char*)alloc->memory)) & 0x7F;
					uint8_t spacing = (span - 2) * ((span - 2) > 0);
					uint64_t arrayCount = *reinterpret_cast<uint64_t*>(alloc->memory);
					if (R[C] >= arrayCount) return error("array index out of bounds!");
					uint64_t offset = span * R[C];
					switch (span)
					{
						case 1:
						{
							auto addr = reinterpret_cast<uint8_t*>(alloc->memory + spacing + offset);
							*addr = static_cast<uint8_t>(R[A]);
							break;
						}
						case 2:
						{
							auto addr = reinterpret_cast<uint16_t*>(alloc->memory + spacing + offset);
							*addr = static_cast<uint16_t>(R[A]);
							break;
						}
						case 4:
						{
							auto addr = reinterpret_cast<uint32_t*>(alloc->memory + spacing + offset);
							*addr = static_cast<uint32_t>(R[A]);
							break;
						}
						case 8:
						{
							auto addr = reinterpret_cast<uint64_t*>(alloc->memory + spacing + offset);
							*addr = R[A];
							break;
						}
					}
					break;
				}
				case OP_ARRAY_LOAD:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_ARRAY_LOAD" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					if ((rFlags[B] & REGISTER_HOLDS_ARRAY) == 0) return error("pointer held in register is not an array!");
					if (rFlags[A] & REGISTER_HOLDS_POINTER) refDecrement(reinterpret_cast<Allocation*>(R[A]));
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					uint8_t span = (*((char*)alloc->memory)) & 0x7F;
					uint8_t spacing = (span - 2) * ((span - 2) > 0);
					bool isPtr = (*((char*)alloc->memory)) & 0x80;
					uint64_t arrayCount = *reinterpret_cast<uint64_t*>(alloc->memory);
					if (R[C] >= arrayCount) return error("array index out of bounds!");
					uint64_t offset = span * R[C];
					switch (span)
					{
						case 1:
						{
							auto addr = reinterpret_cast<uint8_t*>(alloc->memory + spacing + offset);
							R[A] = *addr;
							break;
						}
						case 2:
						{
							auto addr = reinterpret_cast<uint16_t*>(alloc->memory + spacing + offset);
							R[A] = *addr;
							break;
						}
						case 4:
						{
							auto addr = reinterpret_cast<uint32_t*>(alloc->memory + spacing + offset);
							R[A] = *addr;
							break;
						}
						case 8:
						{
							auto addr = reinterpret_cast<uint64_t*>(alloc->memory + spacing + offset);
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
					#ifdef LOG_TIMES
					Timer t = { "OP_PUSH" };
					#endif
					uint8_t A = RegisterA(instruction);
					stack.push_back(R[A]);
					stackFlags.push_back(rFlags[A]);
					if ((rFlags[A] & REGISTER_HOLDS_POINTER) != 0)
					{
						stackPointers.push_back(stack.size() - 1);
						refIncrement(*reinterpret_cast<Allocation**>(&R[A]));
					}
					break;
				}
				case OP_POP:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_POP" };
					#endif
					uint8_t A = RegisterA(instruction);
					R[A] =  stack.back(); //TODO: clean up possible memory leak; overwriting register without setRegister();
					rFlags[A] = stackFlags.back();
					if (stackPointers.size() && stackPointers.back() == stack.size() - 1)
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
					#ifdef LOG_TIMES
					Timer t = { "OP_INT_ADD" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C,static_cast<int64_t>(R[A] + R[B]));
					break;
				}
				case OP_INT_SUB:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_INT_SUB" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, static_cast<int64_t>(R[A] - R[B]));
					break;
				}
				case OP_INT_NEGATE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_INT_NEGATE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					setRegister(B, -static_cast<int64_t>(R[A]));
					break;
				}
				case OP_UNSIGN_MUL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_UNSIGN_MUL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, R[A] * R[B]);
					break;
				}
				case OP_UNSIGN_DIV:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_UNSIGN_DIV" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, R[A] / R[B]);
					break;
				}
				case OP_UNSIGN_LESS:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_UNSIGN_LESS" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = R[A] < R[B]);
					break;
				}
				case OP_INT_EQUAL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_INT_EQUAL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = R[A] == R[B]);
					break;
				}
				case OP_UNSIGN_GREATER:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_UNSIGN_GREATER" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = R[A] > R[B]);
					break;
				}
				case OP_SIGN_MUL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_SIGN_MUL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_DIV:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_SIGN_DIV" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (static_cast<int64_t>(R[A]) / static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_LESS:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_SIGN_LESS" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					
					setRegister(C, comparisonRegister = (static_cast<int64_t>(R[A]) < static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_GREATER:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_SIGN_GREATER" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (static_cast<int64_t>(R[A]) > static_cast<int64_t>(R[B])));
					break;
				}
				case OP_FLOAT_ADD:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_ADD" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) + *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_SUB:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_SUB" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) - *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_MUL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_MUL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) * *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_DIV:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_DIV" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<float*>(&R[A]) / *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_NEGATE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_NEGATE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					setRegister(B, -*reinterpret_cast<float*>(&R[A]));
					break;
				}
				case OP_FLOAT_LESS:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_LESS" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (*reinterpret_cast<float*>(&R[A]) < *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_GREATER:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_GREATER" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (*reinterpret_cast<float*>(&R[A]) > *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_FLOAT_EQUAL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_EQUAL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (*reinterpret_cast<float*>(&R[A]) == *reinterpret_cast<float*>(&R[B])));
					break;
				}
				case OP_DOUBLE_ADD:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_ADD" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) + *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_SUB:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_SUB" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) - *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_MUL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_MUL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) * *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_DIV:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_DIV" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, (*reinterpret_cast<double*>(&R[A]) / *reinterpret_cast<double*>(&R[B])));
					break;
				}
				case OP_DOUBLE_NEGATE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_NEGATE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					setRegister(B, *reinterpret_cast<double*>(&R[A]));
					break;
				}
				case OP_DOUBLE_LESS:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_LESS" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (r_cast<double>(&R[A]) < r_cast<double>(&R[B])));
					break;
				}
				case OP_DOUBLE_GREATER:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_GREATER" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (r_cast<double>(&R[A]) > r_cast<double>(&R[B])));
					break;
				}
				case OP_DOUBLE_EQUAL:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_EQUAL" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					uint8_t C = RegisterC(instruction);
					setRegister(C, comparisonRegister = (r_cast<double>(&R[A]) == r_cast<double>(&R[B])));
					break;
				}
				case OP_INT_TO_FLOAT:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_INT_TO_FLOAT" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, (static_cast<float>(static_cast<int64_t>(R[A]))));
					break;
				}
				case OP_FLOAT_TO_INT:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_TO_INT" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<uint64_t>(static_cast<int64_t>(static_cast<float>(R[A]))));
					break;
				}
				case OP_FLOAT_TO_DOUBLE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_FLOAT_TO_DOUBLE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<double>(*reinterpret_cast<float*>(&R[A])));
					break;
				}
				case OP_DOUBLE_TO_FLOAT:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_TO_FLOAT" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<float>(*reinterpret_cast<double*>(&R[A])));
					break;
				}
				case OP_INT_TO_DOUBLE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_INT_TO_DOUBLE" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<double>(static_cast<int64_t>(R[A])));
					break;
				}
				case OP_DOUBLE_TO_INT:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_DOUBLE_TO_INT" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					setRegister(B, static_cast<uint64_t>(static_cast<int64_t>(static_cast<double>(R[A]))));
					break;
				}
				case OP_BITWISE_AND:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_BITWISE_AND" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_BITWISE_OR" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_LOGICAL_AND" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_LOGICAL_OR" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_LOGICAL_NOT" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);
					bool isATruthy = isTruthy(A);

					setRegister(B, comparisonRegister = !isATruthy);
					break;
				}
				case OP_PUSH_IP:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_PUSH_IP" };
					#endif
					stack.push_back((ip - chunk->code()) + 1);
					stackFlags.push_back(0);
					break;
				}
				case OP_RELATIVE_JUMP:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_RELATIVE_JUMP" };
					#endif
					int32_t jump = (int32_t)JumpOffset(instruction);
					if((ip - chunk->code()) + jump - 1 > static_cast<int64_t>(chunk->size()) || (ip - chunk->code()) + jump - 1 < 0) return error("attempted jump beyond code bounds!");
					ip += jump - 1;
					break;
				}
				case OP_RELATIVE_JUMP_IF_TRUE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_RELATIVE_JUMP_IF_TRUE" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_REGISTER_JUMP" };
					#endif
					uint8_t A = RegisterA(instruction);
					if (R[A] > chunk->size()) return error("attempted jump beyond code bounds!");
					ip = chunk->code() + R[A];
					break;
				}
				case OP_REGISTER_JUMP_IF_TRUE:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_REGISTER_JUMP_IF_TRUE" };
					#endif
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
					#ifdef LOG_TIMES
					Timer t = { "OP_OUT" };
					#endif
					uint8_t A = RegisterA(instruction);

					if ((rFlags[A] & REGISTER_HOLDS_SIGNED) != 0) std::cout << static_cast<int64_t>(R[A]) << std::endl;
					else if ((rFlags[A] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<float>(&R[A]) << std::endl;
					else if ((rFlags[A] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<double>(&R[A]) << std::endl;
					else std::cout << R[A] << std::endl;

					break;
				}
				case OP_MOVE_FROM_STACK_FRAME:
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_MOVE_FROM_STACK_FRAME" };
					#endif
					uint8_t A = RegisterA(instruction);
					uint8_t B = RegisterB(instruction);

					if (stack.size() <= (R[FRAME_REGISTER] + R[A])) return error("value is beyond stack size!");
					auto temp = stack[R[FRAME_REGISTER] + R[A]];
					if (stackFlags[R[FRAME_REGISTER] + R[A]] & REGISTER_HOLDS_SIGNED)
					{
						setRegister(B, static_cast<int64_t>(temp));
					}
					else if (stackFlags[R[FRAME_REGISTER] + R[A]] & REGISTER_HOLDS_FLOAT)
					{
						setRegister(B, *reinterpret_cast<float*>(&temp));
					}
					else if (stackFlags[R[FRAME_REGISTER] + R[A]] & REGISTER_HOLDS_DOUBLE)
					{
						setRegister(B, *reinterpret_cast<double*>(&temp));
					}
					else if (stackFlags[R[FRAME_REGISTER] + R[A]] & REGISTER_HOLDS_POINTER)
					{
						setRegister(B, reinterpret_cast<Allocation*>(temp));
					}
					else
					{
						setRegister(B, temp);
					}
					rFlags[B] = stackFlags[R[FRAME_REGISTER] + R[A]];
					break;
				}
				case OP_RETURN:
				{
#ifdef LOG_TIMES
					Timer t = { "OP_RETURN" };
#endif
					//if ((rFlags[RETURN_REGISTER] & REGISTER_HOLDS_SIGNED) != 0) std::cout << static_cast<int64_t>(R[RETURN_REGISTER]) << std::endl;
					//else if ((rFlags[RETURN_REGISTER] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<float>(&R[RETURN_REGISTER]) << std::endl;
					//else if ((rFlags[RETURN_REGISTER] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<double>(&R[RETURN_REGISTER]) << std::endl;
					//else std::cout << R[RETURN_REGISTER] << std::endl;
					auto addr = stack.back();
					stack.resize(R[FRAME_REGISTER]);
					stackFlags.resize(R[FRAME_REGISTER]);
					ip = chunk->code() + addr;
					break;
				}
				case OP_HALT: 
				{
					#ifdef LOG_TIMES
					Timer t = { "OP_HALT" };
					#endif

					if ((rFlags[RETURN_REGISTER] & REGISTER_HOLDS_SIGNED) != 0) std::cout << static_cast<int64_t>(R[RETURN_REGISTER]) << std::endl;
					else if ((rFlags[RETURN_REGISTER] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<float>(&R[RETURN_REGISTER]) << std::endl;
					else if ((rFlags[RETURN_REGISTER] & REGISTER_HOLDS_FLOAT) != 0) std::cout << r_cast<double>(&R[RETURN_REGISTER]) << std::endl;
					else std::cout << R[RETURN_REGISTER] << std::endl;
					R.fill(0);
					rFlags.fill(0);
					stack.clear();
					stackFlags.clear();
					stackPointers.clear();
					types.clear();
					comparisonRegister = false;
					freeAllocations();
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
		
		size_t size = typeInfo->fields.back().offset + util::fieldSize(typeInfo->fields.back().type);
		uint8_t exp;
		exp = util::ilog2(size) + 1;
		Allocation* result = Memory::allocate(exp);
		memset(result->memory, 0, (size_t)1<<exp);
		result->refCount = 0;
		result->right = allocationList;
		result->typeInfo = typeInfo;
		result->allocationType = AllocationType::Type;
		if (allocationList) allocationList->left = result;
		result->exp = (exp >= Memory::minExponentSize) ? exp : Memory::minExponentSize;
		allocationList = result;
		return result;
	}

	Allocation* VM::allocateArray(Allocation* pointer, size_t oldCount, size_t newCount, uint64_t typeID)
	{
		/*if (newCount > oldCount)
		{
#ifdef STRESSTEST_GC
			collectGarbage();
#else
			if (false) collectGarbage();
#endif
		}
		size_t oldSize = 0;
		if (pointer)
		{
			oldSize = (oldCount * (fieldType & 0x7F)); //8 bytes for capacity, 1 byte for span, 1 byte for refcount
		}
		size_t newSize = (newCount * (fieldType & 0x7F)); // 8 bytes for capacity, 1 byte for span, 1 byte for refcount
		
		uint8_t exp = util::ilog2(newSize) + 1;
		Allocation* result = Memory::allocate(exp);
		memset((void*)(((char*)result->memory) + oldSize), 0, (1<<exp) - oldSize);
		uint64_t* count = reinterpret_cast<uint64_t*>(result->memory);
		*count = newCount;
		uint8_t* arraySpan = ((uint8_t*)result) + 8;

		result->refCount = 1;
		Allocation* allocation = new Allocation();
		allocation->memory = (char*)result;
		allocation->right = allocationList;
		if(allocationList) allocationList->left = allocation;
		allocation->exp = exp;
		allocationList = allocation;
		return allocation;
		*/
		return pointer;
	}


	void VM::freeAllocation(Allocation* alloc)
	{
		switch (alloc->type())
		{
			case AllocationType::Type:
			{
				char* mem = alloc->memory;
				TypeMetadata* metadata = alloc->typeInfo;
				size_t currentOffset = 0;
				for (const auto& field : metadata->fields)
				{
					currentOffset = field.offset;
					if (field.type == FieldType::Struct || field.type == FieldType::Array)
					{
						Allocation* fieldObject = *(Allocation**)(mem + currentOffset);
						if (fieldObject != nullptr)
							refDecrement(fieldObject);
					}
				}
				break;
			}
			case AllocationType::Array:
			{
				//uint8_t span = (*(arrayAlloc->memory + ARRAY_TYPE_OFFSET) & 0x7F);
				//uint8_t spacing = (span - 2) * (span - 2 > 0);
				//char* indexZero = arrayAlloc->memory + OBJECT_BEGIN_OFFSET;
				//bool nonbasic = (*(arrayAlloc->memory + ARRAY_TYPE_OFFSET) & 0x80);
				//if (nonbasic)
				//{
				//	for (size_t i = 0; i < *(size_t*)arrayAlloc->memory; i++)
				//	{
				//		Allocation* ref = *(Allocation**)(indexZero + (span * i));
				//		if (ref != nullptr)
				//			refDecrement(ref);
				//	}
				//}
			}
		}
		Memory::free(alloc);
	}

	void VM::freeAllocations()
	{
		Allocation* allocation = allocationList;

		while (allocation != nullptr)
		{
			Allocation* next = allocation->right;
			Memory::free(allocation);
			allocation = next;
		}
		allocationList = nullptr;
	}

	void VM::collectGarbage()
	{
#ifdef LOG_GC
		std::cout << "> begin gc" << std::endl;
#endif
		Allocation* ptr = allocationList;
		while (ptr)
		{
			uint32_t* refCount = &ptr->refCount;
			*refCount = 0;
			ptr = ptr->right;
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
#ifdef LOG_GC
				std::cout << "adding " << static_cast<void*>(alloc) << " to greyset" << std::endl;
#endif
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
#ifdef LOG_GC
			std::cout << "adding " << static_cast<void*>(alloc) << " to greyset" << std::endl;
#endif
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
					uint8_t span = *(uint8_t*)(current->memory);
					if (span & 0x80)
					{
						size_t size = *(uint64_t*)current->memory;
						uint8_t spacing = sizeof(Allocation*) - 2;
						auto mem = current->memory + spacing;
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
					TypeMetadata* metadata = *(TypeMetadata**)current->memory;
					if (metadata == nullptr)
					{
						throw std::runtime_error("Invalid type object!");
					}
					size_t offset = 0;
					uint8_t spacing = *(uint8_t*)(current->memory);
					auto mem = current->memory;
					for (const auto& field : metadata->fields)
					{
						offset = field.offset;
						if (field.type == FieldType::Struct || field.type == FieldType::Array)
						{
							Allocation* ptr = *(Allocation**)(mem + offset);
							if (ptr)
							{
#ifdef LOG_GC
								std::cout << "adding " << static_cast<void*>(ptr) << " to greyset" << std::endl;
#endif
								refIncrement(ptr);
								greyset.push(ptr);
							}
						}
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
			if(alloc->refCount == 0)
			{
				auto white = alloc;
				alloc = alloc->right;
				if (alloc) alloc->left = white->left;
				if (white->left == nullptr)
				{
					allocationList = alloc;
				}
				else
				{
					white->left->right = alloc;
				}
				freeAllocation(white);
			}
			else alloc = alloc->right;
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
			refDecrement(*reinterpret_cast<Allocation**>(&R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		R[_register] = value;
	}

	void VM::setRegister(uint8_t _register, int64_t value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(*reinterpret_cast<Allocation**>(&R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= ((REGISTER_HOLDS_SIGNED) * (value < 0));
		R[_register] = value;
	}

	void VM::setRegister(uint8_t _register, float value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(*reinterpret_cast<Allocation**>(&R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= REGISTER_HOLDS_FLOAT;
		R[_register] = *reinterpret_cast<uint64_t*>(&value);
	}
			
	void VM::setRegister(uint8_t _register, double value)
	{
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			refDecrement(*reinterpret_cast<Allocation**>(&R[_register]));
		}

		rFlags[_register] &= REGISTER_HIGH_BITS;
		rFlags[_register] |= REGISTER_HOLDS_DOUBLE;
		R[_register] = *reinterpret_cast<uint64_t*>(&value);
	}

	void VM::setRegister(uint8_t _register, Allocation* value)
	{
		refIncrement(value);
		if (rFlags[_register] & (REGISTER_HOLDS_POINTER))
		{
			auto oldRef = *reinterpret_cast<Allocation**>(&R[_register]);
			refDecrement(oldRef);
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
#ifdef LOG_GC
		std::cout << "Incrementing " << static_cast<void*>(ref);
		std::cout << " to " << (ref->refCount) + 1 << std::endl;
#endif
		ref->refCount++;
	}

	void VM::refDecrement(Allocation* ref)
	{
#ifdef LOG_GC
		std::cout << "Decrementing " << static_cast<void*>(ref);
		std::cout << " to " << (ref->refCount) - 1 << std::endl;
#endif
		if (ref->refCount == 0 || ref->refCount == 1)
		{
			ref->refCount = 0;
			if (allocationList == ref)
			{
				allocationList = ref->right;
				if(ref->right) ref->right->left = nullptr;
			}
			else
			{
				if(ref->left)	ref->left->right = ref->right;
				if(ref->right) ref->right->left = ref->left;
			}
			freeAllocation(ref);
			return;
		}
		ref->refCount--;
	}
}