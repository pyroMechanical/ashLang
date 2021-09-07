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
			0x00: 1 byte
			0x01: 2 bytes
			0x02: 4 bytes
			0x04: 8 bytes
			0x08: pointer
			0x80: array; will be the only field in TypeMetadata.fields
		*/
	};

	struct Allocation
	{
		char* memory;
		Allocation* next;
		bool marked = false;
		TypeMetadata* offsets;
	};
}