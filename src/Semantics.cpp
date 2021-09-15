#include "Semantics.h"

#include <unordered_set>

namespace ash
{

	std::shared_ptr<ProgramNode> Semantics::findSymbols(std::shared_ptr<ProgramNode> ast)
	{
		std::shared_ptr<ScopeNode> currentScope = ast->globalScope = std::make_shared<ScopeNode>();
		currentScope->scopeIndex = scopeCount++;
		scopes.push_back(currentScope);

		bool hadError = false;
		for(const auto& declaration: ast->declarations)
		{
			auto scope = getScope((ParseNode*)declaration.get(), currentScope);
			hadError |= enterNode((ParseNode*)declaration.get(), scope);
			panicMode = false;
		}
		for (const auto& declaration : ast->declarations)
		{
			hadError |= functionValidator((ParseNode*)declaration.get());
		}
		std::vector<std::shared_ptr<DeclarationNode>> newDeclarations;
		for(const auto& declaration : ast->declarations)
		{
			newDeclarations.push_back(linearizeAST((ParseNode*)declaration.get(), newDeclarations, ast->globalScope));
		}
		ast->declarations = newDeclarations;
		ast->hadError = hadError;
		for (const auto& error : errorQueue)
		{
			std::cout << "Error on line " << error.line << ": " << error.string << std::endl;
		}
		return ast;
	}

