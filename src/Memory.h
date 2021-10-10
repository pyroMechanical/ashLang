#pragma once

#include <stdlib.h>
#include <vector>
#include <map>
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

	enum class AllocationType : uint8_t
	{
		Basic, Type, Array
	};

	struct Allocation
	{
		Allocation() = default;
		uint8_t exp = 0;
		bool isSplit = false;
		AllocationType allocationType = AllocationType::Basic;
		uint32_t refCount = 0;
		char* memory;
		Allocation* left;
		Allocation* right;
		TypeMetadata* typeInfo;

		AllocationType type() { return allocationType; };
		size_t size() { return (size_t)1 << exp; }
		bool free() { return refCount == 0; }
		void split();
		void merge();
	};

	class MemBlock
	{
	public:
		Allocation* root;
		size_t size() { return (size_t)1 << exp; }

	private:
		uint8_t exp; //size is 1<<exp
	};

	class Memory
	{
	public:
		static Allocation* allocate(uint8_t powerOfTwo);
		static void allocateList();

		static Allocation* freeStructList;
	private:
		static MemBlock block;
		static Allocation* searchNode(Allocation* node, uint8_t exp);
	};
}