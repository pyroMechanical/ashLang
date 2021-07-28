#pragma once
#include "Parser.h"

namespace ash
{
	class Semantics
	{
	private:
		bool enterNode(ParseNode* node, std::shared_ptr<ScopeNode> currentScope);

		Token expressionTypeInfo(ExpressionNode* node, std::shared_ptr<ScopeNode> currentScope, Token expected = {});
	public:
		std::unique_ptr<ProgramNode> findSymbols(std::unique_ptr<ProgramNode> ast);
	};
}