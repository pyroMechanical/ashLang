#pragma once

#include <stdlib.h>
#include <vector>
#include <map>
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


	struct FieldMetadata
	{
		FieldType type;
		size_t offset = 0;
		int64_t typeID = 0; //location within the typemetadata array
	};

	struct TypeMetadata
	{
		TypeMetadata* parent;
		std::vector<FieldMetadata> fields;
	};

	enum class AllocationType
	{
		Type, Array
	};

	struct Allocation
	{
		char* memory;
		Allocation* next;
		Allocation* previous;
		uint8_t exp;
		//TypeMetadata* typeInfo;
		//uint32_t refCount;

		virtual AllocationType type() = 0;
		size_t size() { return (size_t)1 << exp; }
	};

	struct TypeAllocation : public Allocation
	{
		virtual AllocationType type() override { return AllocationType::Type; }
	};

	struct ArrayAllocation : public Allocation
	{
		virtual AllocationType type() override { return AllocationType::Array; }
	};

	class MemBlock
	{
	public:
		const void* begin;
		std::map<void*, Allocation*> freeSet;
		size_t size() { return (size_t)1 << exp; }

	private:
		uint8_t exp; //size is 1<<exp
	};
}