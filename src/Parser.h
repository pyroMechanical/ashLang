#pragma once

#include "Scanner.h"
#include "ParseTree.h"

#include <functional>

namespace ash
{
	enum class Precedence
	{
		NONE,
		ASSIGNMENT, // :
		OR, // ||
		AND, // &&
		EQUALITY, // =, !=
		COMPARISON, // <, >, <=, >=
		TERM, // +, -
		FACTOR, // *, /
		UNARY, // !, -
		CALL, // ., ()
		PRIMARY
	};

	struct ParseRule
	{
		std::function<std::unique_ptr<ExpressionNode>(bool)> prefix;
		std::function<std::unique_ptr<ExpressionNode>(std::unique_ptr<ExpressionNode>, bool)> infix;
		Precedence precedence;
	};

	class Parser
	{
	private:
		Token next;
		Token current;
		Token previous;
		bool hadError;
		bool panicMode;
		Scanner scanner;
		
		std::vector<ParseRule> rules;

		void errorAt(Token* token, const char* message);
		void error(const char* message);
		void errorAtCurrent(const char* message);
		void advance();
		void consume(TokenType type, const char* message);
		bool check(TokenType type);
		bool match(TokenType type);

		void resolveNewlines();
		
	public:
		Parser(const char* source);

		std::unique_ptr<ExpressionNode> expression();
		std::unique_ptr<StatementNode> statement();
		std::unique_ptr<DeclarationNode> declaration();
		std::vector<parameter> getParameters();
		std::unique_ptr<BlockNode> block();
		ParseRule* getRule(TokenType type);


		std::unique_ptr<ExpressionNode> ParsePrecedence(Precedence precedence);
		std::unique_ptr<ExpressionNode> binary(std::unique_ptr<ExpressionNode> node, bool assign);
		std::unique_ptr<ExpressionNode> call(std::unique_ptr<ExpressionNode> node, bool assign);
		std::unique_ptr<ExpressionNode> grouping(bool assign);
		std::unique_ptr<ExpressionNode> variable(bool assign);
		std::unique_ptr<ExpressionNode> literal(bool assign);
		std::unique_ptr<ExpressionNode> unary(bool assign);

		std::unique_ptr<ProgramNode> parse();
	};
}