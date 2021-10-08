#include "Memory.h"

namespace ash
{
	constexpr uint8_t minExponentSize = 4;
	Allocation* Memory::freeStructList = nullptr;

	void freeNode::split()
	{
		if (value->exp <= minExponentSize) return;

		if (Memory::freeStructList == nullptr) Memory::allocateList();

		if (Memory::freeStructList != nullptr)
		{
			auto left = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->next;
			auto right = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->next;
			left->exp = right->exp = value->exp - 1;
			left->memory = value->memory;
			right->memory = (value->memory + right->size()); //split block in two, one offset by the size of the split allocation
			left->next = nullptr;
			left->previous = nullptr;
			left->refCount = 0;
			right->next = nullptr;
			right->previous = nullptr;
			right->refCount = 0;
			auto leftBranch = std::make_shared<freeNode>();
			leftBranch->value = left;
			auto rightBranch = std::make_shared<freeNode>();
			rightBranch->value = right;
		}
	}

	void Memory::allocateList()
	{
		Allocation* newStructs = new Allocation[10];
		for (int i = 0; i < 10; i++)
		{
			newStructs->next = freeStructList;
			freeStructList->previous = newStructs;
			freeStructList = newStructs;
			newStructs++;
		}
	}

	Allocation* Memory::allocate(uint8_t exp)
	{
		
	}
}