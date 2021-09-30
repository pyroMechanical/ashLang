#include "Compiler.h"
#include "Semantics.h"
#include <string>
#include <chrono>

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
		Parser parser(source);

		auto t1 = std::chrono::high_resolution_clock::now();
		auto ast = parser.parse();
		auto t2 = std::chrono::high_resolution_clock::now();

		std::cout << "Parsing took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";

		if (ast->hadError) return false;
		Semantics analyzer;

		//ast->print(0);
		t1 = std::chrono::high_resolution_clock::now();
		ast = analyzer.findSymbols(ast);
		t2 = std::chrono::high_resolution_clock::now();

		std::cout << "Semantic analysis took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
		temporaries = analyzer.temporaries;
		if (ast->hadError) return false;

		t1 = std::chrono::high_resolution_clock::now();
		pseudochunk result = precompile(ast);
		t2 = std::chrono::high_resolution_clock::now();

		std::cout << "Precompiling took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
		std::cout << std::endl;

		//for(const auto& instruction : result.code)
		//{
		//	instruction->print();
		//}

		t1 = std::chrono::high_resolution_clock::now();
		result = allocateRegisters(result);
		t2 = std::chrono::high_resolution_clock::now();

		std::cout << "Register Allocation took " << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000000.0 << "milliseconds.\n";
		//for (const auto& instruction : result.code)
		//{
		//	instruction->print();
		//}

		/*for (const auto& typeID : typeIDs)
		{
			std::cout << typeID.first << ": " << typeID.second << std::endl;
		}*/

		return false;
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
			chunk.code.push_back(halt);

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
				size_t offset = 12;
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
							fieldData.offset = offset += ~offset & 0x01;
							fieldData.typeID = -9;
							offset += 2;
						}
						else if (!field.type.string.compare("ushort"))
						{
							fieldData.type = FieldType::UShort;
							fieldData.offset = offset += ~offset & 0x01;
							fieldData.typeID = -8;
							offset += 2;
						}
						else if (!field.type.string.compare("int"))
						{
							fieldData.type = FieldType::Int;
							fieldData.offset = offset += ~offset & 0x03;
							fieldData.typeID = -7;
							offset += 4;
						}
						else if (!field.type.string.compare("uint"))
						{
							fieldData.type = FieldType::UInt;
							fieldData.offset = offset += ~offset & 0x03;
							fieldData.typeID = -6;
							offset += 4;
						}
						else if (!field.type.string.compare("char"))
						{
							fieldData.type = FieldType::Char;
							fieldData.offset = offset += ~offset & 0x03;
							fieldData.typeID = -5;
							offset += 4;
						}
						else if (!field.type.string.compare("float"))
						{
							fieldData.type = FieldType::Float;
							fieldData.offset = offset += ((offset ^ 0x03) * (offset & 0x03));
							fieldData.typeID = -4;
							offset += 4;
						}
						else if (!field.type.string.compare("long"))
						{
							fieldData.type = FieldType::Long;
							fieldData.offset = offset += ((offset ^ 0x07) * (offset & 0x07));
							fieldData.typeID = -3;
							offset += 8;
						}
						else if (!field.type.string.compare("ulong"))
						{
							fieldData.type = FieldType::ULong;
							fieldData.offset = offset += ((offset ^ 0x07) * (offset & 0x07));
							fieldData.typeID = -2;
							offset += 8;
						}
						else if (!field.type.string.compare("double"))
						{
							fieldData.type = FieldType::Double;
							fieldData.offset = offset += ((offset ^ 0x07) * (offset & 0x07));
							fieldData.typeID = -1;
							offset += 8;
						}
					}
					else
					{
						fieldData.type = FieldType::Struct;
						fieldData.offset = offset += ~offset & 0x07;
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

				auto identifier = util::renameByScope(varNode->identifier, currentScope);

				if(util::isBasic(varNode->type))
				{
					if (varNode->value)
					{ 
						result = compileNode(varNode->value.get(), &identifier);
						if (result.back()->type() == Asm::TwoAddr)
						{
							Token id = ((twoAddress*)result.back().get())->result;
							OpCodes operator_ = ((twoAddress*)result.back().get())->op;
							if (id.type == TokenType::IDENTIFIER && operator_ == OP_CONST_LOW)
							{
								result.clear();
								auto move = std::make_shared<twoAddress>();
								move->op = OP_MOVE;
								move->A = id;
								move->result = identifier;
								result.push_back(move);
							}
						}
					}
				}
				else
				{
					auto varType = util::renameByScope(varNode->type, currentScope);
					auto alloc = std::make_shared<twoAddress>();
					alloc->op = OP_ALLOC;
					alloc->A = varType;
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
										binaryInstruction->A = ((CallNode*)binaryNode->left.get())->primary;
									}
									else
									{
										auto conversion = std::make_shared<twoAddress>();
										conversion->A = ((CallNode*)binaryNode->left.get())->primary;
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
										binaryInstruction->B = ((CallNode*)binaryNode->right.get())->primary;
									}
									else
									{
										auto conversion = std::make_shared<twoAddress>();
										conversion->A = ((CallNode*)binaryNode->right.get())->primary;
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
										binaryInstruction->result = { TokenType::IDENTIFIER, temp,binaryNode->left->line() };
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
								Token id = ((twoAddress*)chunk.back().get())->result;
								OpCodes operator_ = ((twoAddress*)chunk.back().get())->op;
								if (id.type == TokenType::IDENTIFIER && operator_ == OP_CONST_LOW)
								{
									chunk.clear();
									auto move = std::make_shared<twoAddress>();
									move->op = OP_MOVE;
									move->result = id;
									Token assigned = { TokenType::IDENTIFIER, assignedVar, assignmentNode->line() };
									move->A = assigned;
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
						
					}
					case ExpressionNode::ExpressionType::FunctionCall:
					{
						
					}
					case ExpressionNode::ExpressionType::Constructor:
					{
						auto constructorNode = (ConstructorNode*)node;

						std::vector<std::shared_ptr<assembly>> chunk;
						if (result->string.front() == '#')
						{
							auto tempType = util::renameByScope(constructorNode->ConstructorType, currentScope);
							auto alloc = std::make_shared<twoAddress>();
							alloc->op = OP_ALLOC;
							alloc->A = tempType;
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
						if (primaryNode->primary.type == TokenType::IDENTIFIER) constant->A = util::renameByScope(primaryNode->primary, currentScope);
						constant->op = OP_CONST_LOW;
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

	controlFlowGraph Compiler::analyzeControlFlow(pseudochunk chunk)
	{
		controlFlowGraph graph{};

		std::shared_ptr<controlFlowNode> currentNode = std::make_shared<controlFlowNode>();
		graph.procedures.push_back(currentNode);
		std::vector<std::shared_ptr<controlFlowNode>> nodes;
		std::unordered_map<size_t, std::shared_ptr<controlFlowNode>> nodeLabels;
		nodes.push_back(currentNode);
		size_t currentLabel = -1;
		for(const auto& instruction : chunk.code)
		{
			switch (instruction->type())
			{
				case Asm::Jump:
				{
					std::shared_ptr<relativeJump> jump = std::dynamic_pointer_cast<relativeJump>(instruction);
					auto op = jump->op;
					currentNode->block.push_back(instruction);
					switch(op)
					{
						case OP_RELATIVE_JUMP_IF_TRUE:
						{
							auto nextNode = std::make_shared<controlFlowNode>();
							if (nodes.size() && nodes.back()->block.size() == 0) nextNode = currentNode;
							else
							{
								nodes.push_back(nextNode);
								currentNode->falseBlock = nextNode;
							}
							currentNode = nextNode;
							currentLabel = -1;
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
			else
			{
				std::cout << "block is empty!" << std::endl;
			}
		}

		return graph;
	}

	pseudochunk Compiler::allocateRegisters(pseudochunk chunk)
	{
		std::unordered_map<std::string, int16_t> registers;
		auto cfg = analyzeControlFlow(chunk);
		std::unordered_map<std::string, std::unordered_set<std::string>> interferenceGraph;
		for (const auto& procedure : cfg.procedures)
		{
			interferenceGraph = liveVariables(procedure);
		}
		return chunk;
	}

	std::vector<std::unordered_set<std::string>> Compiler::liveVariables(std::shared_ptr<controlFlowNode> block)
	{
		//rework function to generate a list of live program points which will later be used to generate the interference graph
		block->traversed = true;
		std::vector<std::unordered_set<std::string>> graph;
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
		for (auto& i = block->block.rbegin(); i != block->block.rend(); i++)
		{
			auto instruction = *i;
			switch(instruction->type())
			{
				case Asm::Label:
				case Asm::Jump:
					continue;

				case Asm::OneAddr:
				{
					auto oneAddr = std::dynamic_pointer_cast<oneAddress>(instruction);
					switch(oneAddr->op)
					{
					case OP_POP:
					case OP_RETURN:
					case OP_OUT:
						if (graph.find(oneAddr->A.string) == graph.end())
						{
							graph.insert(std::pair<std::string, std::unordered_set<std::string>>(oneAddr->A.string, std::unordered_set<std::string>()));
						}
					}
				}
				case Asm::TwoAddr:
				{
					auto twoAddr = std::dynamic_pointer_cast<twoAddress>(instruction);
					switch (twoAddr->op)
					{
						case OP_CONST_LOW:
						{
							if (graph.find(twoAddr->A.string) == graph.end())
							{
								graph.insert(std::pair<std::string, std::unordered_set<std::string>>(twoAddr->A.string, std::unordered_set<std::string>()));
							}
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
						{
							
						}
					}
				}
			}
		}


	}
}