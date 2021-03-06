#pragma once
#include <string>

namespace ash
{
	enum class TokenType : int
	{
		PAREN, CLOSE_PAREN,
		BRACE, CLOSE_BRACE,
		BRACKET, CLOSE_BRACKET,
		COMMA, DOT, MINUS, PLUS,
		EQUAL, SEMICOLON, SLASH, STAR,
		BANG, BANG_EQUAL, EQUAL_EQUAL,
		LESS, LESS_EQUAL,
		GREATER, GREATER_EQUAL,
		TYPE, //not used in the scanner; the parser will use lookahead to resolve types from identifier tokens
		AUTO, ANY, MULTI,
		IDENTIFIER, DEF,
		//Literals
		TRUE, FALSE, 
		FLOAT, DOUBLE, CHAR, INT, STRING,
		//Keywords
		IF, WHILE, FOR,
		RETURN, ELSE,
		AND, OR, NOT,
		BIT_AND, BIT_OR,
		BIT_SHIFT_LEFT, BIT_SHIFT_RIGHT,
		BREAK, 
		//special token for resolving newline semicolons
		NEWLINE,

		ERROR, EOF_
	};

	struct Token
	{
		TokenType type = TokenType::ERROR;
		std::string string;
		int line = 0;
	};

	class Scanner
	{
	private:
		const char* start;
		const char* current;
		int line;

		bool isAtEnd();
		char advance();
		char peek(int index = 0);

		bool match(char expected);

		void skipWhitespace();

		Token makeToken(TokenType type);
		Token errorToken(const char* message);
		Token identifierToken();
		Token numberToken();
		Token stringToken();
		Token charToken();

		TokenType checkKeyword(int start, int length, const char* rest, TokenType type);
		TokenType identifierType();

	public:
		Scanner(const char* source)
			:start(source), current(source), line(1) {}
		~Scanner() = default;

		Token scanToken();
	};
}