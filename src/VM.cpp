#include "VM.h"
#include "Compiler.h"

#include <iostream>
#include <bitset>
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
	}

	VM::VM()
	{
		
	}

	VM::~VM()
	{
		mem.freeAllocations();
	}

	InterpretResult VM::interpret(std::string source)
	{
		Compiler compiler;

		compiler.compile(source.c_str());

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
					break;
				}
				case OP_ALLOC:
				{
					uint8_t A = read_byte(ip);
					uint64_t toAllocate = R[A];
					//size_t newSize = heap.size() + toAllocate;
					//heap.resize(toAllocate);
				}
				case OP_CONST:
				{
					uint8_t A = read_byte(ip);
					uint64_t value = read_bytes(ip, 4);
					R[A] = value;
					break;
				}
				case OP_CONST_LONG:
				{
					uint8_t A = read_byte(ip);
					uint64_t value = read_bytes(ip, 8);
					R[A] = value;
					break;
				}
				case OP_STORE:
				{
					//TODO: rewrite op_store
					break;
				}
				case OP_LOAD:
				{
					//TODO: rewrite op_load
					break;
				}
				case OP_INT_ADD:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] + R[B];
					break;
				}
				case OP_INT_SUB:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] - R[B];
					break;
				}
				case OP_UNSIGN_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] * R[B];
					break;
				}
				case OP_UNSIGN_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] / R[B];
					break;
				}
				case OP_UNSIGN_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] < R[B];
					break;
				}
				case OP_INT_EQUAL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] == R[B];
					break;
				}
				case OP_UNSIGN_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = R[A] > R[B];
					break;
				}
				case OP_SIGN_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = static_cast<uint64_t>((static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));

					break;
				}
				case OP_SIGN_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = static_cast<uint64_t>((static_cast<int64_t>(R[A]) * static_cast<int64_t>(R[B])));
					break;
				}
				case OP_SIGN_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = (static_cast<int64_t>(R[A]) < static_cast<int64_t>(R[B]));
					break;
				}
				case OP_SIGN_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					R[C] = (static_cast<int64_t>(R[A]) > static_cast<int64_t>(R[B]));
					break;
				}
				case OP_FLOAT_ADD:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) + r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_SUB:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) - r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) * r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) / r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) < r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) > r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_EQUAL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					float temp = (r_cast<float>(&R[A]) == r_cast<float>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_ADD:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) + r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_SUB:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) - r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_MUL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) * r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_DIV:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) / r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_LESS:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) < r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_GREATER:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) > r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_EQUAL:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);
					double temp = (r_cast<double>(&R[A]) == r_cast<double>(&R[B]));
					R[C] = r_cast<uint64_t>(&temp);
					break;
				}

				case OP_INT_TO_FLOAT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					float temp = static_cast<float>(static_cast<int64_t>(R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_FLOAT_TO_INT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					R[B] = static_cast<uint64_t>(static_cast<int64_t>(static_cast<float>(R[A])));
					break;
				}
				case OP_FLOAT_TO_DOUBLE:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					double temp = static_cast<double>(r_cast<float>(&R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_TO_FLOAT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					float temp = static_cast<float>(r_cast<double>(&R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_INT_TO_DOUBLE:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					double temp = static_cast<double>(static_cast<int64_t>(R[A]));
					R[B] = r_cast<uint64_t>(&temp);
					break;
				}
				case OP_DOUBLE_TO_INT:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					R[B] = static_cast<uint64_t>(static_cast<int64_t>(static_cast<double>(R[A])));
					break;
				}
				case OP_AND:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);

					R[C] = R[A] & R[B];
					break;
				}
				case OP_OR:
				{
					uint8_t A = read_byte(ip);
					uint8_t B = read_byte(ip);
					uint8_t C = read_byte(ip);

					R[C] = R[A] | R[B];
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
}