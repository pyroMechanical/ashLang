#pragma once

#include <stdlib.h>
#include <vector>
#include <multimap>
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
		void* begin;
		std::vector<void*> freeList;
	public:
		MemBlock(uint8_t powerOfTwo);
		~MemBlock();
		void const* const getBegin() { return begin; }
	};

	class Memory
	{
	private:
		static std::multimap<uint8_t, MemBlock> memBlocks;
	public:
		const static uint8_t minExponentSize = 4;
		static void* allocate(uint8_t powerOfTwo);
		static void allocateList(uint8_t count);
		static void freeAllocation(void* ptr);
	};
}