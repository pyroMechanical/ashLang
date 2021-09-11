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
	};

	struct Allocation
	{
		char* memory;
		Allocation* next;
		bool marked = false;
	};
}