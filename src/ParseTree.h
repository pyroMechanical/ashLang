#pragma once
#include "Scanner.h"

#include <memory>
#include <vector>
namespace ash
{
	struct ParseNode
	{
		virtual void print() = 0;
		virtual void type() = 0;
	};

	struct ProgramNode : public ParseNode
	{
		//std::unique_ptr<LibraryNode> library;
		//std::vector<std::unique_ptr<ImportNode>> imports;
		std::vector<std::unique_ptr<DeclarationNode>> declarations;
	};

	struct LibraryNode : public ParseNode
	{
		Token libraryIdentifier;
	};

	struct ImportNode : public ParseNode
	{
		std::vector<Token> usingList;
		Token fromIdentifier;
	};

	struct DeclarationNode : public ParseNode
	{
	
	};

	struct TypeDeclarationNode : public DeclarationNode
	{
		Token typeDefined;
		std::vector<parameter> fields;
	};

	struct FunctionDeclarationNode : public DeclarationNode
	{
		Token type;
		Token identifier;
		std::vector<parameter> parameters;
		std::unique_ptr<BlockNode> body;
	};

	struct VariableDeclarationNode : public DeclarationNode
	{
		Token type;
		Token identifier;
		std::unique_ptr<ExpressionNode> value;
	};

	struct StatementNode : public DeclarationNode
	{
	
	};

	struct ExpressionStatement : public StatementNode
	{
		std::unique_ptr<ExpressionNode> expression;
	};

	struct ForStatementNode : public StatementNode
	{
		std::unique_ptr<VariableDeclarationNode> declaration;
		std::unique_ptr<ExpressionNode> expression; //if no declaration, then this
		std::unique_ptr<ExpressionNode> conditional;
		std::unique_ptr<ExpressionNode> increment;
		std::unique_ptr<StatementNode> statement;
	};

	struct IfStatementNode : public StatementNode
	{
		std::unique_ptr<ExpressionNode> condition;
		std::unique_ptr<StatementNode> thenStatement;
		std::unique_ptr<StatementNode> elseStatement;
	};

	struct ReturnStatementNode : public StatementNode
	{
		std::unique_ptr<ExpressionNode> returnValue;
	};

	struct WhileStatementNode : public StatementNode
	{
		std::unique_ptr<ExpressionNode> condition;
		std::unique_ptr<StatementNode> doStatement;
	};

	struct BlockNode : public StatementNode
	{
		std::vector<std::unique_ptr<DeclarationNode>> declarations;
	};

	struct ExpressionNode : public ParseNode
	{
		std::unique_ptr<PseudoAssignmentNode> assignment;
	};


	struct PseudoAssignmentNode : public ParseNode
	{
	};

	struct AssignmentNode : public PseudoAssignmentNode
	{
		std::unique_ptr<CallNode> call;
		Token assignedIdentifier;
		std::unique_ptr<PseudoAssignmentNode> assignment;
	};

	struct OrNode : public PseudoAssignmentNode
	{
		std::unique_ptr<AndNode> left;
		std::unique_ptr<AndNode> right;
	};

	struct AndNode :public ParseNode
	{
		std::unique_ptr<EqualityNode> left;
		std::unique_ptr<EqualityNode> right;
	};

	struct EqualityNode : public ParseNode
	{
		std::unique_ptr<ComparisonNode> left;
		bool not; // true: != false: =
		std::unique_ptr<ComparisonNode> right;
	};

	struct ComparisonNode : public ParseNode
	{
		std::unique_ptr<TermNode> left;
		Token op;
		std::unique_ptr<TermNode> right;
	};

	struct TermNode : public ParseNode
	{
		std::unique_ptr<FactorNode> left;
		bool sub; // true: - false: +
		std::unique_ptr<FactorNode> right;
	};

	struct FactorNode : public ParseNode
	{
		std::unique_ptr<PseudoUnaryNode> left;
		bool div; // true: / false: *
		std::unique_ptr<PseudoUnaryNode> right;
	};

	struct PseudoUnaryNode : public ParseNode
	{
	};

	struct UnaryNode : public PseudoUnaryNode
	{
		Token op;
		std::unique_ptr<UnaryNode> unary;
	};

	struct CallNode : public PseudoUnaryNode
	{
		Token primary;
	};

	struct FunctionCallNode : public CallNode
	{
		std::vector<ExpressionNode> arguments;
		std::unique_ptr<CallNode> nextCall;
	};

	struct FieldCallNode : public CallNode
	{
		Token field;
		std::unique_ptr<CallNode> nextCall;
	};

	struct parameter
	{
		Token type;
		Token identifier;
	};
}