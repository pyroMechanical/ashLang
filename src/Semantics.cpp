#include "Semantics.h"

#include <unordered_set>

namespace ash
{

	namespace util
	{
		static std::shared_ptr<ScopeNode> getScope(ParseNode* node, std::shared_ptr<ScopeNode> scope)
		{
			std::shared_ptr<ScopeNode> currentScope;

			if(!node)
			{
				return scope;
			}

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

		bool hadError = false;
		for(const auto& declaration: ast->declarations)
		{
			auto scope = util::getScope((ParseNode*)declaration.get(), currentScope);
			hadError |= enterNode((ParseNode*)declaration.get(), scope);
		}
		std::cout << "Error?: " << hadError << std::endl;
		ast->hadError = hadError;
		return ast;
	}

	bool Semantics::enterNode(ParseNode* node, std::shared_ptr<ScopeNode> currentScope)
	{
		switch (node->nodeType())
		{
			case NodeType::Block:
			{
				BlockNode* blockNode = (BlockNode*)node;
				blockNode->scope = currentScope;
				bool hadError = false;
				for(const auto& declaration : blockNode->declarations)
				{
					auto scope = util::getScope((ParseNode*)declaration.get(), currentScope);
					hadError |= enterNode((ParseNode*)declaration.get(), scope);
				}
				return hadError;
			}
			case NodeType::TypeDeclaration:
			{
				TypeDeclarationNode* typeNode = (TypeDeclarationNode*)node;
				std::string typeName = util::tokenstring(typeNode->typeDefined);
				Symbol s = { typeName, category::Type, typeNode->typeDefined };
				bool hadError = false;
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
							hadError = true;
						}
					}

					currentScope->typeParameters.emplace(typeName, typeNode->fields);
				}
				else
				{
					std::cout << "Symbol " << typeName << " already defined!" << std::endl;
					Symbol defined = currentScope->symbols.at(typeName);
					std::cout << "Name: " << defined.name << " Category: " << util::categoryToString(defined.cat) << " Type: " << util::tokenstring(defined.type) << std::endl;
					hadError = true;
				}
				return hadError;
			}
			case NodeType::FunctionDeclaration:
			{
				FunctionDeclarationNode* funcNode = (FunctionDeclarationNode*)node;
				std::string funcName = util::tokenstring(funcNode->identifier);
				Symbol s = { funcName, category::Function, funcNode->type };
				bool hadError = false;
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
					hadError = true;
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
						hadError = true;
					}
				}

				hadError |= enterNode((ParseNode*)funcNode->body.get(), blockScope);
				return hadError;
			}

			case NodeType::VariableDeclaration:
			{
				VariableDeclarationNode* varNode = (VariableDeclarationNode*)node;
				std::string varName = util::tokenstring(varNode->identifier);
				Symbol s = { varName, category::Variable, varNode->type };
				bool hadError = false;
				if (currentScope->symbols.find(varName) == currentScope->symbols.end())
				{
					currentScope->symbols.emplace(varName, s);
				}
				else
				{
					std::cout << "Symbol " << varName << " already defined!" << std::endl;
					Symbol defined = currentScope->symbols.at(varName);
					std::cout << "Name: " << defined.name << " Category: " << util::categoryToString(defined.cat) << " Type: " << util::tokenstring(defined.type) << std::endl;
					hadError = true;
				}

				if(varNode->value)
				{
					auto valueType = expressionTypeInfo((ExpressionNode*)varNode->value.get(), currentScope);

					if (util::tokenstring(valueType) != util::tokenstring(varNode->type))
					{
						std::cout << "Error: value is of type " << util::tokenstring(valueType) << ", assigned to variable of type " << util::tokenstring(varNode->type) << std::endl;
						hadError = true;
					}
				}
				return hadError;
			}

			case NodeType::ForStatement:
			{
				ForStatementNode* forNode = (ForStatementNode*)node;
				std::shared_ptr<ScopeNode> forScope = currentScope;
				bool hadError = false;
				if(forNode->statement) forScope = util::getScope(forNode->statement.get(), currentScope);
				if (forNode->declaration) hadError |= enterNode(forNode->declaration.get(), forScope);
				if (forNode->conditional) hadError |= enterNode(forNode->conditional.get(), forScope);
				if (forNode->increment) hadError |= enterNode(forNode->increment.get(), forScope);
				if (forNode->statement) hadError |= enterNode(forNode->statement.get(), forScope);
				return hadError;
			}
			case NodeType::IfStatement:
			{
				IfStatementNode* ifNode = (IfStatementNode*)node;
				std::shared_ptr<ScopeNode> thenScope = currentScope;
				std::shared_ptr<ScopeNode> elseScope = currentScope;
				if (ifNode->thenStatement) thenScope = util::getScope(ifNode->thenStatement.get(), currentScope);
				if (ifNode->elseStatement) elseScope = util::getScope(ifNode->elseStatement.get(), currentScope);
				bool hadError = false;
				if (ifNode->condition) hadError |= enterNode(ifNode->condition.get(), currentScope);
				if (ifNode->thenStatement) hadError |= enterNode(ifNode->thenStatement.get(), thenScope);
				if (ifNode->elseStatement) hadError |= enterNode(ifNode->elseStatement.get(), elseScope);
				return hadError;
			}
			case NodeType::ReturnStatement:
			{
				ReturnStatementNode* returnNode = (ReturnStatementNode*)node;
				bool hadError = false;
				if (returnNode->returnValue) hadError |= enterNode(returnNode->returnValue.get(), currentScope);
				return hadError;
			}
			case NodeType::WhileStatement:
			{
				WhileStatementNode* whileNode = (WhileStatementNode*)node;
				std::shared_ptr<ScopeNode> whileScope = currentScope;
				bool hadError = false;
				if (whileNode->doStatement) whileScope = util::getScope(whileNode->doStatement.get(), currentScope);
				if (whileNode->condition) hadError |= enterNode(whileNode->condition.get(), whileScope);
				if (whileNode->doStatement) hadError |= enterNode(whileNode->doStatement.get(), whileScope);
				return hadError;
			}
			case NodeType::ExpressionStatement:
			{
				ExpressionStatement* exprNode = (ExpressionStatement*)node;
				bool hadError = false;
				if (exprNode->expression) hadError |= enterNode(exprNode->expression.get(), currentScope);
				return hadError;
			}
			case NodeType::Expression:
			{
				Token exprType = expressionTypeInfo((ExpressionNode*)node, currentScope);
				if (exprType.type == TokenType::ERROR) return true;
				else return false;
			}
		}
	}

	Token Semantics::expressionTypeInfo(ExpressionNode* node, std::shared_ptr<ScopeNode> currentScope, Token expected)
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
								found = true;
							}
						}
						if (!s)
						{
							std::cout << "Symbol " << name << " has not yet been declared.";
							//return { TokenType::ERROR, callNode->primary.start,callNode->primary.length, callNode->primary.line };
						}
						if (s)
						{
							if (expected.type != TokenType::ERROR)
							{
								if (util::tokenstring(s->type) != util::tokenstring(expected))
								{
									return { TokenType::ERROR, s->type.start, s->type.length, s->type.line };
								}
							}
							return s->type;
						}
					}
					case TokenType::TRUE:
					case TokenType::FALSE:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("bool" != util::tokenstring(expected))
							{
								return { TokenType::ERROR, callNode->primary.start, callNode->primary.length, callNode->primary.line };
							}
						}

						return { TokenType::TYPE, "bool", 4, callNode->primary.line };
					}
						
					case TokenType::NULL_:
					{
						return { TokenType::TYPE, "nulltype", 8, callNode->primary.line };
					}
					case TokenType::FLOAT: 
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("float" != util::tokenstring(expected))
							{
								return { TokenType::ERROR, callNode->primary.start, callNode->primary.length, callNode->primary.line };
							}
						}
						return { TokenType::TYPE, "float", 5, callNode->primary.line }; 
					}
					case TokenType::DOUBLE:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("double" != util::tokenstring(expected))
							{
								return { TokenType::ERROR, callNode->primary.start, callNode->primary.length, callNode->primary.line };
							}
						}
						return { TokenType::TYPE, "double", 6, callNode->primary.line }; 
					}
					case TokenType::INT:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("byte" != util::tokenstring(expected) &&
								"ubyte" != util::tokenstring(expected) && 
								"short" != util::tokenstring(expected) && 
								"ushort" != util::tokenstring(expected) && 
								"int" != util::tokenstring(expected) && 
								"uint" != util::tokenstring(expected) && 
								"long" != util::tokenstring(expected) && 
								"ulong" != util::tokenstring(expected)
								)
							{
								return { TokenType::ERROR, callNode->primary.start, callNode->primary.length, callNode->primary.line };
							}
						}
						return { TokenType::TYPE, "int", 3, callNode->primary.line };
					}
					case TokenType::CHAR: return { TokenType::TYPE, "char", 4, callNode->primary.line };
				}
			}
			case ExpressionNode::ExpressionType::Unary:
			{
				UnaryNode* unaryNode = (UnaryNode*)node;
				Token unaryValue = expressionTypeInfo((ExpressionNode*)unaryNode->unary.get(), currentScope);
				if (expected.type != TokenType::ERROR)
				{
					if (util::tokenstring(unaryValue) != util::tokenstring(expected))
					{
						return { TokenType::ERROR, unaryValue.start, unaryValue.length, unaryValue.line };
					}
				}
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
							if (expected.type != TokenType::ERROR)
							{
								if ("bool" != util::tokenstring(expected))
								{
									return { TokenType::ERROR, leftType.start, leftType.length, leftType.line };
								}
							}
							return { TokenType::TYPE, "bool", 4, leftType.line };
						}
						else return exprType;
					}
					case TokenType::PLUS:
					case TokenType::MINUS:
					case TokenType::STAR:
					case TokenType::SLASH:
					{
						if (exprType.type != TokenType::ERROR)
						{
							if (expected.type != TokenType::ERROR)
							{
								if ("bool" != util::tokenstring(expected))
								{
									return { TokenType::ERROR, leftType.start, leftType.length, leftType.line };
								}
							}
							return exprType;
						}
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
					name = std::string(fullName, 0, fullName.find("."));
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
						found = true;
					}
				}
				if (!s)
				{
					std::cout << "Symbol " << name << " has not yet been declared.";
					//return { TokenType::ERROR, };
				}
				else
				{
					auto assignedType = s->type;
					auto minimumScope = currentScope;

					if (fieldCall)
					{
						size_t lastPos = fullName.find(".") + 1;
						std::string fieldName;
						while (lastPos != 0 && minimumScope != nullptr)
						{
							size_t pos = fullName.find(".", lastPos);
							size_t len = pos - lastPos;
							fieldName = fullName.substr(lastPos, len);
							lastPos = pos + 1;
							auto it = minimumScope->typeParameters.find(util::tokenstring(assignedType));
							if (it == minimumScope->typeParameters.end())
							{
								minimumScope = minimumScope->parentScope;
							}
							else
							{
								auto fields = it->second;
								bool typeExists = false;
								for (const auto& field : fields)
								{
									if (util::tokenstring(field.identifier) == fieldName)
									{
										assignedType = field.type;
										typeExists = true;
									}
								}
								if (!typeExists)
								{
									return { TokenType::ERROR, assignedType.start, assignedType.length, assignedType.line };
								}
							}
						}
					}

					Token valueType = expressionTypeInfo((ExpressionNode*)assignmentNode->value.get(), currentScope, expected);

					if (valueType.type == TokenType::ERROR) std::cout << "Error: expression type does not match assigned type!" << std::endl;

					if (util::tokenstring(valueType) != util::tokenstring(assignedType))
					{
						return { TokenType::ERROR, valueType.start, valueType.length, valueType.line };
					}
					
					return assignedType;
				}
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