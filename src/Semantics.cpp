#include "Semantics.h"

#include <unordered_set>

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

			std::string typestring = util::tokenstring(type);
			for (const auto& string : basicTypes)
			{
				if (typestring == string)
				{
					return true;
				}
			}
			return false;
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

		return ast;
		
	}

	void Semantics::enterNode(ParseNode* node, std::shared_ptr<ScopeNode> currentScope)
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
					std::unordered_set<std::string> typeFields;

					for (const auto& field : typeNode->fields)
					{
						auto id = util::tokenstring(field.identifier);

						if (typeFields.find(id) == typeFields.end())
						{
							typeFields.emplace(id);
						}

						else 
						{
							//Error: field already included!
						}
					}

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
					auto valueType = expressionTypeInfo((ExpressionNode*)varNode->value.get(), currentScope);

					if (util::tokenstring(valueType) != util::tokenstring(varNode->type))
					{
						std::cout << "Error: value is of type " << util::tokenstring(valueType) << ", assigned to variable of type " << util::tokenstring(varNode->type) << std::endl;
					}
				}
				break;
			}
		}
	}

	Token Semantics::expressionTypeInfo(ExpressionNode* node, std::shared_ptr<ScopeNode> currentScope)
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
							//return { TokenType::ERROR, callNode->primary.start,callNode->primary.length, callNode->primary.line };
						}
						if (s)
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
				Token exprType;
				Token leftType = expressionTypeInfo((ExpressionNode*)binaryNode->left.get(), currentScope);
				Token rightType = expressionTypeInfo((ExpressionNode*)binaryNode->right.get(), currentScope);

				if (util::isBasic(leftType) && util::isBasic(rightType))
				{
					exprType =  util::resolveBasicTypes(leftType, rightType);
				}
				else
				{
					if (util::tokenstring(leftType) == util::tokenstring(rightType))
					{
						exprType =  leftType;
					}

					else exprType = { TokenType::ERROR, nullptr, 0, leftType.line };
				}

				switch (binaryNode->op.type)
				{
					case TokenType::EQUAL:
					case TokenType::BANG_EQUAL:
					case TokenType::LESS:
					case TokenType::LESS_EQUAL:
					case TokenType::GREATER:
					case TokenType::GREATER_EQUAL:
					case TokenType::AND:
					case TokenType::OR:
					{
						if (exprType.type != TokenType::ERROR)
						{
							return { TokenType::TYPE, "bool", 4, leftType.line };
						}
						else return exprType;
					}
					case TokenType::PLUS:
					case TokenType::MINUS:
					case TokenType::STAR:
					case TokenType::SLASH:
					{
						return exprType;
					}
				}
			}
			case ExpressionNode::ExpressionType::Assignment:
			{
				AssignmentNode* assignmentNode = (AssignmentNode*)node;
				Symbol* s = nullptr;
				bool found = false;
				auto scope = currentScope;
				bool fieldCall = false;
				std::string name;
				std::string fullName = assignmentNode->resolveIdentifier();
				//TODO: check if name is a field call, and if it is, 
				// find the base variable first, then work down the list of fields
				if (fullName == "")
				{
					std::cout << "Can only assign to a named variable!" << std::endl;
					//Error
				}
				if (fullName.find(".") != std::string::npos)
				{
					name = std::string(fullName, 0, fullName.find(".") - 1);
					fieldCall = true;
					
				}
				else
				{
					name = fullName;
					fieldCall = false;
				}
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
					//return { TokenType::ERROR, assignmentNode->identifier.start,assignmentNode->identifier.length, assignmentNode->identifier.line };
				}

				if (fieldCall)
				{
					//TODO: determine type of the field being referenced
				}

				Token valueType = expressionTypeInfo((ExpressionNode*)assignmentNode->value.get(), currentScope);

				if (util::tokenstring(valueType) != util::tokenstring(s->type))
				{
					//Error: attempted to assign invalid type to identifier
				}

				return s->type;
			}
			case ExpressionNode::ExpressionType::FieldCall:
			{
				FieldCallNode* fieldCallNode = (FieldCallNode*)node;

				Token parentType = expressionTypeInfo((ExpressionNode*)fieldCallNode->left.get(), currentScope);

				auto scope = currentScope;
				bool found = false;
				std::string typeID = util::tokenstring(parentType);
				while (!found && scope != nullptr)
				{
					if (scope->symbols.find(typeID) == scope->symbols.end())
					{
						scope = scope->parentScope;
					}
					else
					{
						std::vector<parameter> fields = scope->typeParameters.at(typeID);

						for (const auto& field : fields)
						{
							if (util::tokenstring(field.identifier) == util::tokenstring(fieldCallNode->field))
							{
								return field.type;
							}
						}
						//error: type valid, but does not contain field!
					}
				}
				//Error: type not found!
			}
		}
	}
}