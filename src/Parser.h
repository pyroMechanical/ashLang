#pragma once

#include "Scanner.h"
#include "ParseTree.h"

#include <functional>

namespace ash
{
	enum class Precedence
	{
		NONE,
		ASSIGNMENT, // =
		OR, // ||
		AND, // &&
		EQUALITY, // ==, !=
		COMPARISON, // <, >, <=, >=
		TERM, // +, -
		FACTOR, // *, /
		UNARY, // !, -
		CALL, // ., ()
		PRIMARY
	};

	struct ParseRule
	{
		std::function<std::shared_ptr<ExpressionNode>(bool)> prefix;
		std::function<std::shared_ptr<ExpressionNode>(std::shared_ptr<ExpressionNode>, bool)> infix;
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

		void errorAt(Token* token, std::string message);
		void error(std::string message);
		void errorAtCurrent(std::string message);
		void advance();
		void consume(TokenType type, std::string message);
		bool check(TokenType type);
		bool match(TokenType type);

		void synchronize();

		void resolveNewlines();
		
	public:
		Parser(const char* source);

		std::shared_ptr<ExpressionNode> expression();
		std::shared_ptr<StatementNode> statement();
		std::shared_ptr<DeclarationNode> declaration();
		std::vector<parameter> getParameters();
		std::shared_ptr<BlockNode> block();
		ParseRule* getRule(TokenType type);


		std::shared_ptr<ExpressionNode> ParsePrecedence(Precedence precedence);
		std::shared_ptr<ExpressionNode> binary(std::shared_ptr<ExpressionNode> node, bool assign);
		std::shared_ptr<ExpressionNode> call(std::shared_ptr<ExpressionNode> node, bool assign);
		std::shared_ptr<ExpressionNode> grouping(bool assign);
		std::shared_ptr<ExpressionNode> assignment(std::shared_ptr<ExpressionNode> node, bool assign);
		std::shared_ptr<ExpressionNode> literal(bool assign);
		std::shared_ptr<ExpressionNode> unary(bool assign);
		std::shared_ptr<ExpressionNode> constructor(bool assign);

		std::shared_ptr<ProgramNode> parse();
	};
}