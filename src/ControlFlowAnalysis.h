#pragma once
#include "ParseTree.h"

namespace ash
{
	class ControlFlowAnalysis
	{
	public:
		std::shared_ptr<ControlFlowGraph> createCFG(std::shared_ptr<ProgramNode> ast);
	private:
		std::shared_ptr<ControlFlowGraph> result = nullptr;

		void traverseNodes(ParseNode* node);

		std::shared_ptr<DeclarationNode> serializeNode(ParseNode* node, std::shared_ptr<ParseNode> next);
	};
}