#pragma once
#include "Scanner.h"

#include <vector>
namespace ash
{
	struct node
	{
		Token token;
		node* left;
		node* right;

		void print()
		{
			printf("%.*s\n", token.length, token.start);
		}
	};

	class SyntaxTree
	{
	public:
		node* root;
		std::vector<node*> branch;

		SyntaxTree(node* root)
			:root(root)
		{
		};

		std::vector<node*> currentBranch() const { return branch; }

		node* currentNode() const { return branch.back(); }

		node* addLeft(node* child)
		{
			branch.back()->left = child;
			branch.push_back(child);

			return child;
		}

		node* addRight(node* child)
		{
			branch.back()->right = child;
			branch.push_back(child);

			return child;
		}

		void print()
		{
			printNode(root, 0);
		}

	private:
		void printNode(node* node, int level)
		{
			if (node == nullptr) return;

			printNode(node->left, level + 1);
			printNode(node->right, level + 1);

			printf("%*s", level * 2, "");
			node->print();
		}
	};
}