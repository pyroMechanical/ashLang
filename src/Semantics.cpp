#include "Semantics.h"

namespace ash
{

	namespace util
	{
		static std::shared_ptr<ScopeNode> getScope(ParseNode* node, std::shared_ptr<ScopeNode> scope)
		{
			std::shared_ptr<ScopeNode> currentScope;

			if(node->nodeType() == NodeType::Block)
			{
				currentScope = std::make_shared<ScopeNode>();
				currentScope->parentScope = scope;
			}
			else
			{
				currentScope = scope;
			}

			return currentScope;
		}

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
		}

		static Token resolveBasicTypes(Token lhs, Token rhs)
		{
			std::string left = tokenstring(lhs);
			std::string right = tokenstring(rhs);
			if (left == right) return lhs;

			if (left.c_str() == "double")
			{
				return lhs;
			}

			if (left.c_str() == "float")
			{
				if (right.c_str() == "double")
					return rhs;
				return lhs;
			}

			if (left.c_str() == "long"
				|| left.c_str() == "ulong")
			{
				if (right.c_str() == "double"
					|| right.c_str() == "float")
					return rhs;
				return lhs;
			}
			if (left.c_str() == "int"
				|| left.c_str() == "uint")
			{
				if (right.c_str() == "double"
					|| right.c_str() == "float"
					|| right.c_str() == "long"
					|| right.c_str() == "ulong")
					return rhs;
				return lhs;
			}
			if (left.c_str() == "short"
				|| left.c_str() == "ushort")
			{
				if (right.c_str() == "double"
					|| right.c_str() == "float"
					|| right.c_str() == "long"
					|| right.c_str() == "int"
					|| right.c_str() == "ulong"
					|| right.c_str() == "uint")
					return rhs;
				return lhs;
			}
			if (left.c_str() == "byte"
				|| left.c_str() == "ubyte")
			{
				if (right.c_str() == "double"
					|| right.c_str() == "float"
					|| right.c_str() == "long"
					|| right.c_str() == "int"
					|| right.c_str() == "short"
					|| right.c_str() == "ulong"
					|| right.c_str() == "uint"
					|| right.c_str() == "ushort")
					return rhs;
				return lhs;
			}

			if(left.c_str() == "char")
			{
				if (right.c_str() != "char")
					return { TokenType::ERROR, nullptr, 0, lhs.line };
			}
		}
	}
	
	std::unique_ptr<ProgramNode> Semantics::findSymbols(std::unique_ptr<ProgramNode> ast)
	{
		std::shared_ptr<ScopeNode> currentScope = ast->globalScope = std::make_shared<ScopeNode>();

		for(const auto& declaration: ast->declarations)
		{
			auto scope = util::getScope((ParseNode*)declaration.get(), currentScope);
			enterNode((ParseNode*)declaration.get(), scope);
		}
		
	}

	void enterNode(ParseNode* node, std::shared_ptr<ScopeNode> currentScope)
	{
		switch (node->nodeType())
		{
			case NodeType::Block:
			{
				BlockNode* blockNode = (BlockNode*)node;
				blockNode->scope = currentScope;
				for(const auto& declaration : blockNode->declarations)
				{
					auto scope = util::getScope((ParseNode*)declaration.get(), currentScope);
					enterNode((ParseNode*)declaration.get(), scope);
				}
				break;
			}
			case NodeType::TypeDeclaration:
			{
				TypeDeclarationNode* typeNode = (TypeDeclarationNode*)node;
				std::string typeName = util::tokenstring(typeNode->typeDefined);
				Symbol s = { typeName, category::Type, typeNode->typeDefined };
				if (currentScope->symbols.find(typeName) == currentScope->symbols.end())
				{
					currentScope->symbols.emplace(typeName, s);
					currentScope->typeParameters.emplace(typeName, typeNode->fields);
				}
				else
				{
					std::cout << "Symbol " << typeName << " already defined!" << std::endl;
					Symbol defined = currentScope->symbols.at(typeName);
					std::cout << "Name: " << defined.name << " Category: " << util::categoryToString(defined.cat) << " Type: " << util::tokenstring(defined.type) << std::endl;
				}
				break;
			}
			case NodeType::FunctionDeclaration:
			{
				FunctionDeclarationNode* funcNode = (FunctionDeclarationNode*)node;
				std::string funcName = util::tokenstring(funcNode->identifier);
				Symbol s = { funcName, category::Function, funcNode->type };
				if (currentScope->symbols.find(funcName) == currentScope->symbols.end())
				{
					currentScope->symbols.emplace(funcName, s);

					currentScope->functionParameters.emplace(funcName, funcNode->parameters);
				}
				else
				{
					std::cout << "Symbol " << funcName << " already defined!" << std::endl;
					Symbol defined = currentScope->symbols.at(funcName);
					std::cout << "Name: " << defined.name << " Category: " << util::categoryToString(defined.cat) << " Type: " << util::tokenstring(defined.type) << std::endl;
				}
				auto blockScope = std::make_shared<ScopeNode>();
				blockScope->parentScope = currentScope;
				for(const auto& parameter : funcNode->parameters)
				{
					std::string paramName = util::tokenstring(parameter.identifier);
					Symbol t = { paramName, category::Variable, parameter.type };
					if(blockScope->symbols.find(paramName) == blockScope->symbols.end())
					{
						blockScope->symbols.emplace(paramName, t);
					}
					else
					{
						std::cout << "Symbol " << paramName << " already defined!" << std::endl;
						Symbol defined = blockScope->symbols.at(paramName);
						std::cout << "Name: " << defined.name << " Category: " << util::categoryToString(defined.cat) << " Type: " << util::tokenstring(defined.type) << std::endl;
					}
				}

				enterNode((ParseNode*)funcNode->body.get(), blockScope);
				break;
			}

			case NodeType::VariableDeclaration:
			{
				VariableDeclarationNode* varNode = (VariableDeclarationNode*)node;
				std::string varName = util::tokenstring(varNode->identifier);
				Symbol s = { varName, category::Variable, varNode->type };
				if (currentScope->symbols.find(varName) == currentScope->symbols.end())
				{
					currentScope->symbols.emplace(varName, s);
				}
				else
				{
					std::cout << "Symbol " << varName << " already defined!" << std::endl;
					Symbol defined = currentScope->symbols.at(varName);
					std::cout << "Name: " << defined.name << " Category: " << util::categoryToString(defined.cat) << " Type: " << util::tokenstring(defined.type) << std::endl;
				}

				if(varNode->value)
				{
					
				}
				break;
			}
		}
	}

	Token expressionTypeInfo(ExpressionNode* node, std::shared_ptr<ScopeNode> currentScope)
	{
		switch(node->expressionType())
		{
			case ExpressionNode::ExpressionType::Primary:
			{
				CallNode* callNode = (CallNode*)node;
				switch(callNode->primary.type)
				{
					case TokenType::IDENTIFIER:
					{
						Symbol* s = nullptr;
						bool found = false;
						auto scope = currentScope;
						std::string name = util::tokenstring(callNode->primary);
						while (!found && scope != nullptr)
						{
							auto it = scope->symbols.find(name);
							if (it == scope->symbols.end())
							{
								scope = scope->parentScope;
							}
							else
							{
								s = &it->second;
							}
						}
						if (!s)
						{
							std::cout << "Symbol " << name << " has not yet been declared.";
							return { TokenType::ERROR, callNode->primary.start,callNode->primary.length, callNode->primary.line };
						}

						return s->type;
					}
					case TokenType::TRUE:
					case TokenType::FALSE:
						return { TokenType::TYPE, "bool", 4, callNode->primary.line };
					case TokenType::NULL_:	return { TokenType::TYPE, "nulltype", 8, callNode->primary.line };
					case TokenType::FLOAT:	return { TokenType::TYPE, "float", 5, callNode->primary.line };
					case TokenType::DOUBLE: return { TokenType::TYPE, "double", 6, callNode->primary.line };
					case TokenType::INT: return { TokenType::TYPE, "int", 3, callNode->primary.line };
					case TokenType::CHAR: return { TokenType::TYPE, "char", 4, callNode->primary.line };
				}
			}
			case ExpressionNode::ExpressionType::Unary:
			{
				UnaryNode* unaryNode = (UnaryNode*)node;
				return expressionTypeInfo((ExpressionNode*)unaryNode->unary.get(), currentScope);
			}
			case ExpressionNode::ExpressionType::Binary:
			{
				BinaryNode* binaryNode = (BinaryNode*)node;
				Token leftType = expressionTypeInfo((ExpressionNode*)binaryNode->left.get(), currentScope);
				Token rightType = expressionTypeInfo((ExpressionNode*)binaryNode->right.get(), currentScope);

			}
		}
	}
}