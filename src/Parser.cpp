#include "Parser.h"

#include <iostream>

namespace ash
{

	Parser::Parser(const char* source)
	:scanner(source)
	{
		
#define FN(fn) (std::bind(&fn, this, std::placeholders::_1))
		hadError = false;
		panicMode = false;

		rules = {
		{FN(Parser::grouping), FN(Parser::call), Precedence::CALL},    //[PAREN]
		{nullptr,                       nullptr, Precedence::NONE},    //[CLOSE_PAREN]
		{nullptr,                       nullptr, Precedence::NONE},    //[BRACE]
		{nullptr,                       nullptr, Precedence::NONE},    //[CLOSE_BRACE]
		{FN(Parser::grouping),          nullptr, Precedence::NONE},    //[BRACKET]
		{nullptr,                       nullptr, Precedence::NONE},    //[CLOSE_BRACKET]
		{nullptr,                       nullptr, Precedence::NONE},    //[COMMA]
		{nullptr,                       nullptr, Precedence::NONE},    //[DOT]
		{FN(Parser::unary),  FN(Parser::binary), Precedence::TERM},    //[MINUS]
		{nullptr,            FN(Parser::binary), Precedence::TERM},    //[PLUS]
		{nullptr,                       nullptr, Precedence::NONE},    //[COLON]
		{nullptr,                       nullptr, Precedence::NONE},	   //[SEMICOLON]
		{nullptr,            FN(Parser::binary), Precedence::FACTOR},  //[SLASH]
		{nullptr,            FN(Parser::binary), Precedence::FACTOR},  //[STAR]
		{FN(Parser::unary),             nullptr, Precedence::NONE},	   //[BANG]
		{nullptr,                       nullptr, Precedence::NONE},	   //[BANG_EQUAL]
		{nullptr,                       nullptr, Precedence::NONE},	   //[EQUAL]
		{nullptr,                       nullptr, Precedence::NONE},	   //[LESS]
		{nullptr,                       nullptr, Precedence::NONE},	   //[LESS_EQUAL]
		{nullptr,                       nullptr, Precedence::NONE},	   //[GREATER]
		{nullptr,                       nullptr, Precedence::NONE},	   //[GREATER_EQUAL]
		{nullptr,                       nullptr, Precedence::NONE},	   //[TYPE]
		{nullptr,                       nullptr, Precedence::NONE},	   //[IDENTIFIER]
		};

		advance();
		while (!match(TokenType::EOF_))
		{
			declaration();
		}
	}

	void Parser::errorAt(Token* token, const char* message)
	{
		if (panicMode) return;
		panicMode = true;
		std::cerr << token->line << " Error" << std::endl;

		if(token->type == TokenType::EOF_)
		{
			std::cerr << " at end";
		}
		else if (token->type == TokenType::ERROR)
		{
			
		}
		else
		{
			fprintf(stderr, " at '%.*s'", token->length, token->start);
		}

		fprintf(stderr, ": %s\n", message);
		hadError = true;
	}

	void Parser::error(const char* message)
	{
		errorAt(&previous, message);
	}

	void Parser::errorAtCurrent(const char* message)
	{
		errorAt(&current, message);
	}

	void Parser::advance()
	{
		previous = current;


		for (;;)
		{
			current = scanner.scanToken();
			if (current.type != TokenType::ERROR) break;
			errorAtCurrent(current.start);
		}
	}

	void Parser::consume(TokenType type, const char* message)
	{
		if(check(type))
		{
			advance();
			return;
		}

		errorAtCurrent(message);
	}

	bool Parser::check(TokenType type)
	{
		return current.type == type;
	}

	bool Parser::match(TokenType type)
	{
		if (!check(type)) return false;
		advance();
		return true;
	}

	void Parser::binary(bool canAssign)
	{

	}

	void Parser::call(bool canAssign)
	{

	}

	void Parser::grouping(bool canAssign)
	{

	}

	void Parser::literal(bool canAssign)
	{

	}

	void Parser::unary(bool canAssign)
	{
		
	}

	void Parser::declaration()
	{
		
	}
}