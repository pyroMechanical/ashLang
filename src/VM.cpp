#include "VM.h"
#include "Compiler.h"

#include <iostream>
#include <bitset>

#define STRESSTEST_GC
//#def LOG_GC

namespace ash
{
	namespace util
	{
		inline static uint8_t read_byte(uint8_t*& ip)
		{
			return *ip++;
		}
		inline static uint64_t read_bytes(uint8_t*& ip, uint8_t bytes)
		{
			uint64_t result = 0;
			for (uint8_t i = 0; i < bytes; i++)
			{
				result = result << 8;
				result += read_byte(ip);
			}

			return result;
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

		return InterpretResult::INTERPRET_COMPILE_ERROR;
	}

	InterpretResult VM::interpret(Chunk* chunk)
	{
		this->chunk = chunk;
		ip = chunk->code();

		return run();
	}

	InterpretResult VM::run()
	{
		using namespace util;

		while(true)
		{
			uint8_t instruction = read_byte(ip);
			switch (instruction)
			{
				case OP_MOVE:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					R[B] = R[A];
					rFlags[B] = rFlags[A];
					break;
				}
				case OP_ALLOC:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					size_t toAllocate = R[A];
					void* alloc = allocate(nullptr, 0, toAllocate);
					R[B] = reinterpret_cast<uint64_t>(alloc);
					rFlags[B] |= REGISTER_HOLDS_POINTER;
					break;
				}
				case OP_CONST:
				{
					uint8_t A = read_byte(ip);
					uint64_t value = read_bytes(ip, 4);
					R[A] = value;
					rFlags[A] = 0;
					break;
				}
				case OP_CONST_LONG:
				{
					uint8_t A = read_byte(ip);
					uint64_t value = read_bytes(ip, 8);
					R[A] = value;
					rFlags[A] = 0;
					break;
				}
				case OP_STORE8:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory);
					*addr = R[A];
					break;
				}
				case OP_STORE16:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory);
					*addr = R[A];
					break;
				}
				case OP_STORE32:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory);
					*addr = R[A];
					break;
				}
				case OP_STORE64:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory);
					*addr = R[A];
					break;
				}
				case OP_LOAD8:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_LOAD16:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_LOAD32:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_LOAD64:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory);
					R[A] = *addr;
					break;
				}
				case OP_STORE8_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory + R[C]);
					*addr = R[A];
					break;
				}
				case OP_STORE16_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory + R[C]);
					*addr = R[A];
					break;
				}
				case OP_STORE32_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");

					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory + R[C]);
					*addr = R[A];
					break;
				}
				case OP_STORE64_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory + R[C]);
					*addr = R[A];
					break;
				}
				case OP_LOAD8_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint8_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_LOAD16_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint16_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_LOAD32_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint32_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_LOAD64_OFFSET:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					if ((rFlags[B] & REGISTER_HOLDS_POINTER) == 0) return error("register not a memory address!");
					
					auto alloc = reinterpret_cast<Allocation*>(R[B]);
					auto addr = reinterpret_cast<uint64_t*>(alloc->memory + R[C]);
					R[A] = *addr;
					break;
				}
				case OP_PUSH:
				{
					uint8_t A = read_byte(ip);
					stack.push_back(R[A]);
					if ((rFlags[A] & REGISTER_HOLDS_POINTER) != 0) stackPointers.push_back(stack.size() - 1);
					rFlags[A] = 0;
					break;
				}
				case OP_POP:
				{
					uint8_t A = read_byte(ip);
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
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] + R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_INT_SUB:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] - R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_UNSIGN_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] * R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_UNSIGN_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] / R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_UNSIGN_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] < R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_INT_EQUAL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] == R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_UNSIGN_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] > R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_SIGN_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = static_cast<uint64_t>((static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					rFlags[C] = 0;
					break;
				}
				case OP_SIGN_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = static_cast<uint64_t>((static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					rFlags[C] = 0;
					break;
				}
				case OP_SIGN_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = (static_cast<int64_t>(R[A]) < static_cast<int64_t>(R[B]));
					rFlags[C] = 0;
					break;
				}
				case OP_SIGN_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = (static_cast<int64_t>(R[A]) > static_cast<int64_t>(R[B]));
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_ADD:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) + r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_SUB:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) - r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) * r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) / r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) < r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) > r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_FLOAT_EQUAL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) == r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_ADD:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) + r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_SUB:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) - r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) * r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) / r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) < r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) > r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}
				case OP_DOUBLE_EQUAL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) == r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					rFlags[C] = 0;
					break;
				}

				case OP_INT_TO_FLOAT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					float temp = static_cast<float>(static_cast<int64_t>(R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] = 0;
					break;
				}
				case OP_FLOAT_TO_INT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					R[B] = static_cast<uint64_t>(static_cast<int64_t>(static_cast<float>(R[A])));
					rFlags[B] = 0;
					break;
				}
				case OP_FLOAT_TO_DOUBLE:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					double temp = static_cast<double>(r_cast<float>(&R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] = 0;
					break;
				}
				case OP_DOUBLE_TO_FLOAT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					float temp = static_cast<float>(r_cast<double>(&R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] = 0;
					break;
				}
				case OP_INT_TO_DOUBLE:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					double temp = static_cast<double>(static_cast<int64_t>(R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					rFlags[B] = 0;
					break;
				}
				case OP_DOUBLE_TO_INT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					R[B] = static_cast<uint64_t>(static_cast<int64_t>(static_cast<double>(R[A])));
					rFlags[B] = 0;
					break;
				}
				case OP_AND:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] & R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_OR:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] | R[B];
					rFlags[C] = 0;
					break;
				}
				case OP_RETURN: 
				{
					std::cout << "bits: " << std::bitset<64>(R[0]) << std::endl;
					std::cout << "boolean: " << ((std::bitset<64>(R[0]).count() > 0) ? "true" : "false") << std::endl;
					std::cout << "unsigned integer: " << R[0] << std::endl;
					std::cout << "signed integer: " << static_cast<int64_t>(R[0]) << std::endl;
					std::cout << "float: " << r_cast<float>(&R[0]) << std::endl;
					std::cout << "double: " << r_cast<double>(&R[0]) << std::endl;

					return InterpretResult::INTERPRET_OK;
				}
			}
		}
	}

	void* VM::allocate(void* pointer, size_t oldSize, size_t newSize)
	{
		if (newSize > oldSize)
#ifdef STRESSTEST_GC
			collectGarbage();
#endif

		if (newSize == 0)
		{
			free(pointer);
			return nullptr;
		}

		void* result = realloc(pointer, newSize);
		memset(result, 0, newSize);
		if (result == nullptr) exit(1);
		Allocation* allocation = new Allocation();
		allocation->memory = static_cast<char*>(result);
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