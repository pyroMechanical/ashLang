#pragma once

#include <stdlib.h>
#include <vector>
namespace ash
{

	struct TypeMetadata
	{
		TypeMetadata* parent;
		std::vector<uint8_t> fields;
		/*
			0x01: 1 byte
			0x03: 2 bytes
			0x04: 4 bytes
			0x08: 8 bytes
			0x80: pointer
		*/
		uint8_t spacing;
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
		uint64_t size;

		virtual AllocationType type() = 0;
	};

	struct TypeAllocation : public Allocation
	{
		virtual AllocationType type() override { return AllocationType::Type; }
	};

	struct ArrayAllocation : public Allocation
	{
		virtual AllocationType type() override { return AllocationType::Array; }
	};
}