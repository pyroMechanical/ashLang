#include "ControlFlowAnalysis.h"

namespace ash
{
	std::shared_ptr<ControlFlowGraph> ControlFlowAnalysis::createCFG(std::shared_ptr<ProgramNode> ast)
	{
		result = std::make_shared<ControlFlowGraph>();
		auto astFunc = ast->convertToFunctionNode();
		traverseNodes(astFunc.get());
		return result;
	}

	void ControlFlowAnalysis::traverseNodes(ParseNode* node)
	{
		switch (node->nodeType())
		{
			case NodeType::FunctionDeclaration:
			{
				auto funcNode = (FunctionDeclarationNode*)node;
				std::shared_ptr<CFGFunctionDeclarationNode> resultFunction = std::make_shared<CFGFunctionDeclarationNode>();
				resultFunction->identifier = funcNode->identifier;
				resultFunction->type = funcNode->type;
				resultFunction->usign = funcNode->usign;
				resultFunction->parameters = funcNode->parameters;
				resultFunction->body = std::dynamic_pointer_cast<CFGBlockNode>(serializeNode(funcNode->body.get(), nullptr));
				result->basicBlocks.push_back(resultFunction);
				break;
			}
		}
	}

	std::shared_ptr<DeclarationNode> ControlFlowAnalysis::serializeNode(ParseNode* node, std::shared_ptr<ParseNode> next)
	{
		switch (node->nodeType())
		{
			case NodeType::VariableDeclaration:
			{
				auto varNode = (VariableDeclarationNode*)node;
				std::shared_ptr<CFGVariableDeclarationNode> result = std::make_shared<CFGVariableDeclarationNode>();
				result->identifier = varNode->identifier;
				result->type = varNode->type;
				result->usign = varNode->usign;
				result->value = varNode->value;
				result->next = next;
				return result;
			}
			case NodeType::TypeDeclaration:
			{
				auto typeNode = (TypeDeclarationNode*)node;
				std::shared_ptr<CFGTypeDeclarationNode> result = std::make_shared<CFGTypeDeclarationNode>();
				result->typeDefined = typeNode->typeDefined;
				result->fields = typeNode->fields;
				result->next = next;
				return result;
			}
			case NodeType::IfStatement:
			{ 
				auto ifNode = (IfStatementNode*)node;
				std::shared_ptr<CFGIFStatementNode> result = std::make_shared<CFGIFStatementNode>();
				result->condition = ifNode->condition;
				result->thenStatement = std::dynamic_pointer_cast<StatementNode>(serializeNode(ifNode->thenStatement.get(), nullptr));
				result->elseStatement = std::dynamic_pointer_cast<StatementNode>(serializeNode(ifNode->elseStatement.get(), nullptr));
				result->next = next;
				return result;
			}
			case NodeType::WhileStatement:
			{
				auto whileNode = (WhileStatementNode*)node;
				std::shared_ptr<CFGWhileStatementNode> result = std::make_shared<CFGWhileStatementNode>();
				result->condition = whileNode->condition;
				result->doStatement = std::dynamic_pointer_cast<StatementNode>(serializeNode(whileNode->doStatement.get(), nullptr));
				result->next = next;
				return result;
			}
			case NodeType::ForStatement:
			{
				auto forNode = (ForStatementNode*)node;
				std::shared_ptr<CFGForStatementNode> result = std::make_shared<CFGForStatementNode>();
				result->declaration = forNode->declaration;
				result->conditional = forNode->conditional;
				result->increment = forNode->increment;
				result->statement = std::dynamic_pointer_cast<StatementNode>(serializeNode(forNode->statement.get(), nullptr));
				result->next = next;
				return result;
			}
			case NodeType::ReturnStatement:
			{
				auto returnNode = (ReturnStatementNode*)node;
				std::shared_ptr<CFGReturnStatementNode> result = std::make_shared<CFGReturnStatementNode>();
				result->returnValue = returnNode->returnValue;
				result->next = next;
				return result;
			}
			case NodeType::ExpressionStatement:
			{
				auto exprNode = (ExpressionStatement*)node;
				std::shared_ptr<CFGExpressionStatement> result = std::make_shared<CFGExpressionStatement>();
				result->expression = exprNode->expression;
				result->next = next;
				return result;
			}
			case NodeType::Block:
			{
				auto blockNode = (BlockNode*)node;
				std::shared_ptr<CFGBlockNode> result = std::make_shared<CFGBlockNode>();
				std::shared_ptr<DeclarationNode> declarationList = nullptr;
				for (size_t i = blockNode->declarations.size(); i > 0 ; i--)
				{
					std::shared_ptr<DeclarationNode> declaration = blockNode->declarations[i - 1];

					switch (declaration->nodeType())
					{
						case NodeType::FunctionDeclaration:
						{
							traverseNodes(declaration.get());
							break;
						}
						default:
						{
							auto cfgDeclaration = serializeNode(declaration.get(), declarationList);
							declarationList = cfgDeclaration; 
							break;					
						}
					}
				}
				result->declarations = declarationList;
				result->scope = blockNode->scope;
				return result;
			}

		}
	}
}