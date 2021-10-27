#include "Compiler.h"
#include "Semantics.h"
#include <string>
#include <chrono>
#include <algorithm>
#include <bitset>

namespace ash
{

	namespace util
	{
		static bool unsigned_int(Token check)
		{
			if (check.string.compare("uint") == 0
				|| check.string.compare("ushort") == 0
				|| check.string.compare("ubyte") == 0
				|| check.string.compare("ulong") == 0
				|| check.string.compare("bool") == 0
				|| check.string.compare("char") == 0)
			{
				return true;
			}
			else return false;
		}

		static bool signed_int(Token check)
		{
			if (check.string.compare("int") == 0
				|| check.string.compare("short") == 0
				|| check.string.compare("byte") == 0
				|| check.string.compare("long") == 0)
			{
				return true;
			}
			else return false;
		}

		static Token renameByScope(Token identifier, std::shared_ptr<ScopeNode> current)
		{
			std::shared_ptr<ScopeNode> varScope = current;
			std::string id = identifier.string;
			while (varScope)
			{
				if (varScope->symbols.find(id) != varScope->symbols.end())
				{
					return { identifier.type, id.append("#").append(std::to_string(varScope->scopeIndex)), identifier.line };
				}
				else
				{
					varScope = varScope->parentScope;
				}
			}
			return { TokenType::ERROR, "identifier not found!", identifier.line };
		}

		static OpCodes typeConversion(Token toConvert, Token resultType)
		{
			if (resultType.string.compare("double") == 0)
			{
				if(toConvert.string.compare("float") == 0)
				{
					return OP_FLOAT_TO_DOUBLE;
				}
				else if(signed_int(toConvert) || unsigned_int(toConvert))
				{
					return OP_INT_TO_DOUBLE;
				}
			}
			else if(resultType.string.compare("float") == 0)
			{
				if(toConvert.string.compare("double") == 0)
				{
					return OP_DOUBLE_TO_FLOAT;
				}
				else if(signed_int(toConvert) || unsigned_int(toConvert))
				{
					return OP_INT_TO_DOUBLE;
				}
			}
			else if(signed_int(resultType) || unsigned_int(resultType))
			{
				if(toConvert.string.compare("float") == 0)
				{
					return OP_FLOAT_TO_INT;
				}
				if(toConvert.string.compare("double") == 0)
				{
					return OP_DOUBLE_TO_INT;
				}
			}
			else
			{
				return OP_HALT;
			}
		}

		template<typename K, typename V>
		void mergeMaps(std::unordered_map<K, V>& dst, std::unordered_map<K, V>& src)
		{
			for (const auto& node : sr)
			{
				if (dst.find(node.first) == graph.end())
				{
					dst.emplace(node);
				}
				else
				{
					auto& edges = dst.at(node.first);
					edges.insert(node.second.begin(), node.second.end());
				}
			}
		}
	}
	bool Compiler::compile(const char* source)
	{

		std::shared_ptr<ProgramNode> ast;
		std::chrono::steady_clock::time_point t1;
		std::chrono::steady_clock::time_point t2;
		{
			Parser parser(source);
			t1 = std::chrono::high_resolution_clock::now();
			ast = parser.parse();
			t2 = std::chrono::high_resolution_clock::now();

			std::cout << "Parsing took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";

			if (ast->hadError) return false;
		}

		{
			Semantics analyzer;

			//ast->print(0);
			t1 = std::chrono::high_resolution_clock::now();
			ast = analyzer.findSymbols(ast);
			t2 = std::chrono::high_resolution_clock::now();

			std::cout << "Semantic analysis took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
			temporaries = analyzer.temporaries;
			if (ast->hadError) return false;
		}
		pseudochunk result;
		{
			t1 = std::chrono::high_resolution_clock::now();
			result = precompile(ast);
			t2 = std::chrono::high_resolution_clock::now();

			std::cout << "Precompiling took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
			std::cout << std::endl;
		}
		//for(const auto& instruction : result.code)
		//{
		//	instruction->print();
		//}
		{
			t1 = std::chrono::high_resolution_clock::now();
			result = correctLoadStore(result);
			t2 = std::chrono::high_resolution_clock::now();
		}
		for (const auto& instruction : result.code)
		{
			instruction->print();
		}
		{
			t1 = std::chrono::high_resolution_clock::now();
			result = allocateRegisters(result);
			t2 = std::chrono::high_resolution_clock::now();

			std::cout << "Register Allocation took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
		}
		for (const auto& instruction : result.code)
		{
			instruction->print();
		}
		
		currentChunk = finalizeCode(result);
		return true;
	}

	pseudochunk Compiler::precompile(std::shared_ptr<ProgramNode> ast)
	{
		pseudochunk chunk;
		currentScope = ast->globalScope;
		for (const auto& declaration : ast->declarations)
		{
			auto nextCode = compileNode((ParseNode*)declaration.get(), nullptr);
			if(nextCode.size()) chunk.code.insert(chunk.code.end(), nextCode.begin(), nextCode.end());
		}
		auto halt = std::make_shared<pseudocode>();
		halt->op = OP_HALT;
		if(chunk.code.size())
		{
			chunk.code.push_back(halt);
		}
			

		return chunk;
	}

