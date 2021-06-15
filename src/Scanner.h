#pragma once


namespace ash
{
	enum class TokenType
	{
		LEFT_PAREN, RIGHT_PAREN,
		LEFT_BRACE, RIGHT_BRACE,
		COMMA, DOT, MINUS, PLUS,
		COLON, SEMICOLON, SLASH, STAR,
		BANG, BANG_EQUAL, EQUAL,
		LESS, LESS_EQUAL,
		GREATER, GREATER_EQUAL,
		//Types
		AUTO, ANY, MULTI,
		IDENTIFIER, DEF,
		//Literals
		TRUE, FALSE, NULL_,
		FLOAT, INT, STRING,
		//Keywords
		IF, WHILE, FOR,
		RETURN, ELSE,
		AND, OR,

		ERROR, EOF
	};

	struct Token
	{
		TokenType type;
		const char* start;
		int length;
		int line;
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

		Token scanToken();

		TokenType checkKeyword(int start, int length, const char* rest, TokenType type);
		TokenType identifierType();

	public:
		Scanner(const char* source)
			:start(source), current(source), line(1) {}
		~Scanner() = default;
	};
}