#include "Memory.h"

namespace ash
{
	constexpr uint8_t minExponentSize = 4;
	Allocation* Memory::freeStructList = nullptr;

	void Allocation::split()
	{
		if (isSplit || exp <= minExponentSize) return;

		if (Memory::freeStructList == nullptr || Memory::freeStructList->right == nullptr) Memory::allocateList();

		if (Memory::freeStructList != nullptr && Memory::freeStructList->right != nullptr)
		{
			auto left = Memory::freeStructList;
			Memory::freeStructList = Memory::freeStructList->right;
			auto right = Memory::freeStructList;
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
			freeStructList->left = newStructs;
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
	}
}