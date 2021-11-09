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
		count = (PAGE_SIZE - OVERHEAD_BYTES) / size + (PAGE_SIZE / size == 0);
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

	Memory::Memory()
	{
		std::vector<MemBlock> two;
		two.push_back(MemBlock(2));
		memBlockLists.emplace(2, two);
		std::vector<MemBlock> three;
		three.push_back(MemBlock(3));
		memBlockLists.emplace(3, three);
		std::vector<MemBlock> four;
		four.push_back(MemBlock(4));
		memBlockLists.emplace(4, four);
		std::vector<MemBlock> five;
		five.push_back(MemBlock(5));  
		memBlockLists.emplace(5, five);
		std::vector<MemBlock> six;
		six.push_back(MemBlock(6));
		memBlockLists.emplace(6, six);
		std::vector<MemBlock> seven;
		seven.push_back(MemBlock(7));
		memBlockLists.emplace(7, seven);
		std::vector<MemBlock> eight;
		eight.push_back(MemBlock(8));
		memBlockLists.emplace(8, eight);
	}

	void* Memory::allocate(uint8_t powerOfTwo, std::vector<uint8_t>& bitFlags)
	{
		if (memBlockLists.find(powerOfTwo) != memBlockLists.end())
		{
			auto& list = memBlockLists.at(powerOfTwo);
			for (auto it = list.rbegin(); it != list.rend(); it++)
			{
				if (it->notFull())
				{
					auto& block = *it;
					auto addr = block.alloc();
					if (bitFlags.size())
					{
						size_t diff = (size_t*)addr - (size_t*)block.firstAddress();
						char* flagsBegin = (char*)block.firstAddress() + (diff / sizeof(void*));
						uint8_t offset = diff % sizeof(void*);

					}
					if (addr)
					{
						return addr;
					}
					else
					{
						std::cerr << "freelist contains invalid memory!";
						exit(3);
					}
				}
			}
			list.push_back(MemBlock(powerOfTwo));
			MemBlock& newBlock = list.back();
			return newBlock.alloc();
		}
	}

	void Memory::freeAllocation(void* ptr)
	{
		char* diffptr = (char*)ptr;
		for (auto it = memBlocks.begin(); it != memBlocks.end(); it++)
		{
			ptrdiff_t offset = diffptr - it->second.firstAddress();
			if (diffptr >= it->second.firstAddress() && offset < it->second.size())
			{
				it->second.free(ptr);
			}
		}
	}
}