#include "Memory.h"
#include "VM.h"

#include <iostream>

#define STRESSTEST_GC
#define LOG_GC

namespace ash
{
	void* Memory::allocate(void* pointer, size_t oldSize, size_t newSize)
	{
		if (newSize > oldSize)
#ifdef STRESSTEST_GC
			collectGarbage();
#endif

		if(newSize == 0)
		{
			free(pointer);
			return nullptr;
		}

		void* result = realloc(pointer, newSize);
		memset(result, 0, newSize);
		if (result == nullptr) exit(1);
		Allocation* allocation = new Allocation();
		allocation->memory = result;
		allocation->next = allocationList;
		allocationList = allocation;

		return (void*)allocation;
	}

	void Memory::freeAllocation(Allocation* alloc)
	{
		delete alloc->memory;
		delete alloc;
	}

	void Memory::freeAllocations()
	{
		Allocation* allocation = allocationList;

		while (allocation != nullptr)
		{
			Allocation* next = allocation->next;
			Memory::freeAllocation(allocation);
			allocation = next;
		}
	}

	void Memory::collectGarbage()
	{
#ifdef LOG_GC
		std::cout << "--begin gc" << std::endl;
#endif



#ifdef LOG_GC
		std::cout << "--end gc" << std::endl;
#endif
	}
}