#pragma once
#include "Parser.h"

namespace ash
{
	namespace util
	{

		static bool isBasic(Token type)
		{
			std::vector<std::string> basicTypes = {
				std::string("bool"),
				std::string("byte"),
				std::string("short"),
				std::string("int"),
				std::string("long"),
				std::string("float"),
				std::string("double"),
				std::string("char"),
				std::string("ubyte"),
				std::string("ushort"),
				std::string("uint"),
				std::string("ulong")
			};

			for (const auto& string : basicTypes)
			{
				if (type.string == string)
				{
					return true;
				}
			}
			return false;
		}

		static Token resolveBasicTypes(Token lhs, Token rhs)
		{
			std::string left = lhs.string;
			std::string right = rhs.string;
			if (left.compare(right) == 0) return lhs;

			if (left.compare("double") == 0)
			{
				return lhs;
			}
			else if (left.compare("float") == 0)
			{
				if (right.compare("double") == 0)
					return rhs;
				return lhs;
			}

			else if (left.compare("long") == 0
				|| left.compare("ulong") == 0)
			{
				if (right.compare("double") == 0
					|| right.compare("float") == 0)
					return rhs;
				return lhs;
			}
			else if (left.compare("int") == 0
				|| left.compare("uint") == 0)
			{
				if (right.compare("double") == 0
					|| right.compare("float") == 0
					|| right.compare("long") == 0
					|| right.compare("ulong") == 0)
					return rhs;
				return lhs;
			}
			else if (left.compare("short") == 0
				|| left.compare("ushort") == 0)
			{
				if (right.compare("double") == 0
					|| right.compare("float") == 0
					|| right.compare("long") == 0
					|| right.compare("int") == 0
					|| right.compare("ulong") == 0
					|| right.compare("uint") == 0)
					return rhs;
				return lhs;
			}
			else if (left.compare("byte") == 0
				|| left.compare("ubyte") == 0)
			{
				if (right.compare("double") == 0
					|| right.compare("float") == 0
					|| right.compare("long") == 0
					|| right.compare("int") == 0
					|| right.compare("short") == 0
					|| right.compare("ulong") == 0
					|| right.compare("uint") == 0
					|| right.compare("ushort") == 0)
					return rhs;
				return lhs;
			}

			else if (left.compare("char") == 0)
			{
				if (right.compare("char") != 0)
					return { TokenType::ERROR, "", lhs.line };
			}

			return { TokenType::ERROR, "", lhs.line };
		}
	}

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

		std::shared_ptr<ScopeNode> getScope(ParseNode* node, std::shared_ptr<ScopeNode> scope)
		{
			std::shared_ptr<ScopeNode> currentScope;

			if (!node)
			{
				return scope;
			}

			if (node->nodeType() == NodeType::Block)
			{
				currentScope = std::make_shared<ScopeNode>();
				currentScope->parentScope = scope;
				currentScope->scopeIndex = scopeCount++;
				scopes.push_back(currentScope);
			}
			else
			{
				currentScope = scope;
			}

			return currentScope;
		}

		bool panicMode = false;
	public:
		std::shared_ptr<ProgramNode> findSymbols(std::shared_ptr<ProgramNode> ast);
		std::vector<Token> errorQueue;
		std::vector<std::shared_ptr<ScopeNode>> scopes;
		size_t temporaries = 0;
		size_t scopeCount = 0;
	};
}