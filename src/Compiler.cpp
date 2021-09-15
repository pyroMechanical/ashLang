#include "Compiler.h"
#include "Semantics.h"
#include "ControlFlowAnalysis.h"
#include <string>

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
	}
	bool Compiler::compile(const char* source)
	{
		Parser parser(source);

		auto ast = parser.parse();

		Semantics analyzer;

		ast->print(0);

		ast = analyzer.findSymbols(ast);
		temporaries = analyzer.temporaries;
		if (ast->hadError) return false;

		//ast->print(0);

		//ControlFlowAnalysis cfa;

		//std::shared_ptr<ControlFlowGraph> cfg = cfa.createCFG(ast);

		//for (const auto func : cfg->basicBlocks)
		//{
		//	func->print(0);
		//}

		pseudochunk result = precompile(ast);
		
		std::cout << std::endl;

		for(const auto& instruction : result.code)
		{
			instruction->print();
		}

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

				if(util::isBasic(varNode->type))
				{
					auto identifier = util::renameByScope(varNode->identifier, currentScope);
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
				else
				{
					
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

						std::vector<std::shared_ptr<assembly>> result;

						if(util::isBasic(assignmentNode->assignmentType))
						{
							Token id = { TokenType::IDENTIFIER, assignmentNode->resolveIdentifier(), assignmentNode->line() };
							result = compileNode(assignmentNode->value.get(), &id);

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
									Token assigned = { TokenType::IDENTIFIER, assignmentNode->resolveIdentifier(), assignmentNode->line() };
									move->result = assigned;
									result.push_back(move);
								}
							}
						}
						else
						{
							
						}

						return result;
					}
					case ExpressionNode::ExpressionType::FieldCall:
					{
						
					}
					case ExpressionNode::ExpressionType::FunctionCall:
					{
						
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
						constant->result = primaryNode->primary;
						constant->op = OP_CONST_LOW;
						if(result != nullptr)
						{
							constant->A = *result;
						}
						else
						{
							std::string temp("#");
							temp.append(std::to_string(temporaries++));
							constant->A = { TokenType::IDENTIFIER, temp, primaryNode->line() };
						}
						std::vector<std::shared_ptr<assembly>> chunk;
						chunk.push_back(constant);
						return chunk;
					}
				}
			}
		}
	}
}