	std::vector<std::shared_ptr<assembly>> Compiler::compileNode(ParseNode* node, Token* result)
	{
		switch (node->nodeType())
		{
			case NodeType::IfStatement:
			{
				std::shared_ptr<label> elseLabel = std::make_shared<label>();
				elseLabel->label = jumpLabels++;
				std::shared_ptr<label> exitLabel = std::make_shared<label>();
				exitLabel->label = jumpLabels++;
				std::shared_ptr<relativeJump> thenJump = std::make_shared<relativeJump>();
				thenJump->op = OP_RELATIVE_JUMP_IF_TRUE;
				thenJump->jumpLabel = elseLabel->label;
				std::shared_ptr<relativeJump> exitJump = std::make_shared<relativeJump>();
				exitJump->op = OP_RELATIVE_JUMP;
				exitJump->jumpLabel = exitLabel->label;
				IfStatementNode* ifNode = (IfStatementNode*)node;
				std::vector<std::shared_ptr<assembly>> ifChunk;
				auto hold = currentScope;
				currentScope = ifNode->ifScope;
				auto conditionChunk = compileNode((ParseNode*)ifNode->condition.get(), nullptr);
				ifChunk.insert(ifChunk.end(), conditionChunk.begin(), conditionChunk.end());
				ifChunk.push_back(thenJump);
				auto elseChunk = compileNode((ParseNode*)ifNode->elseStatement.get(), nullptr);
				ifChunk.insert(ifChunk.end(), elseChunk.begin(), elseChunk.end());
				ifChunk.push_back(exitJump);
				ifChunk.push_back(elseLabel);
				auto thenChunk = compileNode((ParseNode*)ifNode->thenStatement.get(), nullptr);
				ifChunk.insert(ifChunk.end(), thenChunk.begin(), thenChunk.end());
				ifChunk.push_back(exitLabel);
				currentScope = hold;
				return ifChunk;
			}

			case NodeType::TypeDeclaration:
			{
				std::vector<std::shared_ptr<assembly>> defChunk;
				TypeDeclarationNode* typeNode = (TypeDeclarationNode*)node;
				std::shared_ptr<TypeMetadata> metadata = std::make_shared<TypeMetadata>();
				auto& typeParameters = currentScope->typeParameters.at(typeNode->typeDefined.string);
				auto typeName = util::renameByScope(typeNode->typeDefined, currentScope);
				typeIDs.emplace(std::pair<std::string, size_t>(typeName.string, types.size()));
				size_t offset = 0;
				for(const parameter& field : typeParameters)
				{
					FieldMetadata fieldData{};
					if (util::isBasic(field.type))
					{
						if (!field.type.string.compare("bool"))
						{
							fieldData.type = FieldType::Bool;
							fieldData.offset = offset;
							fieldData.typeID = -12;
							offset += 1;
						}
						else if (!field.type.string.compare("byte"))
						{
							fieldData.type = FieldType::Byte;
							fieldData.offset = offset;
							fieldData.typeID = -11;
							offset += 1;
						}
						else if (!field.type.string.compare("ubyte"))
						{
							fieldData.type = FieldType::UByte;
							fieldData.offset = offset;
							fieldData.typeID = -10;
							offset += 1;
						}
						else if (!field.type.string.compare("short"))
						{
							fieldData.type = FieldType::Short;
							fieldData.offset = (~(offset & 0x01) + 1) & 0x01;
							fieldData.typeID = -9;
							offset += 2;
						}
						else if (!field.type.string.compare("ushort"))
						{
							fieldData.type = FieldType::UShort;
							fieldData.offset = offset += (~(offset & 0x01) + 1) & 0x01;
							fieldData.typeID = -8;
							offset += 2;
						}
						else if (!field.type.string.compare("int"))
						{
							fieldData.type = FieldType::Int;
							fieldData.offset = offset += (~(offset & 0x03) + 1) & 0x03;
							fieldData.typeID = -7;
							offset += 4;
						}
						else if (!field.type.string.compare("uint"))
						{
							fieldData.type = FieldType::UInt;
							fieldData.offset = offset += (~(offset & 0x03) + 1) & 0x03;
							fieldData.typeID = -6;
							offset += 4;
						}
						else if (!field.type.string.compare("char"))
						{
							fieldData.type = FieldType::Char;
							fieldData.offset = offset += (~(offset & 0x03) + 1) & 0x03;
							fieldData.typeID = -5;
							offset += 4;
						}
						else if (!field.type.string.compare("float"))
						{
							fieldData.type = FieldType::Float;
							fieldData.offset = offset += (~(offset & 0x03) + 1) & 0x03;
							fieldData.typeID = -4;
							offset += 4;
						}
						else if (!field.type.string.compare("long"))
						{
							fieldData.type = FieldType::Long;
							fieldData.offset = offset += (~(offset & 0x07) + 1) & 0x07;
							fieldData.typeID = -3;
							offset += 8;
						}
						else if (!field.type.string.compare("ulong"))
						{
							fieldData.type = FieldType::ULong;
							fieldData.offset = offset += (~(offset & 0x07) + 1) & 0x07;
							fieldData.typeID = -2;
							offset += 8;
						}
						else if (!field.type.string.compare("double"))
						{
							fieldData.type = FieldType::Double;
							fieldData.offset = offset += (~(offset & 0x07) + 1) & 0x07;
							fieldData.typeID = -1;
							offset += 8;
						}
					}
					else
					{
						fieldData.type = FieldType::Struct;
						fieldData.offset = offset += (~(offset & 0x07) + 1) & 0x07;
						auto fieldType = util::renameByScope(field.type, currentScope);
						fieldData.typeID = typeIDs.at(fieldType.string);
						offset += 8;
					}
					metadata->fields.push_back(fieldData);
				}
				types.push_back(metadata);
				return defChunk;
			}

			case NodeType::WhileStatement:
			{
				std::shared_ptr<label> loopLabel = std::make_shared<label>();
				loopLabel->label = jumpLabels++;
				std::shared_ptr<label> conditionLabel = std::make_shared<label>();
				conditionLabel->label = jumpLabels++;
				std::shared_ptr<label> exitLabel = std::make_shared<label>();
				exitLabel->label = jumpLabels++;
				std::shared_ptr<relativeJump> exitJump = std::make_shared<relativeJump>();
				exitJump->op = OP_RELATIVE_JUMP;
				exitJump->jumpLabel = exitLabel->label;
				std::shared_ptr<relativeJump> loopJump = std::make_shared<relativeJump>();
				loopJump->op = OP_RELATIVE_JUMP;
				loopJump->jumpLabel = loopLabel->label;
				std::shared_ptr<relativeJump> conditionJump = std::make_shared<relativeJump>();
				conditionJump->op = OP_RELATIVE_JUMP_IF_TRUE;
				conditionJump->jumpLabel = conditionLabel->label;

				WhileStatementNode* whileNode = (WhileStatementNode*)node;
				auto hold = currentScope;
				currentScope = whileNode->whileScope;
				std::vector<std::shared_ptr<assembly>> whileChunk;
				whileChunk.push_back(loopLabel);
				auto conditionChunk = compileNode((ParseNode*)whileNode->condition.get(), nullptr);
				whileChunk.insert(whileChunk.end(), conditionChunk.begin(), conditionChunk.end());
				whileChunk.push_back(conditionJump);
				whileChunk.push_back(exitJump);
				whileChunk.push_back(conditionLabel);
				auto stmtChunk = compileNode((ParseNode*)whileNode->doStatement.get(), nullptr);
				whileChunk.insert(whileChunk.end(), stmtChunk.begin(), stmtChunk.end());
				whileChunk.push_back(loopJump);
				whileChunk.push_back(exitLabel);
				currentScope = hold;
				return whileChunk;
			}

			case NodeType::ForStatement:
			{
				std::shared_ptr<label> loopLabel = std::make_shared<label>();
				loopLabel->label = jumpLabels++;
				std::shared_ptr<label> conditionLabel = std::make_shared<label>();
				conditionLabel->label = jumpLabels++;
				std::shared_ptr<label> exitLabel = std::make_shared<label>();
				exitLabel->label = jumpLabels++;
				
				std::shared_ptr<relativeJump> loopJump = std::make_shared<relativeJump>();
				loopJump->op = OP_RELATIVE_JUMP;
				loopJump->jumpLabel = loopLabel->label;
				std::shared_ptr<relativeJump> conditionJump = std::make_shared<relativeJump>();
				conditionJump->op = OP_RELATIVE_JUMP_IF_TRUE;
				conditionJump->jumpLabel = conditionLabel->label;
				std::shared_ptr<relativeJump> exitJump = std::make_shared<relativeJump>();
				exitJump->op = OP_RELATIVE_JUMP;
				exitJump->jumpLabel = exitLabel->label;

				ForStatementNode* forNode = (ForStatementNode*)node;
				auto hold = currentScope;
				currentScope = forNode->forScope;
				std::vector<std::shared_ptr<assembly>> forChunk;
				auto declarationChunk = compileNode((ParseNode*)forNode->declaration.get(), nullptr);
				forChunk.insert(forChunk.end(), declarationChunk.begin(), declarationChunk.end());
				forChunk.push_back(loopLabel);
				auto conditionChunk = compileNode((ParseNode*)forNode->conditional.get(), nullptr);
				forChunk.insert(forChunk.end(), conditionChunk.begin(), conditionChunk.end());
				forChunk.push_back(conditionJump);
				forChunk.push_back(exitJump);
				forChunk.push_back(conditionLabel);
				auto stmtChunk = compileNode((ParseNode*)forNode->statement.get(), nullptr);
				forChunk.insert(forChunk.end(), stmtChunk.begin(), stmtChunk.end());
				auto incrementChunk = compileNode((ParseNode*)forNode->increment.get(), nullptr);
				forChunk.insert(forChunk.end(), incrementChunk.begin(), incrementChunk.end());
				forChunk.push_back(loopJump);
				forChunk.push_back(exitLabel);
				currentScope = hold;
				return forChunk;
			}

			case NodeType::Block:
			{
				auto blockNode = (BlockNode*)node;
				auto hold = currentScope;
				currentScope = blockNode->scope;
				std::vector<std::shared_ptr<assembly>> result;
				for(const auto& declaration : blockNode->declarations)
				{
					auto next = compileNode(declaration.get(), nullptr);
					result.insert(result.end(), next.begin(), next.end());
				}
				currentScope = hold;

				return result;
			}

			case NodeType::VariableDeclaration:
			{
				auto varNode = (VariableDeclarationNode*)node;

				std::vector<std::shared_ptr<assembly>> result;

				auto identifier = varNode->identifier;
				if(identifier.string.find('#') == std::string::npos) identifier = util::renameByScope(varNode->identifier, currentScope);

				if(util::isBasic(varNode->type))
				{
					if (varNode->value)
					{ 
						result = compileNode(varNode->value.get(), &identifier);
						if (result.size() && result.back()->type() == Asm::TwoAddr)
						{
							Token value = ((twoAddress*)result.back().get())->A;
							OpCodes operator_ = ((twoAddress*)result.back().get())->op;
							if (value.type == TokenType::IDENTIFIER && operator_ == OP_CONST_LOW && value.string.compare(identifier.string) != 0)
							{
								result.pop_back();
								auto move = std::make_shared<twoAddress>();
								move->op = OP_MOVE;
								move->A = value;
								move->result = identifier;
								result.push_back(move);
							}
						}
					}
				}
				else
				{
					auto varType = util::renameByScope(varNode->type, currentScope);
					std::string temp = std::string("#");
					temp.append(std::to_string(temporaries++));
					Token tempToken = { TokenType::IDENTIFIER, temp, varType.line };
					auto allocType = std::make_shared<twoAddress>();
					allocType->op = OP_CONST_LOW;
					allocType->A = { TokenType::INT,  std::to_string(typeIDs.at(varType.string)), varType.line };
					allocType->result = tempToken;
					result.push_back(allocType);
					auto alloc = std::make_shared<twoAddress>();
					alloc->op = OP_ALLOC;
					alloc->A = tempToken;
					alloc->result = identifier;
					result.push_back(alloc);
					if(varNode->value)
					{
						auto exprResult = compileNode(varNode->value.get(), &identifier);
						if (exprResult.size())
							result.insert(result.end(), exprResult.begin(), exprResult.end());
					}
				}

				return result;
			}

			case NodeType::FunctionDeclaration:
			{
				auto funcNode = (FunctionDeclarationNode*)node;
				std::vector<std::shared_ptr<assembly>> result;
				auto skipFunction = std::make_shared<relativeJump>();
				auto skipLabel = std::make_shared<label>();
				skipLabel->label = jumpLabels++;
				skipFunction->jumpLabel = skipLabel->label;
				skipFunction->op = OP_RELATIVE_JUMP;
				result.push_back(skipFunction);
				auto funcLabel = std::make_shared<functionLabel>();
				funcLabel->name = funcNode->identifier.string;
				result.push_back(funcLabel);
				for(size_t i = 0; i < funcNode->parameters.size(); i++)
				{
					auto constant = std::make_shared<twoAddress>();
					constant->op = OP_CONST_LOW;
					constant->A = { TokenType::INT, std::to_string(i), funcNode->parameters[i].type.line };
					std::string temp{ "#" };
					temp.append(std::to_string(temporaries++));
					Token tempToken = { TokenType::IDENTIFIER, temp, funcNode->parameters[i].type.line };
					constant->result = tempToken;
					result.push_back(constant);
					auto move = std::make_shared<twoAddress>();
					move->op = OP_MOVE_FROM_STACK_FRAME;
					move->A = tempToken;
					move->result = util::renameByScope(funcNode->parameters[i].identifier, funcNode->body->scope);
					result.push_back(move);
				}
				auto body = compileNode(funcNode->body.get(), nullptr);
				result.insert(result.end(), body.begin(), body.end());
				if (result.back()->type() != Asm::pseudocode || std::dynamic_pointer_cast<pseudocode>(result.back())->op != OP_RETURN)
				{
					auto retn = std::make_shared<pseudocode>();
					retn->op = OP_RETURN;
					result.push_back(retn);
				}
				result.push_back(skipLabel);
				return result;
			}

			case NodeType::ReturnStatement:
			{
				auto retNode = (ReturnStatementNode*)node;

				std::vector<std::shared_ptr<assembly>> result;

				if(retNode->returnValue)
				{
					std::string returnRegister("@");
					returnRegister.append(std::to_string(RETURN_REGISTER));
					Token returnRegisterToken = { TokenType::IDENTIFIER, returnRegister, retNode->returnValue->line() };
					result = compileNode(retNode->returnValue.get(), &returnRegisterToken);
				}

				auto ret = std::make_shared<pseudocode>();
				ret->op = OP_RETURN;
				result.push_back(ret);

				return result;
			}

			case NodeType::ExpressionStatement:
			{
				auto exprStmt = (ExpressionStatement*)node;
				return compileNode(exprStmt->expression.get(), nullptr);
			}

			case NodeType::Expression:
			{
				ExpressionNode* exprNode = (ExpressionNode*)node;
				switch (exprNode->expressionType())
				{
					case ExpressionNode::ExpressionType::Binary:
					{
						auto binaryNode = (BinaryNode*)exprNode;
						std::vector<std::shared_ptr<assembly>> chunk;
						Token left = ((CallNode*)binaryNode->left.get())->primary;
						if(left.type != TokenType::IDENTIFIER)
						{
							auto constant = std::make_shared<twoAddress>();
							std::string temp = { "#" };
							temp.append(std::to_string(temporaries++));
							Token tempToken = { TokenType::IDENTIFIER, temp, ((CallNode*)binaryNode->left.get())->line() };
							constant->op = OP_CONST_LOW;
							constant->A = left;
							constant->result = tempToken;
							chunk.push_back(constant);
							left = tempToken;
						}
						else if(left.string.find('#') == std::string::npos)
						{
							left = util::renameByScope(left, currentScope);
						}
						Token right = ((CallNode*)binaryNode->right.get())->primary;
						if (right.type != TokenType::IDENTIFIER)
						{
							auto constant = std::make_shared<twoAddress>();
							std::string temp = { "#" };
							temp.append(std::to_string(temporaries++));
							Token tempToken = { TokenType::IDENTIFIER, temp, ((CallNode*)binaryNode->left.get())->line() };
							constant->op = OP_CONST_LOW;
							constant->A = right;
							constant->result = tempToken;
							chunk.push_back(constant);
							right = tempToken;
						}
						else if( right.string.find('#') == std::string::npos)
						{
							right = util::renameByScope(right, currentScope);
						}
						if(util::isBasic(binaryNode->leftType) && util::isBasic(binaryNode->rightType))
						{
							auto expressionType = util::resolveBasicTypes(binaryNode->leftType, binaryNode->rightType);
							auto& op = binaryNode->op;
							switch(op.type)
							{
								case TokenType::PLUS:
								case TokenType::MINUS:
								case TokenType::SLASH:
								case TokenType::STAR:
								case TokenType::LESS:
								case TokenType::LESS_EQUAL:
								case TokenType::GREATER:
								case TokenType::GREATER_EQUAL:
								case TokenType::EQUAL_EQUAL:
								case TokenType::BANG_EQUAL:
								{
									auto binaryInstruction = std::make_shared<threeAddress>();

									if (binaryNode->leftType.string.compare(expressionType.string) == 0)
									{
										binaryInstruction->A = left;
									}
									else
									{
										auto conversion = std::make_shared<twoAddress>();
										conversion->A = left;
										std::string temp("#");
										temp.append(std::to_string(temporaries++));
										Token tempToken = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
										conversion->result = tempToken;
										conversion->op = util::typeConversion(binaryNode->leftType, expressionType);
										chunk.push_back(conversion);
										binaryInstruction->A = tempToken;
									}
									if (binaryNode->leftType.string.compare(expressionType.string) == 0)
									{
										binaryInstruction->B = right;
									}
									else
									{
										auto conversion = std::make_shared<twoAddress>();
										conversion->A = right;
										std::string temp("#");
										temp.append(std::to_string(temporaries++));
										Token tempToken = { TokenType::IDENTIFIER, temp, binaryNode->right->line() };
										conversion->result = tempToken;
										conversion->op = util::typeConversion(binaryNode->rightType, expressionType);
										chunk.push_back(conversion);
										binaryInstruction->B = tempToken;
									}
									if (result != nullptr)
									{
										binaryInstruction->result = *result;
									}
									else
									{
										std::string temp("#");
										temp.append(std::to_string(temporaries++));
										binaryInstruction->result = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
									}

									if(expressionType.string.compare("double") == 0)
									{
										if (op.type == TokenType::PLUS)
										{
											binaryInstruction->op = OP_DOUBLE_ADD;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::MINUS) 
										{ 
											binaryInstruction->op = OP_DOUBLE_SUB;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::SLASH)
										{
											binaryInstruction->op = OP_DOUBLE_DIV;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::STAR)
										{
											binaryInstruction->op = OP_DOUBLE_MUL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS) 
										{
											binaryInstruction->op = OP_DOUBLE_LESS;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_DOUBLE_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											binaryInstruction->op = OP_DOUBLE_LESS;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::GREATER)
										{
											binaryInstruction->op = OP_DOUBLE_GREATER;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::GREATER_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_DOUBLE_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											binaryInstruction->op = OP_DOUBLE_GREATER;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::EQUAL)
										{
											binaryInstruction->op = OP_DOUBLE_EQUAL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::BANG_EQUAL)
										{
											std::shared_ptr<twoAddress> not;
											not->result = binaryInstruction->result;
											not->op = OP_LOGICAL_NOT;
											std::string temp("#");
											temp.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
											not->A = binaryInstruction->result;
											binaryInstruction->op = OP_DOUBLE_EQUAL;
											chunk.push_back(binaryInstruction);
											chunk.push_back(not);
										}
									}
									else if(expressionType.string.compare("float") == 0)
									{
										if (op.type == TokenType::PLUS)
										{
											binaryInstruction->op = OP_FLOAT_ADD;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::MINUS)
										{
											binaryInstruction->op = OP_FLOAT_SUB;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::SLASH)
										{
											binaryInstruction->op = OP_FLOAT_DIV;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::STAR)
										{
											binaryInstruction->op = OP_FLOAT_MUL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS)
										{
											binaryInstruction->op = OP_FLOAT_LESS;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_FLOAT_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											or_->op = OP_LOGICAL_OR;
											binaryInstruction->op = OP_FLOAT_LESS;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::GREATER)
										{
											binaryInstruction->op = OP_FLOAT_GREATER;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::GREATER_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_FLOAT_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											or_->op = OP_LOGICAL_OR;
											binaryInstruction->op = OP_FLOAT_GREATER;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::EQUAL)
										{
											binaryInstruction->op = OP_FLOAT_EQUAL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::BANG_EQUAL)
										{
											auto not = std::make_shared<twoAddress>();
											not->result = binaryInstruction->result;
											not->op = OP_LOGICAL_NOT;
											std::string temp("#");
											temp.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
											not->A = binaryInstruction->result;
											binaryInstruction->op = OP_FLOAT_EQUAL;
											chunk.push_back(binaryInstruction);
											chunk.push_back(not);
										}
									}
									else if(util::signed_int(expressionType))
									{
										if (op.type == TokenType::PLUS)
										{
											binaryInstruction->op = OP_INT_ADD;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::MINUS)
										{
											binaryInstruction->op = OP_INT_SUB;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::SLASH)
										{
											binaryInstruction->op = OP_SIGN_DIV;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::STAR)
										{
											binaryInstruction->op = OP_SIGN_MUL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS)
										{
											binaryInstruction->op = OP_SIGN_LESS;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_INT_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											or_->op = OP_LOGICAL_OR;
											binaryInstruction->op = OP_SIGN_LESS;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::GREATER)
										{
											binaryInstruction->op = OP_SIGN_GREATER;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::GREATER_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_INT_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											or_->op = OP_LOGICAL_OR;
											binaryInstruction->op = OP_SIGN_GREATER;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::EQUAL)
										{
											binaryInstruction->op = OP_INT_EQUAL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::BANG_EQUAL)
										{
											auto not = std::make_shared<twoAddress>();
											not->result = binaryInstruction->result;
											not->op = OP_LOGICAL_NOT;
											std::string temp("#");
											temp.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
											not->A = binaryInstruction->result;
											binaryInstruction->op = OP_INT_EQUAL;
											chunk.push_back(binaryInstruction);
											chunk.push_back(not);
										}
									}
									else if (util::unsigned_int(expressionType))
									{
										if (op.type == TokenType::PLUS)
										{
											binaryInstruction->op = OP_INT_ADD;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::MINUS)
										{
											binaryInstruction->op = OP_INT_SUB;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::SLASH)
										{
											binaryInstruction->op = OP_UNSIGN_DIV;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::STAR)
										{
											binaryInstruction->op = OP_UNSIGN_MUL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS)
										{
											binaryInstruction->op = OP_UNSIGN_LESS;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::LESS_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_INT_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											or_->op = OP_LOGICAL_OR;
											binaryInstruction->op = OP_UNSIGN_LESS;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::GREATER)
										{
											binaryInstruction->op = OP_UNSIGN_GREATER;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::GREATER_EQUAL)
										{
											auto equal = std::make_shared<threeAddress>();
											auto or_ = std::make_shared<threeAddress>();
											or_->result = binaryInstruction->result;
											equal->A = binaryInstruction->A;
											equal->B = binaryInstruction->B;
											equal->op = OP_INT_EQUAL;
											std::string temp1("#");
											temp1.append(std::to_string(temporaries++));
											equal->result = { TokenType::IDENTIFIER, temp1, binaryNode->left->line() };
											std::string temp2("#");
											temp2.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp2, binaryNode->left->line() };
											or_->A = equal->result;
											or_->B = binaryInstruction->result;
											or_->op = OP_LOGICAL_OR;
											binaryInstruction->op = OP_UNSIGN_GREATER;
											chunk.push_back(equal);
											chunk.push_back(binaryInstruction);
											chunk.push_back(or_);
										}
										else if (op.type == TokenType::EQUAL)
										{
											binaryInstruction->op = OP_INT_EQUAL;
											chunk.push_back(binaryInstruction);
										}
										else if (op.type == TokenType::BANG_EQUAL)
										{
											auto not = std::shared_ptr<twoAddress>();
											not->result = binaryInstruction->result;
											not->op = OP_LOGICAL_NOT;
											std::string temp("#");
											temp.append(std::to_string(temporaries++));
											binaryInstruction->result = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
											not->A = binaryInstruction->result;
											binaryInstruction->op = OP_INT_EQUAL;
											chunk.push_back(binaryInstruction);
											chunk.push_back(not);
										}
									}
									break;
								}
								case TokenType::AND:
								case TokenType::OR:
								{
									auto binaryInstruction = std::make_shared<threeAddress>();
									binaryInstruction->A = ((CallNode*)binaryNode->left.get())->primary;
									binaryInstruction->B = ((CallNode*)binaryNode->right.get())->primary;
									if(result != nullptr)
									{
										binaryInstruction->result = *result;
									}
									else
									{
										std::string temp("#");
										temp.append(std::to_string(temporaries++));
										binaryInstruction->result = { TokenType::IDENTIFIER, temp, binaryNode->left->line() };
									}
									if (op.type == TokenType::AND)
										binaryInstruction->op = OP_LOGICAL_AND;
									else if (op.type == TokenType::BIT_AND)
										binaryInstruction->op = OP_BITWISE_AND;
									else if (op.type == TokenType::OR)
										binaryInstruction->op = OP_LOGICAL_OR;
									else if (op.type == TokenType::BIT_OR)
										binaryInstruction->op = OP_BITWISE_OR;

									chunk.push_back(binaryInstruction);
								}
							}
						}
						return chunk;
					}
					case ExpressionNode::ExpressionType::Assignment:
					{
						auto assignmentNode = (AssignmentNode*)exprNode;

						std::vector<std::shared_ptr<assembly>> chunk;
						std::string assigned = assignmentNode->resolveIdentifier();

						size_t pos = assigned.find(".");
						bool isFieldAssignment = (pos != std::string::npos);
						
						std::string assignedVar = util::renameByScope({ TokenType::IDENTIFIER, assigned.substr(0, pos) , assignmentNode->line() }, currentScope).string;
						if (isFieldAssignment) assignedVar = assignedVar.append(assigned.substr(pos));

						Token id = { TokenType::IDENTIFIER, assignedVar, assignmentNode->line() };

						if (!isFieldAssignment)
						{
							chunk = compileNode(assignmentNode->value.get(), &id);
							if (chunk.back()->type() == Asm::TwoAddr)
							{
								Token value = ((twoAddress*)chunk.back().get())->A;
								OpCodes operator_ = ((twoAddress*)chunk.back().get())->op;
								if (value.type == TokenType::IDENTIFIER && operator_ == OP_CONST_LOW && value.string.compare(id.string) != 0)
								{
									chunk.pop_back();
									auto move = std::make_shared<twoAddress>();
									move->op = OP_MOVE;
									move->A = value;
									move->result = id;
									chunk.push_back(move);
								}
							}
						}
						else
						{
							std::string temp("#");
							temp.append(std::to_string(temporaries++));
							Token tempToken = { TokenType::IDENTIFIER, temp, assignmentNode->value->line() };
							chunk = compileNode(assignmentNode->value.get(), &tempToken);
							auto substr = assignedVar;
							std::string remainder = substr.substr(0, substr.find("."));
							auto scope = currentScope;
							std::string var = assignedVar.substr(0, assignedVar.find("#"));
							std::string currentType;
							std::vector<std::shared_ptr<threeAddress>> storeStack;
							while (scope != nullptr)
							{
								if(scope->symbols.find(var) != scope->symbols.end())
								{
									currentType = scope->symbols.at(var).type.string;
									scope = nullptr;
								}
								else
								{
									scope = scope->parentScope;
								}
							}
							while (substr.length() > 0)
							{
								size_t offset = substr.find(".");
								std::string last = remainder;
								remainder = substr.substr(0, offset);
								substr = substr.substr(offset + 1);
								if (offset == std::string::npos)
								{
									substr = std::string("");
								}
								auto scope = currentScope;
								std::vector<parameter> params;
								while (scope != nullptr)
								{
									if (scope->typeParameters.find(currentType) != scope->typeParameters.end())
									{
										params = scope->typeParameters.at(currentType);
										scope = nullptr;
									}
									else
									{
										scope = scope->parentScope;
									}
								}
								for (size_t i = 0; i < params.size(); i++)
								{
									if (remainder.compare(params[i].identifier.string) == 0)
									{
										currentType = params[i].type.string;
										if (offset != std::string::npos)
										{
											auto load = std::make_shared<threeAddress>();
											std::string newTemp("#");
											newTemp.append(std::to_string(temporaries++));
											Token newTempToken = { TokenType::IDENTIFIER, newTemp, assignmentNode->value->line() };
											load->op = OP_LOAD_OFFSET;
											load->A = newTempToken;
											if (last.compare(assignedVar.substr(0, assignedVar.find("."))) == 0)
											{
												load->B = { TokenType::IDENTIFIER, last , assignmentNode->line() };
											}
											else
											{
												load->B = storeStack.back()->A;
											}
											load->result = { TokenType::INT, std::to_string(i), assignmentNode->line() };
											chunk.push_back(load);
											auto store = std::make_shared<threeAddress>();
											store->op = OP_STORE_OFFSET;
											store->A = load->A;
											store->B = load->B;
											store->result = load->result;
											storeStack.push_back(store);
										}
										else
										{
											auto store = std::make_shared<threeAddress>();
											store->op = OP_STORE_OFFSET;
											store->A = tempToken;
											if(chunk.back()->type() == Asm::ThreeAddr)
											{
												std::shared_ptr<threeAddress> lastResult = std::dynamic_pointer_cast<threeAddress>(chunk.back());
												if (lastResult->op == OP_LOAD_OFFSET)
												{
													store->B = lastResult->A;
												}
												else store->B = { TokenType::IDENTIFIER, last, assignmentNode->line() };
											}
											store->result = { TokenType::INT, std::to_string(i), assignmentNode->line() };
											chunk.push_back(store);

											for (auto i = storeStack.rbegin(); i < storeStack.rend(); i++)
											{
												chunk.push_back(*i);
											}
										}
									}
								}
							}
						}

						return chunk;
					}
					case ExpressionNode::ExpressionType::FieldCall:
					{
						auto fieldNode = (FieldCallNode*)exprNode;

						std::vector<std::shared_ptr<assembly>> chunk;
						std::string temp("#");
						temp.append(std::to_string(temporaries++));
						Token tempToken = { TokenType::IDENTIFIER, temp, fieldNode->field.line };
						auto var = fieldNode->resolve().substr(0, fieldNode->resolve().find("."));
						auto substr = fieldNode->resolve();
						std::string remainder = util::renameByScope({ TokenType::IDENTIFIER, fieldNode->resolve().substr(0, fieldNode->resolve().find(".")), 0 }, currentScope).string;
						substr =substr.substr(substr.find("."));
						substr = substr.substr(substr.find(".") + 1);
						auto scope = currentScope;
						std::string currentType;
						while (scope != nullptr)
						{
							if (scope->symbols.find(var) != scope->symbols.end())
							{
								currentType = scope->symbols.at(var).type.string;
								scope = nullptr;
							}
							else
							{
								scope = scope->parentScope;
							}
						}
						Token lastTempToken = {};
						while (substr.length() > 0)
						{
							size_t offset = substr.find(".");
							std::string last = remainder;
							remainder = substr.substr(0, offset);
							substr = substr.substr(offset + 1);
							if (offset == std::string::npos)
							{
								substr = std::string("");
							}
							auto scope = currentScope;
							std::vector<parameter> params;
							while (scope != nullptr)
							{
								if (scope->typeParameters.find(currentType) != scope->typeParameters.end())
								{
									params = scope->typeParameters.at(currentType);
									scope = nullptr;
								}
								else
								{
									scope = scope->parentScope;
								}
							}
							for (size_t i = 0; i < params.size(); i++)
							{
								if (remainder.compare(params[i].identifier.string) == 0)
								{
									currentType = params[i].type.string;
									auto load = std::make_shared<threeAddress>();
									load->op = OP_LOAD_OFFSET;

									std::string newTemp("#");
									newTemp.append(std::to_string(temporaries++));
									Token newTempToken = { TokenType::IDENTIFIER, newTemp, fieldNode->line() };
									if (substr.size() || !result)
									{
										load->A = newTempToken;
									}
									else
									{
										load->A = *result;
									}
									if (lastTempToken.type == TokenType::ERROR)
									{
										load->B = { TokenType::IDENTIFIER, last , fieldNode->line() };
									}
									else
									{
										load->B = lastTempToken;
									}
									load->result = { TokenType::INT, std::to_string(i), fieldNode->line() };
									chunk.push_back(load);
									lastTempToken = newTempToken;
								}
								else
								{
									
								}
							}
						}
						return chunk;
					}
					case ExpressionNode::ExpressionType::FunctionCall:
					{
						auto callNode = (FunctionCallNode*)exprNode;

						std::vector<std::shared_ptr<assembly>> chunk;

						auto stackFrame = std::make_shared<pseudocode>();
						stackFrame->op = OP_NEW_STACK_FRAME;
						chunk.push_back(stackFrame);

						for (const auto& argument : callNode->arguments)
						{
							std::string temp("#");
							temp.append(std::to_string(temporaries++));
							Token tempToken = { TokenType::IDENTIFIER, temp, argument->line() };
							auto argResult = compileNode(argument.get(), &tempToken);
							chunk.insert(chunk.end(), argResult.begin(), argResult.end());
							auto push = std::make_shared<oneAddress>();
							push->op = OP_PUSH;
							push->A = tempToken;
							chunk.push_back(push);
						}

						std::string temp2("#");
						temp2.append(std::to_string(temporaries++));
						Token tempToken2 = { TokenType::IDENTIFIER, temp2, callNode->line() };
						auto constant = std::make_shared<twoAddress>();
						constant->op = OP_CONST_LOW;
						constant->result = tempToken2;
						constant->A = { TokenType::IDENTIFIER, callNode->resolveName(), callNode->line() };
						chunk.push_back(constant);

						auto ipPush = std::make_shared<pseudocode>();
						ipPush->op = OP_PUSH_IP;
						chunk.push_back(ipPush);

						auto call = std::make_shared<oneAddress>();
						call->op = OP_REGISTER_JUMP;
						call->A = tempToken2;
						chunk.push_back(call);

						std::string frameRegister("@");
						frameRegister.append(std::to_string(FRAME_REGISTER));
						Token frameRegisterToken = { TokenType::IDENTIFIER, frameRegister, callNode->left->line() };
						auto framePop = std::make_shared<oneAddress>();
						framePop->op = OP_POP;
						framePop->A = frameRegisterToken;
						chunk.push_back(framePop);

						if (result)
						{
							frameRegister = "@";
							frameRegister.append(std::to_string(RETURN_REGISTER));
							frameRegisterToken.string = frameRegister;
							auto move = std::make_shared<twoAddress>();
							move->op = OP_MOVE;
							move->A = frameRegisterToken;
							move->result = *result;
							chunk.push_back(move);
						}

						return chunk;
					}
					case ExpressionNode::ExpressionType::Constructor:
					{
						auto constructorNode = (ConstructorNode*)node;

						std::vector<std::shared_ptr<assembly>> chunk;
						if (result->string.front() == '#')
						{
							auto tempType = util::renameByScope(constructorNode->ConstructorType, currentScope);
							std::string temp = std::string("#");
							temp.append(std::to_string(temporaries++));
							Token tempToken = { TokenType::IDENTIFIER, temp, tempType.line };
							auto allocType = std::make_shared<twoAddress>();
							allocType->op = OP_CONST_LOW;
							allocType->A = { TokenType::INT,  std::to_string(typeIDs.at(tempType.string)), tempType.line };
							allocType->result = tempToken;
							chunk.push_back(allocType);
							auto alloc = std::make_shared<twoAddress>();
							alloc->op = OP_ALLOC;
							alloc->A = tempToken;
							alloc->result = *result;
							chunk.push_back(alloc);
						}
						size_t index = 0;
						for(const auto& arg : constructorNode->arguments)
						{
							std::string temp("#");
							temp.append(std::to_string(temporaries++));
							Token tempToken = { TokenType::IDENTIFIER, temp, constructorNode->line() };
							auto argChunk = compileNode(arg.get(), &tempToken);
							chunk.insert(chunk.end(), argChunk.begin(), argChunk.end());
							auto store = std::make_shared<threeAddress>();
							store->op = OP_STORE_OFFSET;
							store->result = { TokenType::INT, std::to_string(index), constructorNode->line() };
							store->B = *result;
							if(chunk.back()->type() == Asm::OneAddr)
							{
								auto oneAddr = std::dynamic_pointer_cast<oneAddress>(chunk.back());
								store->A = oneAddr->A;
							}
							else if ( chunk.back()->type() == Asm::TwoAddr)
							{
								auto twoAddr = std::dynamic_pointer_cast<twoAddress>(chunk.back());
								store->A = twoAddr->result;
							}
							else if (chunk.back()->type() == Asm::ThreeAddr)
							{
								auto threeAddr = std::dynamic_pointer_cast<threeAddress>(chunk.back());
								if(threeAddr->op == OP_STORE_OFFSET || threeAddr->op == OP_LOAD_OFFSET)
								{
									store->A = threeAddr->B;
								}
								else
								{
									store->A = threeAddr->result;
								}
							}
							chunk.push_back(store);
							index++;
						}
						return chunk;
					}
					case ExpressionNode::ExpressionType::Unary:
					{
						auto unaryNode = (UnaryNode*)node;

						std::vector<std::shared_ptr<assembly>> chunk;

						auto operand = compileNode(unaryNode->unary.get(), result);
						
						chunk.insert(chunk.end(), operand.begin(), operand.end());
						auto& last = chunk.back();
						auto& type = unaryNode->unaryType;

						auto& op = unaryNode->op;

						auto not = std::make_shared<twoAddress>();
						if (result != nullptr)
						{
							not->result = *result;
						}
						else
						{
							std::string temp("#");
							temp.append(std::to_string(temporaries++));
							not->result = { TokenType::IDENTIFIER, temp, unaryNode->line() };
						}
						not->A = not->result;

						switch(op.type)
						{
							case TokenType::BANG:
							{
								not->op = OP_LOGICAL_NOT;
								break;
							}
							case TokenType::MINUS:
							{
								if(type.string.compare("double") == 0)
								{
									not->op = OP_DOUBLE_NEGATE;
								}
								else if(type.string.compare("float") == 0)
								{
									not->op = OP_FLOAT_NEGATE;
								}
								else if(util::signed_int(type) || util::unsigned_int(type))
								{
									not->op = OP_INT_NEGATE;
								}
								break;
							}
						}
						if (last->type() == Asm::TwoAddr)
						{
							Token id = ((twoAddress*)last.get())->result;
							OpCodes operator_ = ((twoAddress*)last.get())->op;
							if (id.type == TokenType::IDENTIFIER && operator_ == OP_CONST_LOW)
							{
								((twoAddress*)last.get())->op = not->op;
							}
							else
							{
								chunk.push_back(not);
							}
						}
						else
						{
							chunk.push_back(not);
						}
						return chunk;
					}
					case ExpressionNode::ExpressionType::Primary:
					{
						auto primaryNode = (CallNode*)exprNode;

						auto constant = std::make_shared<twoAddress>();
						constant->A = primaryNode->primary;
						constant->op = OP_CONST_LOW;
						if (primaryNode->primary.type == TokenType::IDENTIFIER)
						{
							constant->A = util::renameByScope(primaryNode->primary, currentScope);
							constant->op = OP_MOVE;
						}
						
						if(result != nullptr)
						{
							constant->result = *result;
						}
						else
						{
							std::string temp("#");
							temp.append(std::to_string(temporaries++));
							constant->result = { TokenType::IDENTIFIER, temp, primaryNode->line() };
						}
						std::vector<std::shared_ptr<assembly>> chunk;
						chunk.push_back(constant);
						return chunk;
					}
				}
			}
		}
	}

	pseudochunk Compiler::correctLoadStore(pseudochunk chunk)
	{
		pseudochunk result{};
		for(auto& i : chunk.code)
		{
			switch(i->type())
			{
				case Asm::ThreeAddr:
				{
					auto instruction = std::dynamic_pointer_cast<threeAddress>(i);
					if (instruction->op == OP_LOAD_OFFSET ||
						instruction->op == OP_STORE_OFFSET ||
						instruction->op == OP_ARRAY_LOAD ||
						instruction->op == OP_ARRAY_STORE)
					{
						auto temp = std::string("#");
						temp.append(std::to_string(temporaries++));
						Token tempToken = { TokenType::IDENTIFIER, temp, instruction->result.line };
						auto constant = std::make_shared<twoAddress>();
						constant->op = OP_CONST_LOW;
						constant->A = instruction->result;
						constant->result = tempToken;
						instruction->result = tempToken;
						result.code.push_back(constant);
						result.code.push_back(instruction);
					}
					else
					{
						result.code.push_back(instruction);
					}
					break;
				}
				default:
				{
					result.code.push_back(i);
					break;
				}
			}
		}
		return result;
	}

	controlFlowGraph Compiler::analyzeControlFlow(pseudochunk chunk)
	{
		controlFlowGraph graph{};

		std::shared_ptr<controlFlowNode> currentNode = std::make_shared<controlFlowNode>();
		graph.procedures.emplace("<global>",currentNode);
		std::vector<std::shared_ptr<controlFlowNode>> nodes;
		std::unordered_map<size_t, std::shared_ptr<controlFlowNode>> nodeLabels;
		nodes.push_back(currentNode);
		
		size_t currentLabel = -1;
		std::vector<size_t> endLabels;
		std::vector<std::shared_ptr<controlFlowNode>> functionTails;
			for (size_t i = 0; i < chunk.code.size(); i++)
			{
				if (endLabels.size() &&
					chunk.code[i]->type() == Asm::Label &&
					std::dynamic_pointer_cast<label>(chunk.code[i])->label == endLabels.back())
				{
					currentNode = functionTails.back();
					endLabels.pop_back();
					functionTails.pop_back();
				}
				auto& instruction = chunk.code[i];
				switch (instruction->type())
				{
					case Asm::Jump:
					{
						std::shared_ptr<relativeJump> jump = std::dynamic_pointer_cast<relativeJump>(instruction);
						auto op = jump->op;
						currentNode->block.push_back(instruction);
						auto nextNode = std::make_shared<controlFlowNode>();
						switch (op)
						{
						case OP_RELATIVE_JUMP_IF_TRUE:
						{

							if (nodes.size() && nodes.back()->block.size() == 0) nextNode = currentNode;
							else
							{
								nodes.push_back(nextNode);
								currentNode->falseBlock = nextNode;
							}
							currentLabel = -1;
							break;
						}
						default:
						{
							currentNode = nextNode;
							break;
						}
						}
						break;
					}
					case Asm::Label:
					{
						auto newLabel = std::dynamic_pointer_cast<label>(instruction);
						auto nextNode = std::make_shared<controlFlowNode>();
						if (nodes.size() && nodes.back()->block.size() == 0) nextNode = currentNode;
						else
						{
							nodes.push_back(nextNode);
							if (currentNode->block.size() && currentNode->block.back()->type() != Asm::Jump) currentNode->trueBlock = nextNode;
						}
						nextNode->block.push_back(instruction);
						currentLabel = newLabel->label;
						currentNode = nextNode;
						if (currentLabel != -1) nodeLabels.emplace(currentLabel, currentNode);
						break;
					}
					case Asm::FunctionLabel:
					{
						auto fLabel = std::dynamic_pointer_cast<functionLabel>(instruction);
						auto fSkip = std::dynamic_pointer_cast<relativeJump>(chunk.code[i - 1]);
						endLabels.push_back(fSkip->jumpLabel);
						functionTails.push_back(currentNode);
						auto nextNode = std::make_shared<controlFlowNode>();
						currentNode = nextNode;
						currentLabel = -1;
						graph.procedures.emplace(fLabel->name, nextNode);
						break;
					}
					default:
					{
						currentNode->block.push_back(instruction);
					}
				}
			}
		for(const auto& node : nodes)
		{
			if (node->block.size())
			{
				if (node->block.back()->type() == Asm::Jump)
				{
					auto jump = std::dynamic_pointer_cast<relativeJump>(node->block.back());
					if (nodeLabels.find(jump->jumpLabel) != nodeLabels.end())
					{
						node->trueBlock = nodeLabels.at(jump->jumpLabel);
					}
					else
					{
						std::cout << "no jump label " << jump->jumpLabel << " found!" << std::endl;
					}
				}
			}
		}

		return graph;
	}

	pseudochunk Compiler::allocateRegisters(pseudochunk chunk)
	{
		auto cfg = analyzeControlFlow(chunk);
		std::vector<std::unordered_set<std::string>> livePoints;
		std::unordered_map<std::string, size_t> functions;
		std::unordered_map<std::string, std::bitset<256>> functionRegisters;
		for (const auto& procedure : cfg.procedures)
		{
			std::unordered_map<std::string, int16_t> registers;
			std::unordered_map<std::string, std::unordered_set<std::string>> interferenceGraph;
			livePoints = liveVariables(procedure.second);
			for(auto i = livePoints.rbegin(); i != livePoints.rend(); i++)
			{
				auto& set = *i;
				for(const auto& string : set)
				{
					if(interferenceGraph.find(string) == interferenceGraph.end())
					{
						interferenceGraph.emplace(std::make_pair(string, set));
					}
					else
					{
						interferenceGraph.at(string).insert(set.begin(), set.end());
					}
					interferenceGraph.at(string).erase(string);
				}
			}

				//show live variables at each code point:
				for (auto i = livePoints.rbegin(); i != livePoints.rend(); i++)
				{
					bool first = true;
					auto& node = *i;
					for(const auto& string : node)
					{
						if (!first) std::cout << ", ";
						else first = false;
						std::cout << string;
					}
					std::cout << std::endl;
				}

				// output to graphvis:
				//std::unordered_set<std::string> existingEdges;
				//for(auto& kv : interferenceGraph)
				//{				
				//	std::string node = kv.first;
				//	node.insert(0, 1, '\"');
				//	node.append("\"");
				//	for(auto string : kv.second)
				//	{
				//		if (existingEdges.find(string) == existingEdges.end())
				//		{
				//			string.insert(0, 1, '\"');
				//			string.append("\"");
				//			std::cout << node << " -- ";
				//			std::cout << string << ";" << std::endl;
				//		}
				//	}
				//	existingEdges.insert(kv.first);
				//}

				// association graph:
				//for(auto& kv : interferenceGraph)
				//{
				//	std::cout << kv.first << ": ";
				//	for(const auto& string : kv.second)
				//	{
				//		std::cout << string << ", ";
				//	}
				//	std::cout << std::endl;
				//}

			std::unordered_set<std::string> poppedSet;
			std::vector <std::pair<std::string, std::unordered_set<std::string>>> nodeStack = {};
			std::bitset<256> functionRegister;
			std::unordered_map<std::string, std::unordered_set<std::string>> workingGraph = interferenceGraph;
			for(auto& kv : workingGraph)
			{
				std::unordered_set<std::string> liveEdges = {};
				std::copy_if(kv.second.begin(), kv.second.end(), std::inserter(liveEdges, liveEdges.begin()), [&poppedSet](std::string s) { return (poppedSet.find(s) == poppedSet.end()); });
				if(liveEdges.size() < 253)
				{
					poppedSet.insert(kv.first);
					nodeStack.push_back(std::make_pair(kv.first,liveEdges));
				}
				else
				{
					std::cout << kv.first << " has too many edges!";
				}	
				
			}

			//for(auto& node : nodeStack)
			//{
			//	std::cout << node.first << ": ";
			//	bool first = true;
			//	for(auto& string : node.second)
			//	{
			//		if (!first) std::cout << ", ";
			//		std::cout << string;
			//	}
			//	std::cout << std::endl;
			//}

			for (auto it = nodeStack.rbegin(); it != nodeStack.rend(); it++)
			{
				auto kv = *it;
				if(kv.first.find('@') == 0)
				{
					auto value = kv.first.substr(1);
					registers.emplace(kv.first, std::stoi(value));
				}
				else
				{
					if (kv.second.size() == 0 && registers.find(kv.first) != registers.end())
					{
						registers.emplace(kv.first, 3);
					}
					else
					{
						std::bitset<256> openRegisters{ 0x7 }; //set first three bits as reserved registers
						for (auto& node : kv.second)
						{
							int16_t usedRegister = registers.at(node);
							if (usedRegister >= 0 && usedRegister < 256)
							{
								openRegisters.set(usedRegister);
							}
						}

						for (int i = 0; i < 256; i++)
						{
							if (!openRegisters[i] && registers.find(kv.first) == registers.end())
							{
								registers.emplace(kv.first, i);
								openRegisters.set(i);
								break;
							}
						}
						functionRegister |= openRegisters;
					}
				}
			}
			if (procedure.first.compare("<global>") != 0)
			{
				functionRegisters.emplace(procedure.first, functionRegister);
			}
			std::vector<std::string> remove;
			for(auto& kv : registers)
			{
				if (kv.first.find('@') == 0) remove.push_back(kv.first);
			}
			for(auto& str : remove)
			{
				registers.erase(str);
			}
			for (size_t j = 0; j < chunk.code.size(); j++)
			{

				auto& instruction = chunk.code[j];
				switch (instruction->type())
				{
					case Asm::OneAddr:
					{
						auto& oneAddr = std::dynamic_pointer_cast<oneAddress>(instruction);
						if (registers.find(oneAddr->A.string) != registers.end())
						{
							oneAddr->A = { TokenType::IDENTIFIER, std::to_string(registers.at(oneAddr->A.string)), oneAddr->A.line };
						}
						break;
					}
					case Asm::TwoAddr:
					{
						auto& twoAddr = std::dynamic_pointer_cast<twoAddress>(instruction);
						if (twoAddr->op == OP_CONST_LOW)
						{
							if (registers.find(twoAddr->result.string) != registers.end())
							{
								twoAddr->result = { TokenType::IDENTIFIER, std::to_string(registers.at(twoAddr->result.string)), twoAddr->result.line };
							}
						}
						else
						{
							if (registers.find(twoAddr->A.string) != registers.end())
							{
								twoAddr->A = { TokenType::IDENTIFIER, std::to_string(registers.at(twoAddr->A.string)), twoAddr->A.line };
							}
							if (registers.find(twoAddr->result.string) != registers.end())
							{
								twoAddr->result = { TokenType::IDENTIFIER, std::to_string(registers.at(twoAddr->result.string)), twoAddr->result.line };
							}
						}
						break;
					}
					case Asm::ThreeAddr:
					{
						auto& threeAddr = std::dynamic_pointer_cast<threeAddress>(instruction);
						if (registers.find(threeAddr->A.string) != registers.end())
						{
							threeAddr->A = { TokenType::IDENTIFIER, std::to_string(registers.at(threeAddr->A.string)), threeAddr->A.line };
						}
						if (registers.find(threeAddr->B.string) != registers.end())
						{
							threeAddr->B = { TokenType::IDENTIFIER, std::to_string(registers.at(threeAddr->B.string)), threeAddr->B.line };
						}
						if (registers.find(threeAddr->result.string) != registers.end())
						{
							threeAddr->result = { TokenType::IDENTIFIER,
								std::to_string(registers.at(threeAddr->result.string)),
								threeAddr->result.line };
						}
						break;
					}
					case Asm::FunctionLabel:
					{
						auto fLabel = std::dynamic_pointer_cast<functionLabel>(instruction);
						auto fSkip = std::dynamic_pointer_cast<relativeJump>(chunk.code[j - 1]);
						functions.emplace(fLabel->name, j);
						break;
					}
				}
			}
		}
		for (auto& kv : functions)
		{
			if (kv.first.compare("<global>") == 0) continue;
			std::vector<std::shared_ptr<assembly>> pushes;
			std::vector<std::shared_ptr<assembly>> pops;
			auto& registers = functionRegisters.at(kv.first);
			for (size_t j = 3; j < registers.size(); j++)
			{
				if (registers[j])
				{
					auto pop = std::make_shared<oneAddress>();
					pop->op = OP_POP;
					pop->A = { TokenType::INT, std::to_string(j), 0 };
					pops.push_back(pop);
					auto push = std::make_shared<oneAddress>();
					push->op = OP_PUSH;
					push->A = { TokenType::INT, std::to_string(j), 0 };
					pushes.push_back(push);
				}
 			}

			auto& fSkip = std::dynamic_pointer_cast<relativeJump>(chunk.code[kv.second - 1]);
			std::vector<size_t> returns;
			for (size_t i = kv.second + 1; (chunk.code[i]->type() != Asm::Label ||
				std::dynamic_pointer_cast<label>(chunk.code[i])->label != fSkip->jumpLabel); i++)
			{
				auto& instruction = chunk.code[i];
				switch (instruction->type())
				{
				case Asm::FunctionLabel:
				{
					auto& fSkip = std::dynamic_pointer_cast<relativeJump>(chunk.code[i - 1]);
					while (chunk.code[i]->type() != Asm::Label ||
						std::dynamic_pointer_cast<label>(chunk.code[i])->label != fSkip->jumpLabel)
					{
						i++;
					}
				}
				case Asm::pseudocode:
				{
					auto& one = std::dynamic_pointer_cast<pseudocode>(instruction);
					if (one->op == OP_RETURN)
						returns.push_back(i);
				}
				}
			}
			std::reverse(pops.begin(), pops.end());
			for (auto it = returns.rbegin(); it != returns.rend(); it++)
			{
				chunk.code.insert(chunk.code.begin() + *it, pops.begin(), pops.end()); // maybe needs to be -1?
			}
			chunk.code.insert(chunk.code.begin() + kv.second + 1, pushes.begin(), pushes.end());
		}
		for(const auto& procedure : cfg.procedures)
		{
			cleanupCFGNode(procedure.second);
		}
		return chunk;
	}

	std::vector<std::unordered_set<std::string>> Compiler::liveVariables(std::shared_ptr<controlFlowNode> block)
	{
		//rework function to generate a list of live program points which will later be used to generate the interference graph
		block->traversed = true;
		std::vector<std::unordered_set<std::string>> trueGraph;
		std::vector<std::unordered_set<std::string>> falseGraph;
		std::vector<std::unordered_set<std::string>> livePoints;
		if(block->trueBlock != nullptr)
		{
			if(block->trueBlock->traversed == false)
			{
				trueGraph = liveVariables(block->trueBlock);
			}

			if(block->falseBlock != nullptr)
			{
				if(block->falseBlock->traversed == false)
				{
					falseGraph = liveVariables(block->falseBlock);
				}
			}
		}
		std::unordered_set<std::string> lastNodes;
		if (trueGraph.size())
		{
			auto trueBegin = trueGraph.back();
			lastNodes.insert(trueBegin.begin(), trueBegin.end());
		}
		if (falseGraph.size())
		{
			auto falseBegin = trueGraph.back();
			lastNodes.insert(falseBegin.begin(), falseBegin.end());
		}
		livePoints.insert(livePoints.end(), trueGraph.begin(), trueGraph.end());
		livePoints.insert(livePoints.end(), falseGraph.begin(), falseGraph.end());
		livePoints.push_back(lastNodes);
		for (auto& i = block->block.rbegin(); i != block->block.rend(); i++)
		{
			std::unordered_set<std::string> liveNodes;
			liveNodes.insert(lastNodes.begin(), lastNodes.end());
			auto instruction = *i;
			switch(instruction->type())
			{
				case Asm::Label:
				case Asm::Jump:
					continue;

				case Asm::OneAddr:
				{
					auto oneAddr = std::dynamic_pointer_cast<oneAddress>(instruction);
					liveNodes = lastNodes;
					switch(oneAddr->op)
					{
					case OP_POP:
					case OP_PUSH:
					case OP_REGISTER_JUMP:
					case OP_REGISTER_JUMP_IF_TRUE:
					case OP_OUT:
						if(liveNodes.find(oneAddr->A.string) == liveNodes.end())
						{
							liveNodes.insert(oneAddr->A.string);
						}
						livePoints.push_back(liveNodes);
						break;
					default:
						std::cout << "operation " << OpcodeNames[oneAddr->op] << " not implemented" << std::endl;
					}
					break;
				}
				case Asm::TwoAddr:
				{
					auto twoAddr = std::dynamic_pointer_cast<twoAddress>(instruction);
					switch (twoAddr->op)
					{
						case OP_CONST_LOW:
						{
							if(liveNodes.find(twoAddr->result.string) == liveNodes.end())
							{
								//std::cout << twoAddr->result.string << " can be discarded, no longer used" << std::endl;
								livePoints.back().insert(twoAddr->result.string);
							}
							else
							{
								liveNodes.erase(twoAddr->result.string);
								livePoints.push_back(liveNodes);
							}
							break;
						}
						case OP_ALLOC:
						{
							if (liveNodes.find(twoAddr->result.string) == liveNodes.end())
							{
								//std::cout << threeAddr->result.string << " can be discarded, no longer used" << std::endl;
								livePoints.back().insert(twoAddr->result.string);
							}
							else
							{
								liveNodes.erase(twoAddr->result.string);
							}
							liveNodes.insert(twoAddr->A.string);
							livePoints.push_back(liveNodes);
							break;
							break;
						}
						case OP_INT_NEGATE:
						case OP_FLOAT_NEGATE:
						case OP_DOUBLE_NEGATE:
						case OP_LOGICAL_NOT:
						case OP_INT_TO_DOUBLE:
						case OP_INT_TO_FLOAT:
						case OP_FLOAT_TO_DOUBLE:
						case OP_FLOAT_TO_INT:
						case OP_DOUBLE_TO_FLOAT:
						case OP_DOUBLE_TO_INT:
						case OP_MOVE:
						case OP_MOVE_FROM_STACK_FRAME:
						{
							if(liveNodes.find(twoAddr->result.string) == liveNodes.end())
							{
								//std::cout << twoAddr->result.string << " can be discarded, no longer used" << std::endl;
								livePoints.back().insert(twoAddr->result.string);
							}
							else
							{
								liveNodes.erase(twoAddr->result.string);
								
							}
							liveNodes.insert(twoAddr->A.string);
							livePoints.push_back(liveNodes);
							break;
						}
						default:
							std::cout << "operation " << OpcodeNames[twoAddr->op] << " not implemented" << std::endl;
					}
					break;
				}
				case Asm::ThreeAddr:
				{
					auto threeAddr = std::dynamic_pointer_cast<threeAddress>(instruction);
					switch(threeAddr->op)
					{
						case OP_STORE_OFFSET:
						case OP_ARRAY_STORE:
						{
							liveNodes.insert(threeAddr->A.string);
							liveNodes.insert(threeAddr->B.string);
							liveNodes.insert(threeAddr->result.string);
							livePoints.push_back(liveNodes);
							break;
						}
						case OP_LOAD_OFFSET:
						case OP_ARRAY_LOAD:
						{
							if (liveNodes.find(threeAddr->A.string) == liveNodes.end())
							{
								//std::cout << threeAddr->A.string << " can be discarded, no longer used" << std::endl;
								livePoints.back().insert(threeAddr->result.string);
							}
							else
							{
								liveNodes.erase(threeAddr->A.string);
							}
							liveNodes.insert(threeAddr->B.string);
							liveNodes.insert(threeAddr->result.string);
							livePoints.push_back(liveNodes);
							break;
						}
						case OP_ALLOC_ARRAY:
						case OP_INT_ADD:
						case OP_INT_SUB:
						case OP_UNSIGN_MUL:
						case OP_UNSIGN_DIV:
						case OP_SIGN_MUL:
						case OP_SIGN_DIV:
						case OP_FLOAT_ADD:
						case OP_FLOAT_SUB:
						case OP_FLOAT_MUL:
						case OP_FLOAT_DIV:
						case OP_BIT_SHIFT_LEFT:
						case OP_BIT_SHIFT_RIGHT:
						case OP_UNSIGN_LESS:
						case OP_UNSIGN_GREATER:
						case OP_SIGN_LESS:
						case OP_SIGN_GREATER:
						case OP_INT_EQUAL:
						case OP_FLOAT_LESS:
						case OP_FLOAT_GREATER:
						case OP_FLOAT_EQUAL:
						case OP_DOUBLE_LESS:
						case OP_DOUBLE_GREATER:
						case OP_DOUBLE_EQUAL:
						case OP_BITWISE_AND:
						case OP_BITWISE_OR:
						case OP_LOGICAL_AND:
						case OP_LOGICAL_OR:
						{
							if (liveNodes.find(threeAddr->result.string) == liveNodes.end())
							{
								//std::cout << threeAddr->result.string << " can be discarded, no longer used" << std::endl;
								livePoints.back().insert(threeAddr->result.string);
							}
							else
							{
								liveNodes.erase(threeAddr->result.string);
							}
							liveNodes.insert(threeAddr->A.string);
							liveNodes.insert(threeAddr->B.string);
							livePoints.push_back(liveNodes);
							break;
						}	
						default:
							std::cout << "operation " << OpcodeNames[threeAddr->op] << " not implemented" << std::endl;
					}
					break;
				}
			}
			if (livePoints.size()) lastNodes = livePoints.back();
			else lastNodes = {};
		}
		return livePoints;
	}

	std::shared_ptr<Chunk> Compiler::finalizeCode(pseudochunk chunk)
	{
		std::shared_ptr<Chunk> result = std::make_shared<Chunk>();
		result->reserve(chunk.code.size());
		std::unordered_map<size_t, size_t> labelIndices;
		std::unordered_multimap<size_t, size_t> unpatchedJumps;
		std::unordered_map<std::string, size_t> functionAddrs;
		size_t labelCodes = 0;
		for(size_t i = 0; i < chunk.code.size(); i++)
		{
			const auto& instruction = chunk.code[i];

			switch(instruction->type())
			{
				case Asm::pseudocode:
				{
					auto pCode = std::dynamic_pointer_cast<pseudocode>(instruction);
					result->WriteOp(pCode->op);
					break;
				}
				case Asm::OneAddr:
				{
					auto i = std::dynamic_pointer_cast<oneAddress>(instruction);
					if (i->A.string.find('@') == 0) i->A.string = i->A.string.substr(1);
					result->WriteA(i->op, std::stoi(i->A.string), i->A.line);
					break;
				}
				case Asm::TwoAddr:
				{
					auto i = std::dynamic_pointer_cast<twoAddress>(instruction);
					if (i->A.string.find('@') == 0) i->A.string = i->A.string.substr(1);
					if (i->result.string.find('@') == 0) i->result.string = i->result.string.substr(1);
					if (i->op == OP_CONST_LOW)
					{
						if (i->A.type == TokenType::FLOAT)
						{
							result->WriteFloat(std::stoi(i->result.string), std::stof(i->A.string));
						}
						else if (i->A.type == TokenType::DOUBLE)
						{
							result->WriteDouble(std::stoi(i->result.string), std::stod(i->A.string));
						}
						else if (i->A.type == TokenType::INT)
						{
							if (i->A.string.at(0) == '-')
							{
								int64_t value = std::stoll(i->A.string);
								if (-value > 0 && -value <= INT16_MAX)
								{
									result->WriteI16(std::stoi(i->result.string), value);
								}
								else if (-value > INT16_MAX && -value <= INT32_MAX)
								{
									result->WriteI32(std::stoi(i->result.string), value);
								}
								else
								{
									result->WriteI64(std::stoi(i->A.string), value);
								}
							}
							else
							{
								uint64_t value = std::stoull(i->A.string);
								if (value >= 0 && value <= UINT16_MAX)
								{
									result->WriteU16(std::stoi(i->result.string), static_cast<uint16_t>(value));
								}
								else if (value > UINT16_MAX && value <= UINT32_MAX)
								{
									result->WriteU32(std::stoi(i->result.string), static_cast<uint32_t>(value));
								}
								else
								{
									result->WriteU64(std::stoi(i->result.string), value);
								}
							}
						}
						else if (i->A.type == TokenType::IDENTIFIER)
						{
							if(functionAddrs.find(i->A.string) != functionAddrs.end())
							{
								if(functionAddrs.at(i->A.string) <= UINT16_MAX)
								{
									result->WriteU16(std::stoi(i->result.string), static_cast<uint16_t>(functionAddrs.at(i->A.string)));
								}
								if (functionAddrs.at(i->A.string) > UINT16_MAX && functionAddrs.at(i->A.string) <= UINT32_MAX)
								{
									result->WriteU32(std::stoi(i->result.string), static_cast<uint32_t>(functionAddrs.at(i->A.string)));
								}
								else
								{
									result->WriteU64(std::stoi(i->result.string), functionAddrs.at(i->A.string));
								}
							}
						}
					}
					else result->WriteAB(i->op, std::stoi(i->A.string), std::stoi(i->result.string), i->result.line);
					break;
				}
				case Asm::ThreeAddr:
				{
					auto i = std::dynamic_pointer_cast<threeAddress>(instruction);
					if (i->A.string.find('@') == 0) i->A.string = i->A.string.substr(1);
					if (i->B.string.find('@') == 0) i->B.string = i->B.string.substr(1);
					if (i->result.string.find('@') == 0) i->result.string = i->result.string.substr(1);
					result->WriteABC(i->op, std::stoi(i->A.string), std::stoi(i->B.string), std::stoi(i->result.string), i->result.line);
					break;
				}
				case Asm::Label:
				{
					auto i = std::dynamic_pointer_cast<label>(instruction);
					labelIndices.emplace(i->label, result->size());
					if (unpatchedJumps.find(i->label) != unpatchedJumps.end())
					{
						auto jumps = unpatchedJumps.equal_range(i->label);
						for (auto& it = jumps.first; it != jumps.second; it++)
						{
							result->at(it->second) += (labelIndices.at(i->label) - it->second);
						}
					}
					labelCodes++;
					break;
				}
				case Asm::FunctionLabel:
				{
					auto fLabel = std::dynamic_pointer_cast<functionLabel>(instruction);
					functionAddrs.emplace(fLabel->name, result->size());

					labelCodes++;
					break;
				}
				case Asm::Jump:
				{
					auto i = std::dynamic_pointer_cast<relativeJump>(instruction);
					if(labelIndices.find(i->jumpLabel) != labelIndices.end())
					{
						auto count = result->size();
						result->WriteRelativeJump(i->op, labelIndices.at(i->jumpLabel) - count , 0);
					}
					else
					{
						size_t index = result->size();
						result->WriteRelativeJump(i->op, 0, 0);
						unpatchedJumps.emplace(i->jumpLabel, index);
					}
					break;
				}
			}
		}
		return result;
	}

	void cleanupCFGNode(std::shared_ptr<controlFlowNode> node)
	{
		if (node->trueBlock && node->trueBlock->traversed)
		{
			node->trueBlock->traversed = false;
			cleanupCFGNode(node->trueBlock);
		}

		if (node->falseBlock && node->falseBlock->traversed)
		{
			node->falseBlock->traversed = false;
			cleanupCFGNode(node->falseBlock);
		}

		node->trueBlock = nullptr;
		node->falseBlock = nullptr;
	}
}