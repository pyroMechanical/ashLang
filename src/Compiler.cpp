#include "Compiler.h"
#include "Semantics.h"
#include "ControlFlowAnalysis.h"
#include <string>

namespace ash
{
	bool Compiler::compile(const char* source)
	{
		Parser parser(source);

		auto ast = parser.parse();

		Semantics analyzer;

		ast->print(0);

		ast = analyzer.findSymbols(ast);

		ast->print(0);

		ControlFlowAnalysis cfa;

		std::shared_ptr<ControlFlowGraph> cfg = cfa.createCFG(ast);

		for (const auto func : cfg->basicBlocks)
		{
			func->print(0);
		}

		//pseudochunk result = precompile(ast);

		return false;
	}

	pseudochunk Compiler::precompile(std::shared_ptr<ProgramNode> ast)
	{
		pseudochunk chunk;
		for (const auto& declaration : ast->declarations)
		{
			auto nextCode = compileNode((ParseNode*)declaration.get());
			chunk.code.insert(chunk.code.end(), nextCode.begin(), nextCode.end());
		}

		return chunk;
	}

	std::vector<std::shared_ptr<assembly>> Compiler::compileNode(ParseNode* node)
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

				auto conditionChunk = compileNode((ParseNode*)ifNode->condition.get());
				ifChunk.insert(ifChunk.end(), conditionChunk.begin(), conditionChunk.end());
				ifChunk.push_back(thenJump);
				auto elseChunk = compileNode((ParseNode*)ifNode->elseStatement.get());
				ifChunk.insert(ifChunk.end(), elseChunk.begin(), elseChunk.end());
				ifChunk.push_back(exitJump);
				ifChunk.push_back(elseLabel);
				auto thenChunk = compileNode((ParseNode*)ifNode->thenStatement.get());
				ifChunk.insert(ifChunk.end(), thenChunk.begin(), thenChunk.end());
				ifChunk.push_back(exitLabel);

				return ifChunk;
			}

			case NodeType::WhileStatement:
			{
				std::shared_ptr<label> loopLabel = std::make_shared<label>();
				loopLabel->label = jumpLabels++;
				std::shared_ptr<label> exitLabel = std::make_shared<label>();
				exitLabel->label = jumpLabels++;
				std::shared_ptr<label> conditionLabel = std::make_shared<label>();
				conditionLabel->label = jumpLabels++;
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
				auto conditionChunk = compileNode((ParseNode*)whileNode->condition.get());
				whileChunk.insert(whileChunk.end(), conditionChunk.begin(), conditionChunk.end());
				whileChunk.push_back(conditionJump);
				whileChunk.push_back(exitJump);
				whileChunk.push_back(conditionLabel);
				auto stmtChunk = compileNode((ParseNode*)whileNode->doStatement.get());
				whileChunk.insert(whileChunk.end(), stmtChunk.begin(), stmtChunk.end());
				whileChunk.push_back(loopJump);
				whileChunk.push_back(exitLabel);

				return whileChunk;
			}

			case NodeType::ForStatement:
			{
				std::shared_ptr<label> loopLabel = std::make_shared<label>();
				loopLabel->label = jumpLabels++;
				std::shared_ptr<label> exitLabel = std::make_shared<label>();
				exitLabel->label = jumpLabels++;
				std::shared_ptr<label> conditionLabel = std::make_shared<label>();
				conditionLabel->label = jumpLabels++;
				std::shared_ptr<relativeJump> loopJump = std::make_shared<relativeJump>();
				loopJump->op = OP_RELATIVE_JUMP;
				loopJump->jumpLabel = loopLabel->label;
				std::shared_ptr<relativeJump> exitJump = std::make_shared<relativeJump>();
				exitJump->op = OP_RELATIVE_JUMP;
				exitJump->jumpLabel = exitLabel->label;
				std::shared_ptr<relativeJump> conditionJump = std::make_shared<relativeJump>();
				conditionJump->op = OP_RELATIVE_JUMP_IF_TRUE;
				conditionJump->jumpLabel = conditionLabel->label;

				ForStatementNode* forNode = (ForStatementNode*)node;
				std::vector<std::shared_ptr<assembly>> forChunk;
				auto declarationChunk = compileNode((ParseNode*)forNode->declaration.get());
				forChunk.insert(forChunk.end(), declarationChunk.begin(), declarationChunk.end());
				forChunk.push_back(loopLabel);
				auto conditionChunk = compileNode((ParseNode*)forNode->conditional.get());
				forChunk.insert(forChunk.end(), conditionChunk.begin(), conditionChunk.end());
				forChunk.push_back(conditionJump);
				forChunk.push_back(exitJump);
				forChunk.push_back(conditionLabel);
				auto stmtChunk = compileNode((ParseNode*)forNode->statement.get());
				forChunk.insert(forChunk.end(), stmtChunk.begin(), stmtChunk.end());
				auto incrementChunk = compileNode((ParseNode*)forNode->increment.get());
				forChunk.insert(forChunk.end(), incrementChunk.begin(), incrementChunk.end());
				forChunk.push_back(loopJump);
				forChunk.push_back(exitLabel);

				return forChunk;
			}

			case NodeType::Expression:
			{
				ExpressionNode* exprNode = (ExpressionNode*)node;
				switch (exprNode->expressionType())
				{
					case ExpressionNode::ExpressionType::Binary:
					{

					}
				}
			}
		}
	}
}