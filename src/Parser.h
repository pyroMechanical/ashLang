#pragma once

#include "Scanner.h"
#include "SyntaxTree.h"

namespace ash
{
	enum class Precedence
	{
		NONE,
		ASSIGNMENT, // :
		OR, // or
		AND, // and
		EQUALITY, // =, !=
		COMPARISON, // <, >, <=, >=
		TERM, // +, -
		FACTOR, // *, /
		UNARY, //~, !, -
		CALL, // ., ()
		PRIMARY
	};

	struct ParseRule
	{
		void (*prefix)(bool);
		void (*infix)(bool);
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
		SyntaxTree tree;
		node* currentNode;

		void errorAt(Token* token, const char* message);
		void error(const char* message);
		void errorAtCurrent(const char* message);
		void advance();
		void consume(TokenType type, const char* message);
		bool check(TokenType type);
		bool match(TokenType type);
		
		void binary(bool assign);
		void call(bool assign);
		void grouping(bool assign);
		void literal(bool assign);
		void unary(bool assign);
		void number(bool assign);
		void string(bool assign);
		void namedVariable(Token name, bool assign);

	public:
		Parser(const char* source)
			:scanner(source)
		{
			hadError = false;
			panicMode = false;
		}

		SyntaxTree parse();
	};
}