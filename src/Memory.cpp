#include "Memory.h"
#include <iostream>
#include <deque>

namespace ash
{
	Allocation* Memory::freeStructList = nullptr;
	MemBlock Memory::block{20};

	MemBlock::MemBlock(uint8_t powerOfTwo)
	{
		exp = powerOfTwo;
		begin = malloc((size_t)1 << exp);
		if (begin == nullptr)
		{
			std::cerr << "Malloc failed! size: " << ((size_t)1 << exp ) << std::endl;
			std::cerr << "Allocation structs: " << allocationStructs << std::endl;
			exit(1);
		}

		if (Memory::freeStructList == nullptr)
		{
			Memory::allocateList(exp);
		}
		if (Memory::freeStructList != nullptr)
		{
			root = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->right;
			root->memory = (char*)begin;
			root->exp = exp;
			root->isSplit = false;
			root->refCount = 0;
			root->left = nullptr;
			root->right = nullptr;
		}
		else
		{
			std::cerr << "No free structs found!" << std::endl;
			std::cerr << "Allocation structs: " << allocationStructs << std::endl;
			exit(1);
		}
	}

	MemBlock::~MemBlock()
	{
		free(begin);
		//TODO: free all Allocation* nodes by returning them to the memory free list
	}

	void Allocation::split()
	{
		if (isSplit || exp <= Memory::minExponentSize) return;

		if (Memory::freeStructList == nullptr || Memory::freeStructList->right == nullptr) Memory::allocateList(exp);

		if (Memory::freeStructList != nullptr && Memory::freeStructList->right != nullptr)
		{
			left = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->right;
			right = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->right;
			left->exp = exp - 1;
			right->exp = exp - 1;
			left->memory = memory;
			right->memory = (memory + left->size()); //split block in two, one offset by the size of the split allocation
			left->right = nullptr;
			left->left = nullptr;
			left->refCount = 0;
			right->right = nullptr;
			right->left = nullptr;
			right->refCount = 0;
			left = left;
			right = right;
			isSplit = true;
		}
	}

	void Allocation::merge()
	{
		if(right->memory != left->memory + left->size()) return;
		
		right->right = Memory::freeStructList;
		right->left = left;
		left->right = right;
		if(Memory::freeStructList) Memory::freeStructList->left = right;
		Memory::freeStructList = left;
		left = nullptr;
		right = nullptr;
		isSplit = false;
	}

	void Memory::allocateList(uint8_t count)
	{
		Allocation* newStructs = new Allocation[count];
		for (int i = 0; i < count; i++)
		{
			newStructs->right = freeStructList;
			if(freeStructList) freeStructList->left = newStructs;
			freeStructList = newStructs;
			newStructs++;
		}
		block.allocationStructs += count;
		std::cout << block.allocationStructs << std::endl;
	}

	Allocation* Memory::allocate(uint8_t exp)
	{
		if (exp < minExponentSize) exp = minExponentSize;

		Allocation* result = searchNode(block.root, exp);

		if (result == nullptr)
		{
			printAllocation(block.root);
			std::cerr << "Malloc failed! size: " << ((size_t)1 << exp) << std::endl;
			std::cerr << "Allocation structs: " << block.allocationStructs << std::endl;
			exit(1);
		}
		return result;
	}

	Allocation* Memory::searchNode(Allocation* node, uint8_t exp)
	{
		if(node->isSplit == false)
		{
			if (exp == node->exp) return node;
			else node->split();
		}

		bool freeLeft = (node->left != nullptr);
		bool freeRight = (node->right != nullptr);

		if (!freeLeft && !freeRight) return nullptr;

		if (freeLeft)
		{
			Allocation* leftResult = searchNode(node->left, exp);
			if (leftResult == node->left) node->left = nullptr;
			if (leftResult != nullptr) return leftResult;
		}
		if (freeRight)
		{
			Allocation* rightResult = searchNode(node->right, exp);
			if (rightResult == node->right) node->right = nullptr;
			if (rightResult != nullptr) return rightResult;
		}
		return nullptr;
	}

	void Memory::free(Allocation* ptr)
	{
		ptr->left = nullptr;
		ptr->right = nullptr;
		returnNode(block.root, ptr);
	}

	void Memory::returnNode(Allocation* node, Allocation* freed)
	{
		if(freed->exp >= node->exp)
		{
			std::cout << "Error: memory returned to wrong node!" << std::endl;
			exit(3);
		}
		if(freed->memory >= node->memory + ((size_t)1<<(node->exp - 1)))
		{
			if (freed->exp == node->exp - 1)
			{
				node->right = freed;
			}
			else
			{
				returnNode(node->right, freed);
			}
		}
		else if (freed->memory >= node->memory && freed->memory < node->memory + ((size_t)1 << (node->exp - 1)))
		{
			if (freed->exp == node->exp - 1) 
			{ 
				node->left = freed; 
			}
			else
			{
				returnNode(node->left, freed);
			}
		}
		else
		{
			std::cerr << "Memory in wrong node! freed: " << (void*)freed->memory << " current: " << (void*)node->memory << std::endl;
			exit(2);
		}

		if (node->left && node->right && !(node->left->isSplit) && !(node->right->isSplit))
		{
			node->merge();
		}
	}

	void Memory::printAllocation(Allocation* node)
	{
		std::deque<Allocation*> allocations;
		allocations.push_back(node);
		while(allocations.size())
		{
			Allocation* current = allocations.front();
			allocations.pop_front();
			std::cout << std::string(block.root->exp - current->exp, ' ') << "node size: " << ((size_t)1 << current->exp) << " node begin: " << (void*)(current->memory) << " split?: " << (current->isSplit? "true" : "false") << std::endl;
			if (current->right && current->right->exp < current->exp) allocations.push_front(current->right);
			if (current->left && current->left->exp < current->exp) allocations.push_front(current->left);
		}
	}
}