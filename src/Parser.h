#pragma once

#include "Scanner.h"
#include "SyntaxTree.h"

#include <functional>

namespace ash
{
	enum class Precedence
	{
		NONE,
		ASSIGNMENT, // :
		EQUALITY, // =, !=
		OR, // |
		AND, // &
		COMPARISON, // <, >, <=, >=
		TERM, // +, -
		FACTOR, // *, /
		UNARY, //~, !, -
		CALL, // ., ()
		PRIMARY
	};

	class Parser;

	struct ParseRule
	{
		std::function<void(bool)> prefix;
		std::function<void(bool)> infix;
		Precedence precedence;
	};

	class Parser
	{
	private:
		Token current;
		Token previous;
		bool hadError;
		bool panicMode;
		Scanner scanner;
		node* currentNode;
		SyntaxTree tree = nullptr;

		std::vector<ParseRule> rules;

		void errorAt(Token* token, const char* message);
		void error(const char* message);
		void errorAtCurrent(const char* message);
		void advance();
		void consume(TokenType type, const char* message);
		bool check(TokenType type);
		bool match(TokenType type);
		
	public:
		Parser(const char* source);

		void expression();
		void statement();
		void declaration();
		void typeDefinition();
		void functionDeclaration();
		void variableDeclaration();
		static ParseRule* getRule(TokenType type);
		static void ParsePrecedence(Precedence precedence);

		void binary(bool assign);
		void call(bool assign);
		void grouping(bool assign);
		void literal(bool assign);
		void unary(bool assign);
		void number(bool assign);
		void string(bool assign);
		void namedVariable(Token name, bool assign);

		SyntaxTree parse();
	};
}