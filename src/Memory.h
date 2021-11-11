#pragma once

#include <stdlib.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

namespace ash
{
	enum class FieldType : uint8_t
	{
		Struct,
		Array,
		Bool,
		Byte,
		UByte,
		Short,
		UShort,
		Int,
		UInt,
		Long,
		ULong,
		Float,
		Double,
		Char
	};

	namespace util
	{
		static uint8_t fieldSize(FieldType type)
		{
			switch(type)
			{
			case FieldType::Struct:
			case FieldType::Array: return sizeof(void*);
			case FieldType::Bool:
			case FieldType::Byte:
			case FieldType::UByte: return 1;
			case FieldType::Short:
			case FieldType::UShort: return 2;
			case FieldType::Int:
			case FieldType::UInt:
			case FieldType::Char:
			case FieldType::Float:return 4;
			case FieldType::Long:
			case FieldType::ULong:
			case FieldType::Double: return 8;
			}
			return 0;
		}
	}

	class MemBlock
	{
		//TODO: support blocks for allocations larger than 256 bytes
	private:
		uint8_t exp; //individual allocation size is 2 ^ exp
		//currently, capacity is 4096/(1<<exp) bytes
		uint32_t count;
		void* begin;
		std::vector<void*> freeList;
	public:
		MemBlock(uint8_t powerOfTwo);
		~MemBlock();
		void const* const firstAddress() { return (char*)begin; }
		const size_t size() { return (1 << exp) * count; }
		const uint8_t exponent() { return exp; }
		void* alloc() 
		{
			auto addr = freeList.back(); freeList.pop_back(); return addr;
		}
		void free(void* ptr) { freeList.push_back(ptr); }
		bool notFull() { return freeList.size() > 0; }
		bool isEmpty() { return freeList.size() == count; }
		bool operator== (MemBlock& other)
		{
			return (other.begin == begin &&
				other.exp == exp);
		}
		bool operator!= (MemBlock& other)
		{
			return !(*this == other);
		}
	};

	class Memory
	{
	private:
		static std::unordered_map<uint8_t, std::vector<MemBlock>> memBlockLists;
		static std::unordered_map<void*, MemBlock&> memBlocks;
	public:
		Memory();
		~Memory();
		const static uint8_t minExponentSize = 4;
		static void* allocate(uint8_t powerOfTwo);
		static void* allocate(uint8_t powerOfTwo, std::vector<uint8_t>& bitFlags);
		static void freeAllocation(void* ptr);
	};
}