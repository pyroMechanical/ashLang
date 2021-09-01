#pragma once

#include <stdlib.h>
#include <vector>
namespace ash
{
	struct FieldMetadata
	{
		bool isPointer; //replace with uint8_t bit flag later?
		uint32_t offset;
	};

	struct TypeMetadata
	{
		std::vector<FieldMetadata> offsets;
	};

	struct Allocation
	{
		char* memory;
		Allocation* next;
		bool marked = false;
		TypeMetadata* offsets;
	};
}