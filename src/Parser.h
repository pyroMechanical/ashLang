#pragma once

#include "Scanner.h"

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
	};
}