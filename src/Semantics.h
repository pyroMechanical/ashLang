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
		Token pushError(std::string msg, int line);

		std::shared_ptr<DeclarationNode> linearizeAST(ParseNode* node, std::vector<std::shared_ptr<DeclarationNode>>& currentBlock, std::shared_ptr<ScopeNode> currentScope);
		std::shared_ptr<ExpressionNode> pruneBinaryExpressions(ExpressionNode* node, std::vector<std::shared_ptr<DeclarationNode>>& currentBlock, std::shared_ptr<ScopeNode> currentScope);


		bool panicMode = false;
	public:
		std::shared_ptr<ProgramNode> findSymbols(std::shared_ptr<ProgramNode> ast);
		std::vector<Token> errorQueue;
		size_t temporaries = 0;
	};
}