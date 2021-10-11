#include "Memory.h"
#include <iostream>

namespace ash
{
	constexpr uint8_t minExponentSize = 4;
	Allocation* Memory::freeStructList = nullptr;
	MemBlock Memory::block{20};

	MemBlock::MemBlock(uint8_t powerOfTwo)
	{
		exp = powerOfTwo;
		begin = malloc((size_t)1 << exp);
		if (begin == nullptr)
		{
			std::cerr << "Malloc failed! size: " << ((size_t)1 << exp ) << std::endl;
			exit(1);
		}

		if (Memory::freeStructList == nullptr)
		{
			Memory::allocateList();
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
		else exit(1);
	}

	MemBlock::~MemBlock()
	{
		free(begin);
		//TODO: free all Allocation* nodes by returning them to the memory free list
	}

	void Allocation::split()
	{
		if (isSplit || exp <= minExponentSize) return;

		if (Memory::freeStructList == nullptr || Memory::freeStructList->right == nullptr) Memory::allocateList();

		if (Memory::freeStructList != nullptr && Memory::freeStructList->right != nullptr)
		{
			left = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->right;
			right = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->right;
			left->exp = right->exp = exp - 1;
			left->memory = memory;
			right->memory = (memory + right->size()); //split block in two, one offset by the size of the split allocation
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
		auto& head = Memory::freeStructList;
		right->right = head;
		head->left = right;
		right->left = left;
		left->right = right;
		head = left;
		left = nullptr;
		right = nullptr;
		isSplit = false;
	}

	void Memory::allocateList()
	{
		Allocation* newStructs = new Allocation[10];
		for (int i = 0; i < 10; i++)
		{
			newStructs->right = freeStructList;
			if(freeStructList) freeStructList->left = newStructs;
			freeStructList = newStructs;
			newStructs++;
		}
	}

	Allocation* Memory::allocate(uint8_t exp)
	{
		if (exp < minExponentSize) exp = minExponentSize;

		Allocation* result = searchNode(block.root, exp);

		if (result == nullptr) exit(1);

		return result;
	}

	Allocation* Memory::searchNode(Allocation* node, uint8_t exp)
	{
		if (node->refCount != 0) return nullptr;
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
			if (leftResult != nullptr) return leftResult;
		}
		if (freeRight)
		{
			Allocation* rightResult = searchNode(node->right, exp);
			if (rightResult != nullptr) return rightResult;
		}
		return nullptr;
	}

	void Memory::free(Allocation* ptr)
	{
		returnNode(block.root, ptr);
	}

	void Memory::returnNode(Allocation* node, Allocation* freed)
	{
		if (freed->exp + 1 == node->exp)
		{
			std::cout << (void*)(freed->memory + freed->exp) << std::endl;
			if(freed->memory > node->memory)
			{
				node->right = freed;
				if (node->left && !node->left->isSplit) node->merge();
			}
			else if (freed->memory == node->memory)
			{
				node->left = freed;
				if (node->right && !node->right->isSplit) node->merge();
			}
		}
		else
		{
			if(freed->memory >= node->memory + (node->exp - 1))
			{
				returnNode(node->right, freed);
				if (!node->right->isSplit && node->left && node->left->isSplit) node->merge();
			}
			else
			{
				returnNode(node->left, freed);
				if (!node->left->isSplit && node->right && node->right->isSplit) node->merge();
			}
		}
	}
}