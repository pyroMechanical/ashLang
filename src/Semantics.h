#pragma once
#include "Parser.h"

namespace ash
{
	class Semantics
	{
	private:
		bool enterNode(ParseNode* node, std::shared_ptr<ScopeNode> currentScope);
		bool functionValidator(ParseNode* node, bool inFunction = false);
		bool hasReturnPath(ParseNode* node, std::shared_ptr<ScopeNode> currentScope, Token returnType);
		Token expressionTypeInfo(ExpressionNode* node, std::shared_ptr<ScopeNode> currentScope, Token expected = {});
		Token pushError(const char* msg, int line);

		bool panicMode = false;
	public:
		std::unique_ptr<ProgramNode> findSymbols(std::unique_ptr<ProgramNode> ast);
		std::vector<Token> errorQueue;
	};
}