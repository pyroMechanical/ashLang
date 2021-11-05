#include "Memory.h"
#include <iostream>
#include <algorithm>
namespace ash
{

#define PAGE_SIZE 4096
#define OVERHEAD_BYTES (PAGE_SIZE / sizeof(void*))
#define OVERHEAD_BITS (OVERHEAD_BYTES / CHAR_BIT) * 2

	MemBlock::MemBlock(uint8_t powerOfTwo)
	{
		exp = powerOfTwo;
		size_t size = (size_t)1 << exp;
		size_t count = (PAGE_SIZE - OVERHEAD_BYTES) / size + (PAGE_SIZE / size == 0);
		char* block = (char*)calloc(count, size);
		freeList.reserve(count - ((size / OVERHEAD_BYTES) - (size/OVERHEAD_BYTES == 0)));
		std::generate(freeList.begin(), freeList.end(), [&block, &size, addr = block + (size < OVERHEAD_BYTES) ? OVERHEAD_BYTES : size]()
			{
				return addr + size;
			});
		std::cout << (void*)block << std::endl;
		std::for_each(freeList.begin(), freeList.end(), [](void* addr)
			{
				std::cout << addr << std::endl;
			});
		begin = block;
	}

	MemBlock::~MemBlock()
	{
		free(begin);
		freeList.clear();
	}

	void* Memory::allocate(uint8_t powerOfTwo)
	{
		
	}

	void Memory::freeAllocation(void* ptr)
	{

	}
}