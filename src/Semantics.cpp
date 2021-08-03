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

			if (strcmp(left.c_str(),"double") == 0)
			{
				return lhs;
			}

			else if (strcmp(left.c_str(),"float") == 0)
			{
				if (strcmp(right.c_str(),"double") == 0)
					return rhs;
				return lhs;
			}

			else if (strcmp(left.c_str(),"long") == 0
				|| strcmp(left.c_str(),"ulong") == 0)
			{
				if (strcmp(right.c_str(),"double") == 0
					|| strcmp(right.c_str(),"float") == 0)
					return rhs;
				return lhs;
			}
			else if (strcmp(left.c_str(),"int") == 0
				|| strcmp(left.c_str(),"uint") == 0)
			{
				if (strcmp(right.c_str(),"double") == 0
					|| strcmp(right.c_str(),"float") == 0
					|| strcmp(right.c_str(),"long") == 0
					|| strcmp(right.c_str(),"ulong") == 0)
					return rhs;
				return lhs;
			}
			else if (strcmp(left.c_str(),"short") == 0
				|| strcmp(left.c_str(),"ushort") == 0)
			{
				if (strcmp(right.c_str(),"double") == 0
					|| strcmp(right.c_str(),"float") == 0
					|| strcmp(right.c_str(),"long") == 0
					|| strcmp(right.c_str(),"int") == 0
					|| strcmp(right.c_str(),"ulong") == 0
					|| strcmp(right.c_str(),"uint") == 0)
					return rhs;
				return lhs;
			}
			else if (strcmp(left.c_str(),"byte") == 0
				|| strcmp(left.c_str(),"ubyte") == 0)
			{
				if (strcmp(right.c_str(),"double") == 0
					|| strcmp(right.c_str(),"float") == 0
					|| strcmp(right.c_str(),"long") == 0
					|| strcmp(right.c_str(),"int") == 0
					|| strcmp(right.c_str(),"short") == 0
					|| strcmp(right.c_str(),"ulong") == 0
					|| strcmp(right.c_str(),"uint") == 0
					|| strcmp(right.c_str(),"ushort") == 0)
					return rhs;
				return lhs;
			}

			else if(strcmp(left.c_str(),"char") == 0)
			{
				if (strcmp(right.c_str(),"char") != 0)
					return { TokenType::ERROR, nullptr, 0, lhs.line };
			}

			return { TokenType::ERROR, nullptr, 0, lhs.line };
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
			panicMode = false;
		}
		for (const auto& declaration : ast->declarations)
		{
			auto scope = util::getScope((ParseNode*)declaration.get(), currentScope);
			hadError |= functionValidator((ParseNode*)declaration.get());
		}
		ast->hadError = hadError;
		for (const auto& error : errorQueue)
		{
			std::cout << "Error on line " << error.line << ": " << util::tokenstring(error).c_str() << std::endl;
		}
		return ast;
	}

	Token Semantics::pushError(const char* msg, int line)
	{
		panicMode = true;
		Token error = { TokenType::ERROR, msg, strlen(msg), line };
		errorQueue.push_back(error);
		return error;
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
							pushError("field already included", field.identifier.line);
							hadError = true;
						}
					}

					currentScope->typeParameters.emplace(typeName, typeNode->fields);
				}
				else
				{
					std::string msg = { typeName.c_str() };
					msg.append(" already defined.");
					char* c_msg = new char[msg.length() + 1];
					strcpy(c_msg, msg.c_str());
					pushError(c_msg, typeNode->typeDefined.line);
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
					std::string msg = { funcName.c_str() };
					msg.append(" already defined.");
					char* c_msg = new char[msg.length() + 1];
					strcpy(c_msg, msg.c_str());
					pushError(c_msg, funcNode->identifier.line);
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
						std::string msg = { paramName.c_str() };
						msg.append(" already defined.");
						char* c_msg = new char[msg.length() + 1];
						strcpy(c_msg, msg.c_str());
						pushError(c_msg, parameter.identifier.line);
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
					std::string msg = { varName.c_str() };
					msg.append(" already defined.");
					char* c_msg = new char[msg.length() + 1];
					strcpy(c_msg, msg.c_str());
					pushError(c_msg, varNode->identifier.line);
					hadError = true;
				}

				if(varNode->value)
				{
					auto valueType = expressionTypeInfo((ExpressionNode*)varNode->value.get(), currentScope);

					if (util::tokenstring(valueType) != util::tokenstring(varNode->type))
					{
						std::cout << "Error: value is of type " << util::tokenstring(valueType) << ", assigned to variable of type " << util::tokenstring(varNode->type) << std::endl;
						std::string msg = { "value is of type " };
						msg.append(util::tokenstring(valueType));
						msg.append(", assigned to variable of type ");
						msg.append(util::tokenstring(varNode->type));
						msg.append(".");
						char* c_msg = new char[msg.length() + 1];
						strcpy(c_msg, msg.c_str());
						pushError(c_msg, varNode->value->line());
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

	bool Semantics::functionValidator(ParseNode* node, bool inFunction)
	{
		switch (node->nodeType())
		{
			case NodeType::FunctionDeclaration:
			{
				FunctionDeclarationNode* funcNode = (FunctionDeclarationNode*)node;
				bool blockHasError = functionValidator(funcNode->body.get(), true);
				bool allBranchesReturn = false;
				if (util::tokenstring(funcNode->type).c_str() != "void")
					allBranchesReturn = hasReturnPath(funcNode->body.get(), nullptr, funcNode->type);
				else allBranchesReturn = true;
				return !allBranchesReturn || blockHasError;
			}
			case NodeType::ForStatement:
			{
				ForStatementNode* forNode = (ForStatementNode*)node;
				return functionValidator(forNode->statement.get(), inFunction);
			}
			case NodeType::WhileStatement:
			{
				WhileStatementNode* whileNode = (WhileStatementNode*)node;
				return functionValidator(whileNode->doStatement.get(), inFunction);
			}
			case NodeType::IfStatement:
			{
				IfStatementNode* ifNode = (IfStatementNode*)node;
				bool hasError = functionValidator(ifNode->thenStatement.get(), inFunction);
				if (ifNode->elseStatement)
					hasError |= functionValidator(ifNode->elseStatement.get(), inFunction);
				return hasError;
			}
			case NodeType::ReturnStatement:
			{
				return !inFunction;
			}
			case NodeType::Block:
			{
				BlockNode* blockNode = (BlockNode*)node;
				bool hasError = false;
				for (const auto& declaration : blockNode->declarations)
				{
					hasError |= functionValidator(declaration.get(), inFunction);
				}
				return hasError;
			}
			default:
				return false;
		}
	}

	bool Semantics::hasReturnPath(ParseNode* node, std::shared_ptr<ScopeNode> currentScope, Token returnType)
	{
		switch (node->nodeType())
		{
			case NodeType::Block:
			{
				BlockNode* blockNode = (BlockNode*)node;
				bool allPathsReturn = false;
				for (const auto& declaration : blockNode->declarations)
				{
					allPathsReturn |= hasReturnPath(declaration.get(), blockNode->scope, returnType);
				}
				return allPathsReturn;
			}
			case NodeType::ForStatement:
			{
				ForStatementNode* forNode = (ForStatementNode*)node;
				return hasReturnPath(forNode->statement.get(),currentScope, returnType);
			}
			case NodeType::WhileStatement:
			{
				WhileStatementNode* whileNode = (WhileStatementNode*)node;
				return hasReturnPath(whileNode->doStatement.get(),currentScope, returnType);
			}
			case NodeType::IfStatement:
			{
				IfStatementNode* ifNode = (IfStatementNode*)node;
				if (!ifNode->elseStatement) return false;
				bool allPathsReturn = hasReturnPath(ifNode->thenStatement.get(),currentScope, returnType);
				allPathsReturn |= hasReturnPath(ifNode->elseStatement.get(),currentScope, returnType);
				return allPathsReturn;
			}
			case NodeType::ReturnStatement:
			{
				ReturnStatementNode* returnNode = (ReturnStatementNode*)node;
				Token statementType = expressionTypeInfo(returnNode->returnValue.get(), currentScope);
				if (statementType.type == TokenType::ERROR) return true;
				if (util::tokenstring(statementType) == util::tokenstring(returnType))
				{
					return true;
				}
				else 
				{
					std::string msg = { "return statement expected type " };
					msg.append(util::tokenstring(returnType));
					msg.append(", got ");
					msg.append(util::tokenstring(statementType));
					char* c_msg = new char[msg.length() + 1];
					strcpy(c_msg, msg.c_str());
					pushError(c_msg, returnNode->returnValue->line());
					return false;
				}
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
							std::string msg = { name };
							msg.append(" not defined.");
							char* c_msg = new char[msg.length() + 1];
							strcpy(c_msg, msg.c_str());
							return pushError(c_msg, callNode->primary.line);
						}
						if (s)
						{
							if (expected.type != TokenType::ERROR)
							{
								if (util::tokenstring(s->type) != util::tokenstring(expected))
								{
									std::string msg = { "expected type " };
									msg.append(util::tokenstring(expected));
									msg.append(", actual type ");
									msg.append(util::tokenstring(s->type));
									msg.append(".");
									char* c_msg = new char[msg.length() + 1];
									strcpy(c_msg, msg.c_str());
									return pushError(c_msg, callNode->primary.line);
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
								std::string msg = { "expected type " };
								msg.append(util::tokenstring(expected));
								msg.append(", actual type bool.");
								char* c_msg = new char[msg.length() + 1];
								strcpy(c_msg, msg.c_str());
								return pushError(c_msg, callNode->primary.line);
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
								std::string msg = { "expected type " };
								msg.append(util::tokenstring(expected));
								msg.append(", actual type float.");
								char* c_msg = new char[msg.length() + 1];
								strcpy(c_msg, msg.c_str());
								return pushError(c_msg, callNode->primary.line);
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
								std::string msg = { "expected type " };
								msg.append(util::tokenstring(expected));
								msg.append(", actual type double.");
								char* c_msg = new char[msg.length() + 1];
								strcpy(c_msg, msg.c_str());
								return pushError(c_msg, callNode->primary.line);
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
								std::string msg = { "expected type " };
								msg.append(util::tokenstring(expected));
								msg.append(", actual type int.");
								char* c_msg = new char[msg.length() + 1];
								strcpy(c_msg, msg.c_str());
								return pushError(c_msg, callNode->primary.line);
							}
						}
						return { TokenType::TYPE, "int", 3, callNode->primary.line };
					}
					case TokenType::CHAR: return { TokenType::TYPE, "char", 4, callNode->primary.line };
					case TokenType::STRING:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("string" != util::tokenstring(expected))
							{
								std::string msg = { "expected type " };
								msg.append(util::tokenstring(expected));
								msg.append(", actual type string.");
								char* c_msg = new char[msg.length() + 1];
								strcpy(c_msg, msg.c_str());
								return pushError(c_msg, callNode->primary.line);
							}
						}
						return { TokenType::TYPE, "string", 6, callNode->primary.line };
					}
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
						std::string msg = { "expected type " };
						msg.append(util::tokenstring(expected));
						msg.append(", actual type ");
						msg.append(util::tokenstring(unaryValue));
						msg.append(".");
						char* c_msg = new char[msg.length() + 1];
						strcpy(c_msg, msg.c_str());
						return pushError(c_msg, unaryNode->op.line);
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

					else if (leftType.type != TokenType::ERROR && rightType.type != TokenType::ERROR)
					{
						std::string msg = { "type mismatch " };
						msg.append(util::tokenstring(leftType));
						msg.append(" and ");
						msg.append(util::tokenstring(rightType));
						msg.append(".");
						char* c_msg = new char[msg.length() + 1];
						strcpy(c_msg, msg.c_str());
						exprType = pushError(c_msg, leftType.line);
					}

					if (leftType.type == TokenType::ERROR) exprType = leftType;
					else if (rightType.type == TokenType::ERROR) exprType = rightType;
				}

				switch (binaryNode->op.type)
				{
					case TokenType::EQUAL_EQUAL:
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
									std::string msg = { "expected type " };
									msg.append(util::tokenstring(expected));
									msg.append(", actual type bool.");
									char* c_msg = new char[msg.length() + 1];
									strcpy(c_msg, msg.c_str());
									return pushError(c_msg, leftType.line);
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
									std::string msg = { "expected type " };
									msg.append(util::tokenstring(expected));
									msg.append(", actual type ");
									msg.append(util::tokenstring(exprType));
									msg.append(".");
									char* c_msg = new char[msg.length() + 1];
									strcpy(c_msg, msg.c_str());
									return pushError(c_msg, leftType.line);
								}
							}
							return exprType;
						}
						else return exprType;
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
					
					return pushError("can only assign to a named variable.", assignmentNode->identifier->line());
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
					std::string msg = { name };
					msg.append(" has not been declared.");
					char* c_msg = new char[msg.length() + 1];
					strcpy(c_msg, msg.c_str());
					return pushError(c_msg, assignmentNode->identifier->line());
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
									
									return pushError("type does not exist.", assignmentNode->identifier->line());
								}
							}
						}
					}

					Token valueType = expressionTypeInfo((ExpressionNode*)assignmentNode->value.get(), currentScope, expected);

					if (valueType.type == TokenType::ERROR) std::cout << "Error: expression type does not match assigned type!" << std::endl;

					if (valueType.type != TokenType::ERROR && util::tokenstring(valueType) != util::tokenstring(assignedType))
					{
						std::string msg = { "expected type " };
						msg.append(util::tokenstring(expected));
						msg.append(", actual type ");
						msg.append(util::tokenstring(valueType));
						msg.append(".");
						char* c_msg = new char[msg.length() + 1];
						strcpy(c_msg, msg.c_str());
						return pushError(c_msg, assignmentNode->value->line());
					}
					
					return assignedType;
				}
			}
			case ExpressionNode::ExpressionType::FieldCall:
			{
				FieldCallNode* fieldCallNode = (FieldCallNode*)node;

				Token parentType = expressionTypeInfo((ExpressionNode*)fieldCallNode->left.get(), currentScope);

				auto scope = currentScope;
				std::string typeID = util::tokenstring(parentType);
				while (scope != nullptr)
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
						return pushError("type valid, but does not contain field.", fieldCallNode->field.line);
					}
				}
				return pushError("type not found.", fieldCallNode->field.line);
			}
			case ExpressionNode::ExpressionType::FunctionCall:
			{
				FunctionCallNode* functionCallNode = (FunctionCallNode*)node;

				std::string funcName = functionCallNode->resolveName();
				std::string name;
				//TODO: support calling functions from modules

				auto scope = currentScope;
				bool found = false;
				bool inModule = false;

				if (funcName.find(".") != std::string::npos)
				{
					name = std::string(funcName, 0, funcName.find("."));
					inModule = true;
				}
				else
				{
					name = funcName;
					inModule = false;
				}
				if (!inModule)
				{
					while (!found & scope != nullptr)
					{
						if (scope->symbols.find(funcName) == scope->symbols.end())
						{
							scope = scope->parentScope;
						}
						else
						{
							std::vector<parameter> parameters = scope->functionParameters.at(funcName);
							
							std::vector<Token> args;

							for (const auto& argument : functionCallNode->arguments)
							{
								args.push_back(expressionTypeInfo(argument.get(), currentScope));
							}

							if(args.size() != parameters.size())
							{
								std::string msg = { "expected " };
								msg.append(std::to_string(parameters.size()));
								msg.append("parameters, received ");
								msg.append(std::to_string(args.size()));
								msg.append(".");
								char* c_msg = new char[msg.length() + 1];
								strcpy(c_msg, msg.c_str());
								return pushError(c_msg, functionCallNode->line());
							}

							for(int i = 0; i < args.size(); i++)
							{
								if(util::tokenstring(args[i]) != util::tokenstring(parameters[i].type))
								{
									std::string msg = { "expected type " };
									msg.append(util::tokenstring(parameters[i].type));
									msg.append(", actual type ");
									msg.append(util::tokenstring(args[i]));
									msg.append(".");
									char* c_msg = new char[msg.length() + 1];
									strcpy(c_msg, msg.c_str());
									return pushError(c_msg, functionCallNode->line());
								}
							}

							return scope->symbols.find(funcName)->second.type;
						}
					}
				}
			}
		}
	}
}