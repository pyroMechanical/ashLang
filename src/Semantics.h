#pragma once
#include "Parser.h"

namespace ash
{
	class Semantics
	{
	private:
		void enterNode(ParseNode* node, std::shared_ptr<ScopeNode> currentScope);

		Token expressionTypeInfo(ExpressionNode* node, std::shared_ptr<ScopeNode> currentScope);
	public:
		std::unique_ptr<ProgramNode> findSymbols(std::unique_ptr<ProgramNode> ast);
	};
}