	Token Semantics::pushError(std::string msg, int line)
	{
		panicMode = true;
		Token error = { TokenType::ERROR, msg, line };
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
					auto scope = getScope((ParseNode*)declaration.get(), currentScope);
					hadError |= enterNode((ParseNode*)declaration.get(), scope);
				}
				return hadError;
			}
			case NodeType::TypeDeclaration:
			{
				TypeDeclarationNode* typeNode = (TypeDeclarationNode*)node;
				std::string typeName = typeNode->typeDefined.string;
				Symbol s = { typeName, category::Type, typeNode->typeDefined };
				bool hadError = false;
				if (currentScope->symbols.find(typeName) == currentScope->symbols.end())
				{
					currentScope->symbols.emplace(typeName, s);
					std::unordered_set<std::string> typeFields;

					for (const auto& field : typeNode->fields)
					{
						auto id = field.identifier.string;

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
					pushError(msg, typeNode->typeDefined.line);
					hadError = true;
				}
				return hadError;
			}
			case NodeType::FunctionDeclaration:
			{
				FunctionDeclarationNode* funcNode = (FunctionDeclarationNode*)node;
				std::string funcName = funcNode->identifier.string;
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
					pushError(msg, funcNode->identifier.line);
					hadError = true;
				}
				auto blockScope = std::make_shared<ScopeNode>();
				blockScope->parentScope = currentScope;
				blockScope->scopeIndex = scopeCount++;
				scopes.push_back(blockScope);
				for(const auto& parameter : funcNode->parameters)
				{
					std::string paramName = parameter.identifier.string;
					Symbol t = { paramName, category::Variable, parameter.type };
					if(blockScope->symbols.find(paramName) == blockScope->symbols.end())
					{
						blockScope->symbols.emplace(paramName, t);
					}
					else
					{
						std::string msg = { paramName.c_str() };
						msg.append(" already defined.");
						pushError(msg, parameter.identifier.line);
						hadError = true;
					}
				}

				hadError |= enterNode((ParseNode*)funcNode->body.get(), blockScope);
				return hadError;
			}

			case NodeType::VariableDeclaration:
			{
				VariableDeclarationNode* varNode = (VariableDeclarationNode*)node;
				std::string varName = varNode->identifier.string;
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
					pushError(msg, varNode->identifier.line);
					hadError = true;
				}

				if(varNode->value)
				{
					auto valueType = expressionTypeInfo((ExpressionNode*)varNode->value.get(), currentScope);

					if (valueType.string != varNode->type.string)
					{
						std::cout << "Error: value is of type " << valueType.string << ", assigned to variable of type " << varNode->type.string << std::endl;
						std::string msg = { "value is of type " };
						msg.append(valueType.string);
						msg.append(", assigned to variable of type ");
						msg.append(varNode->type.string);
						msg.append(".");
						pushError(msg, varNode->value->line());
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
				if(forNode->statement) forScope = getScope(forNode->statement.get(), currentScope);
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
				if (ifNode->thenStatement) thenScope = getScope(ifNode->thenStatement.get(), currentScope);
				if (ifNode->elseStatement) elseScope = getScope(ifNode->elseStatement.get(), currentScope);
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
				if (whileNode->doStatement) whileScope = getScope(whileNode->doStatement.get(), currentScope);
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
				if (funcNode->type.string.compare("void") != 0)
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
				if (statementType.string.compare(returnType.string) == 0)
				{
					return true;
				}
				else 
				{
					std::string msg = { "return statement expected type " };
					msg.append(returnType.string);
					msg.append(", got ");
					msg.append(statementType.string);
					pushError(msg, returnNode->returnValue->line());
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
						std::string name = callNode->primary.string;
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
							return pushError(msg, callNode->primary.line);
						}
						if (s)
						{
							if (expected.type != TokenType::ERROR)
							{
								if (s->type.string != expected.string)
								{
									std::string msg = { "expected type " };
									msg.append(expected.string);
									msg.append(", actual type ");
									msg.append(s->type.string);
									msg.append(".");
									return pushError(msg, callNode->primary.line);
								}
							}
							callNode->primaryType = s->type;
							return s->type;
						}
					}
					case TokenType::TRUE:
					case TokenType::FALSE:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("bool" != expected.string)
							{
								std::string msg = { "expected type " };
								msg.append(expected.string);
								msg.append(", actual type bool.");
								return pushError(msg, callNode->primary.line);
							}
						}
						callNode->primaryType = { TokenType::TYPE, "bool", callNode->primary.line };
						return { TokenType::TYPE, "bool", callNode->primary.line };
					}
					case TokenType::FLOAT: 
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("float" != expected.string)
							{
								std::string msg = { "expected type " };
								msg.append(expected.string);
								msg.append(", actual type float.");
								return pushError(msg, callNode->primary.line);
							}
						}
						callNode->primaryType = { TokenType::TYPE, "float", callNode->primary.line };
						return { TokenType::TYPE, "float", callNode->primary.line }; 
					}
					case TokenType::DOUBLE:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("double" != expected.string)
							{
								std::string msg = { "expected type " };
								msg.append(expected.string);
								msg.append(", actual type double.");
								return pushError(msg, callNode->primary.line);
							}
						}
						callNode->primaryType = { TokenType::TYPE, "double", callNode->primary.line };
						return { TokenType::TYPE, "double", callNode->primary.line }; 
					}
					case TokenType::INT:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("byte" != expected.string &&
								"ubyte" != expected.string && 
								"short" != expected.string && 
								"ushort" != expected.string && 
								"int" != expected.string && 
								"uint" != expected.string && 
								"long" != expected.string && 
								"ulong" != expected.string
								)
							{
								std::string msg = { "expected type " };
								msg.append(expected.string);
								msg.append(", actual type int.");
								return pushError(msg, callNode->primary.line);
							}
						}
						callNode->primaryType = { TokenType::TYPE, "int", callNode->primary.line };
						return { TokenType::TYPE, "int", callNode->primary.line };
					}
					case TokenType::CHAR: 
					{
						callNode->primaryType = { TokenType::TYPE, "char", callNode->primary.line };
						return { TokenType::TYPE, "char",  callNode->primary.line };
					}
					case TokenType::STRING:
					{
						if (expected.type != TokenType::ERROR)
						{
							if ("string" != expected.string)
							{
								std::string msg = { "expected type " };
								msg.append(expected.string);
								msg.append(", actual type string.");
								return pushError(msg, callNode->primary.line);
						
							}
						}
						callNode->primaryType = { TokenType::TYPE, "string", callNode->primary.line };
						return { TokenType::TYPE, "string",  callNode->primary.line };
					}
				}
			}
			case ExpressionNode::ExpressionType::Unary:
			{
				UnaryNode* unaryNode = (UnaryNode*)node;
				Token unaryValue = expressionTypeInfo((ExpressionNode*)unaryNode->unary.get(), currentScope);
				if (expected.type != TokenType::ERROR)
				{
					if (unaryValue.string != expected.string)
					{
						std::string msg = { "expected type " };
						msg.append(expected.string);
						msg.append(", actual type ");
						msg.append(unaryValue.string);
						msg.append(".");
						return pushError(msg, unaryNode->op.line);
					}
				}
				unaryNode->unaryType = unaryValue;
				return unaryValue;
			}
			case ExpressionNode::ExpressionType::Binary:
			{
				BinaryNode* binaryNode = (BinaryNode*)node;
				Token exprType;
				Token leftType = expressionTypeInfo((ExpressionNode*)binaryNode->left.get(), currentScope);
				Token rightType = expressionTypeInfo((ExpressionNode*)binaryNode->right.get(), currentScope);
				binaryNode->leftType = leftType;
				binaryNode->rightType = rightType;
				if (util::isBasic(leftType) && util::isBasic(rightType))
				{
					exprType =  util::resolveBasicTypes(leftType, rightType);
				}
				else
				{
					if (leftType.string == rightType.string)
					{
						exprType =  leftType;
					}

					else if (leftType.type != TokenType::ERROR && rightType.type != TokenType::ERROR)
					{
						std::string msg = { "type mismatch " };
						msg.append(leftType.string);
						msg.append(" and ");
						msg.append(rightType.string);
						msg.append(".");
						exprType = pushError(msg, leftType.line);
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
								if (expected.string.compare("bool") != 0)
								{
									std::string msg = { "expected type " };
									msg.append(expected.string);
									msg.append(", actual type bool.");
									return pushError(msg, leftType.line);
								}
							}

							exprType = { TokenType::TYPE, "bool", leftType.line };
						}
						binaryNode->binaryType = exprType;
						return exprType;
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
								if (expected.string.compare("bool") != 0)
								{
									std::string msg = { "expected type " };
									msg.append(expected.string);
									msg.append(", actual type ");
									msg.append(exprType.string);
									msg.append(".");
									return pushError(msg, leftType.line);
								}
							}
							
						}
						binaryNode->binaryType = exprType;
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
					return pushError(msg, assignmentNode->identifier->line());
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
							auto it = minimumScope->typeParameters.find(assignedType.string);
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
									if (field.identifier.string == fieldName)
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

					if (valueType.type != TokenType::ERROR && valueType.string != assignedType.string)
					{
						std::string msg = { "expected type " };
						msg.append(expected.string);
						msg.append(", actual type ");
						msg.append(valueType.string);
						msg.append(".");
						return pushError(msg, assignmentNode->value->line());
					}
					assignmentNode->assignmentType = assignedType;
					return assignedType;
				}
			}
			case ExpressionNode::ExpressionType::FieldCall:
			{
				FieldCallNode* fieldCallNode = (FieldCallNode*)node;

				Token parentType = expressionTypeInfo((ExpressionNode*)fieldCallNode->left.get(), currentScope);

				auto scope = currentScope;
				std::string typeID = parentType.string;
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
							if (field.identifier.string == fieldCallNode->field.string)
							{
								fieldCallNode->fieldType = field.type;
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
					while (!found && scope != nullptr)
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
								return pushError(msg, functionCallNode->line());
							}

							for(int i = 0; i < args.size(); i++)
							{
								if(args[i].string != parameters[i].type.string)
								{
									std::string msg = { "expected type " };
									msg.append(parameters[i].type.string);
									msg.append(", actual type ");
									msg.append(args[i].string);
									msg.append(".");
									return pushError(msg, functionCallNode->line());
								}
							}
							functionCallNode->functionType = scope->symbols.find(funcName)->second.type;
							return scope->symbols.find(funcName)->second.type;
						}
					}
					std::string msg = { "function " };
					msg.append(name);
					msg.append(" not defined!");
					return pushError(msg, functionCallNode->line());
				}
			}
		}
	}

	std::shared_ptr<DeclarationNode> Semantics::linearizeAST(ParseNode* node, std::vector<std::shared_ptr<DeclarationNode>>& currentBlock, std::shared_ptr<ScopeNode> currentScope)
	{
		switch(node->nodeType())
		{
			case NodeType::Block:
			{
				auto blockNode = (BlockNode*)node;
				std::vector<std::shared_ptr<DeclarationNode>> thisBlock;
				for(const auto& declaration : blockNode->declarations)
				{
					thisBlock.push_back(linearizeAST((ParseNode*)declaration.get(), thisBlock, blockNode->scope));
				}
				auto result = std::make_shared<BlockNode>();
				result->declarations = thisBlock;
				result->scope = blockNode->scope;
				return result;
			}
			case NodeType::VariableDeclaration:
			{
				auto varNode = (VariableDeclarationNode*)node;
				auto result = std::make_shared<VariableDeclarationNode>();

				result->identifier = varNode->identifier;
				result->type = varNode->type;
				result->usign = varNode->usign;
				result->value = pruneBinaryExpressions(varNode->value.get(), currentBlock, currentScope);
				return result;
			}
			case NodeType::TypeDeclaration:
			{
				auto typeNode = (TypeDeclarationNode*)node;
				auto result = std::make_shared<TypeDeclarationNode>();
				result->typeDefined = typeNode->typeDefined;
				result->fields = typeNode->fields;
				return result;
			}
			case NodeType::FunctionDeclaration:
			{
				auto funcNode = (FunctionDeclarationNode*)node;
				auto result = std::make_shared<FunctionDeclarationNode>();
				result->usign = funcNode->usign;
				result->type = funcNode->type;
				result->identifier = funcNode->identifier;
				result->parameters = funcNode->parameters;
				result->body = std::dynamic_pointer_cast<BlockNode>(linearizeAST(funcNode->body.get(), currentBlock, currentScope));
				return result;
			}
			case NodeType::WhileStatement:
			{
				auto whileNode = (WhileStatementNode*)node;
				auto result = std::make_shared<WhileStatementNode>();

				result->condition = pruneBinaryExpressions(whileNode->condition.get(), currentBlock, currentScope);
				std::shared_ptr<BlockNode> stmtBlock = std::make_shared<BlockNode>();
				std::vector<std::shared_ptr<DeclarationNode>> blockDeclarations;
				stmtBlock->declarations = blockDeclarations;
				if (whileNode->doStatement->nodeType() != NodeType::Block)
				{
					stmtBlock->scope = std::make_shared<ScopeNode>();
					blockDeclarations.push_back(linearizeAST((ParseNode*)whileNode->doStatement.get(), blockDeclarations, stmtBlock->scope));
				}
				else
				{
					auto doStmtBlock = (BlockNode*)whileNode->doStatement.get();
					stmtBlock = std::dynamic_pointer_cast<BlockNode>(linearizeAST((ParseNode*)doStmtBlock, blockDeclarations, doStmtBlock->scope));
				}
				
				result->doStatement = stmtBlock;
				return result;
			}
			case NodeType::ForStatement:
			{
				auto forNode = (ForStatementNode*)node;
				auto result = std::make_shared<ForStatementNode>();
				result->declaration = std::dynamic_pointer_cast<ParseNode>(linearizeAST(forNode->declaration.get(), currentBlock, currentScope));
				result->conditional = pruneBinaryExpressions(forNode->conditional.get(), currentBlock, currentScope);
				result->increment = pruneBinaryExpressions(forNode->increment.get(), currentBlock, currentScope);
				std::shared_ptr<BlockNode> stmtBlock = std::make_shared<BlockNode>();
				std::vector<std::shared_ptr<DeclarationNode>> stmtDeclarations;
				if(forNode->statement->nodeType() != NodeType::Block)
				{
					stmtBlock->scope = std::make_shared<ScopeNode>();
					stmtDeclarations.push_back(linearizeAST((ParseNode*)forNode->statement.get(), stmtDeclarations, stmtBlock->scope));
				}
				else
				{
					auto forStmtBlock = (BlockNode*)forNode->statement.get();
					stmtBlock = std::dynamic_pointer_cast<BlockNode>(linearizeAST((ParseNode*)forStmtBlock, stmtDeclarations, forStmtBlock->scope));
				}
				result->statement = stmtBlock;
				return result;
			}
			case NodeType::IfStatement:
			{
				auto ifNode = (IfStatementNode*)node;
				auto result = std::make_shared<IfStatementNode>();

				result->condition = pruneBinaryExpressions(ifNode->condition.get(), currentBlock, currentScope);
				std::shared_ptr<BlockNode> thenBlock = std::make_shared<BlockNode>();
				std::vector<std::shared_ptr<DeclarationNode>> thenDeclarations;
				thenBlock->declarations = thenDeclarations;
				if(ifNode->thenStatement->nodeType() != NodeType::Block)
				{
					thenBlock->scope = std::make_shared<ScopeNode>();
					thenDeclarations.push_back(linearizeAST((ParseNode*)ifNode->thenStatement.get(), thenDeclarations, thenBlock->scope));
				}
				else
				{
					auto thenStmtBlock = (BlockNode*)ifNode->thenStatement.get();
					thenBlock = std::dynamic_pointer_cast<BlockNode>(linearizeAST((ParseNode*)thenStmtBlock, thenDeclarations, thenStmtBlock->scope));
				}
		
				result->thenStatement = thenBlock;
				std::shared_ptr<BlockNode> elseBlock = std::make_shared<BlockNode>();
				std::vector<std::shared_ptr<DeclarationNode>> elseDeclarations;
				elseBlock->declarations = elseDeclarations;
				if(ifNode->elseStatement->nodeType() != NodeType::Block)
				{
					elseBlock->scope = std::make_shared<ScopeNode>();
					elseDeclarations.push_back(linearizeAST((ParseNode*)ifNode->elseStatement.get(), elseDeclarations, elseBlock->scope));
				}
				else
				{
					auto elseStmtBlock = (BlockNode*)ifNode->elseStatement.get();
					elseBlock = std::dynamic_pointer_cast<BlockNode>(linearizeAST((ParseNode*)elseStmtBlock, elseDeclarations, elseStmtBlock->scope));
				}
				result->elseStatement = elseBlock;
				return result;
			}
			case NodeType::ExpressionStatement:
			{
				auto exprNode = (ExpressionStatement*)node;
				auto result = std::make_shared<ExpressionStatement>();

				result->expression = pruneBinaryExpressions(exprNode->expression.get(), currentBlock, currentScope);
				return result;
			}
			case NodeType::ReturnStatement:
			{
				auto returnNode = (ReturnStatementNode*)node;
				auto result = std::make_shared<ReturnStatementNode>();

				result->returnValue = pruneBinaryExpressions(returnNode->returnValue.get(), currentBlock, currentScope);
				return result;
			}
			case NodeType::Expression:
			{
				return std::dynamic_pointer_cast<DeclarationNode>(pruneBinaryExpressions((ExpressionNode*)node, currentBlock, currentScope));
			}
		}
	}

	std::shared_ptr<ExpressionNode> Semantics::pruneBinaryExpressions(ExpressionNode* node, std::vector<std::shared_ptr<DeclarationNode>>& currentBlock, std::shared_ptr<ScopeNode> currentScope)
	{
		switch(node->expressionType())
		{
			case ExpressionNode::ExpressionType::Assignment:
			{
				auto assignmentNode = (AssignmentNode*)node;
				auto result = std::make_shared<AssignmentNode>();
				result->identifier = pruneBinaryExpressions(assignmentNode->identifier.get(), currentBlock, currentScope);
				result->assignmentType = assignmentNode->assignmentType;
				result->value = pruneBinaryExpressions(assignmentNode->value.get(), currentBlock, currentScope);
				return result;
			}
			case ExpressionNode::ExpressionType::Unary:
			{
				auto unaryNode = (UnaryNode*)node;
				auto result = std::make_shared<UnaryNode>();
				result->op = unaryNode->op;
				result->unary = pruneBinaryExpressions(unaryNode->unary.get(), currentBlock, currentScope);
				result->unaryType = unaryNode->unaryType;
				return result;
			}
			case ExpressionNode::ExpressionType::FieldCall:
			{
				auto fieldNode = (FieldCallNode*)node;
				auto result = std::make_shared<FieldCallNode>();
				result->left = pruneBinaryExpressions(fieldNode->left.get(), currentBlock, currentScope);
				result->field = fieldNode->field;
				result->fieldType = fieldNode->fieldType;
				return result;
			}
			case ExpressionNode::ExpressionType::Primary:
			{
				auto primaryNode = (CallNode*)node;
				auto result = std::make_shared<CallNode>();
				result->primary = primaryNode->primary;
				result->primaryType = primaryNode->primaryType;
				return result;
			}
			case ExpressionNode::ExpressionType::FunctionCall:
			{
				auto funcNode = (FunctionCallNode*)node;
				auto result = std::make_shared<FunctionCallNode>();
				result->left = pruneBinaryExpressions(funcNode->left.get(), currentBlock, currentScope);
				std::vector<std::shared_ptr<ExpressionNode>> args;
				args.resize(funcNode->arguments.size());
				for (const auto& arg : funcNode->arguments)
				{
					args.push_back(pruneBinaryExpressions(arg.get(), currentBlock, currentScope));
				}
				result->arguments = args;
				return result;
			}
			case ExpressionNode::ExpressionType::Binary:
			{
				auto binaryNode = (BinaryNode*)node;
				auto result = std::make_shared<BinaryNode>();
				std::shared_ptr<CallNode> leftPrimary;
				std::shared_ptr<CallNode> rightPrimary;
				if(binaryNode->left->expressionType() == ExpressionNode::ExpressionType::Primary)
				{
					leftPrimary = std::dynamic_pointer_cast<CallNode>(binaryNode->left);
				}
				else
				{
					auto temp = std::make_shared<VariableDeclarationNode>();
					temp->value = pruneBinaryExpressions(binaryNode->left.get(), currentBlock, currentScope);
					temp->type = binaryNode->leftType;
					std::string tempName = std::string("#").append(std::to_string(temporaries++));
					temp->identifier = Token{ TokenType::IDENTIFIER, tempName, binaryNode->left->line() };

					currentScope->symbols.emplace(tempName, Symbol{ tempName, category::Variable, temp->type });
					currentBlock.push_back(temp);

					leftPrimary = std::make_shared<CallNode>();
					leftPrimary->primary = temp->identifier;
					leftPrimary->primaryType = temp->type;
				}
				if (binaryNode->right->expressionType() == ExpressionNode::ExpressionType::Primary)
				{
					rightPrimary = std::dynamic_pointer_cast<CallNode>(binaryNode->right);
				}
				else
				{
					auto temp = std::make_shared<VariableDeclarationNode>();
					temp->value = pruneBinaryExpressions(binaryNode->right.get(), currentBlock, currentScope);
					temp->type = binaryNode->leftType;
					std::string tempName = std::string("#").append(std::to_string(temporaries++));
					temp->identifier = Token{ TokenType::IDENTIFIER, tempName, binaryNode->right->line() };

					currentScope->symbols.emplace(tempName, Symbol{ tempName, category::Variable, temp->type });
					currentBlock.push_back(temp);

					rightPrimary = std::make_shared<CallNode>();
					rightPrimary->primary = temp->identifier;
					rightPrimary->primaryType = temp->type;
				}
				result->left = leftPrimary;
				result->leftType = binaryNode->leftType;
				result->right = rightPrimary;
				result->rightType = binaryNode->rightType;
				result->op = binaryNode->op;
				return result;
			}
		}
	}
}