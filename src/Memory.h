#pragma once

#include <stdlib.h>
namespace ash
{


	struct Allocation
	{
		void* memory;
		Allocation* next;
	};

	class Memory
	{
	private:
		Allocation* allocationList;
	public:
		void* allocate(void* pointer, size_t oldSize, size_t newSize);

		void freeAllocation(Allocation* alloc);

		void freeAllocations();

		void collectGarbage();
	};

}