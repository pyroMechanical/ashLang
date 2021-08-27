#include "Scanner.h"

#include <string>

namespace ash
{
	namespace util
	{
		inline static bool isAlpha(char c)
		{
			return (c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z');
		}

		inline static bool isNumeric(char c)
		{
			return c >= '0' && c <= '9';
		}

		inline static bool isAlphaNumeric(char c)
		{
			return isAlpha(c) || isNumeric(c);
		}
	}

	inline bool Scanner::isAtEnd()
	{
		return *current == '\0';
	}

	inline char Scanner::advance()
	{
		current++;
		return current[-1];
	}

	inline char Scanner::peek(int index)
	{
		if (index > (int)strlen(current) || isAtEnd()) return '\0';
		return current[index];
	}

	inline bool Scanner::match(char expected)
	{
		if (isAtEnd()) return false;
		if (*current != expected) return false;
		current++;
		return true;
	}

	Token Scanner::makeToken(TokenType type)
	{
		Token token;
		token.type = type;
		token.string = std::string(start, (int)(current - start));
		token.line = line;
		return token;
	}
	
	Token Scanner::errorToken(const char* message)
	{
		Token token;
		token.type = TokenType::ERROR;
		token.string = std::string(message);
		token.line = line;
		return token;
	}

	Token Scanner::identifierToken()
	{
		//advance();
		while (util::isAlphaNumeric(peek()) || peek() == '_') advance();
		if (*current == '!') advance();
		return makeToken(identifierType());
	}

	Token Scanner::numberToken()
	{
		while (util::isNumeric(peek())) advance();

		if (peek() == '.')
		{
			advance();

			while (util::isNumeric(peek())) advance();
			if(peek() == 'f')
			{
				advance();
				return makeToken(TokenType::FLOAT);
			}
			return makeToken(TokenType::DOUBLE);
			
		}
		return makeToken(TokenType::INT);
	}

	Token Scanner::stringToken()
	{
		while (peek() != '"' && !isAtEnd())
		{
			if (peek() == '\n') line++;
			advance();
		}

		if (isAtEnd()) return errorToken("Unterminated string.");
		advance();
		return makeToken(TokenType::STRING);
	}

	Token Scanner::charToken()
	{
		if (peek() == '\\') advance();
		advance();
		if (peek() != '\'') return errorToken("Unterminated char.");
		advance();
		return makeToken(TokenType::CHAR);
	}

	Token Scanner::scanToken()
	{
		skipWhitespace();
		start = current;
		if (isAtEnd()) return makeToken(TokenType::EOF_);

		char c = advance();
		if (util::isAlpha(c) || c == '_') return identifierToken();
		if (util::isNumeric(c)) return numberToken();

		switch(c)
		{
		case '(': return makeToken(TokenType::PAREN);
		case ')': return makeToken(TokenType::CLOSE_PAREN);
		case '{': return makeToken(TokenType::BRACE);
		case '}': return makeToken(TokenType::CLOSE_BRACE); 
		case '[': return makeToken(TokenType::BRACKET);
		case ']': return makeToken(TokenType::CLOSE_BRACKET);
		case ';': return makeToken(TokenType::SEMICOLON);
		case ',': return makeToken(TokenType::COMMA);
		case '.': return makeToken(TokenType::DOT);
		case '-': return makeToken(TokenType::MINUS);
		case '+': return makeToken(TokenType::PLUS);
		case '/': return makeToken(TokenType::SLASH);
		case '*': return makeToken(TokenType::STAR);
		case '=': return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
		case '\n': line++; return makeToken(TokenType::NEWLINE);
		case '!': return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
		case '<': return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
		case '>': return makeToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
		//case '"': return stringToken();
		//case '\'': return charToken();
		}

		return errorToken("Unexpected character.");
	}

	void Scanner::skipWhitespace()
	{
		for (;;)
		{
			char c = peek();
			switch (c)
			{
				case ' ':
				case '\r':
				case '\t':
					advance();
					break;
				case '/':
				{
					int nestedComments = 0;
					if (peek(1) == '/')
					{
						while (peek() != '\n' && !isAtEnd()) advance();
					}
					else if (peek(1) == '*')
					{
						nestedComments++;
						while (nestedComments > 0 && !isAtEnd())
						{
							advance();
							if (peek() == '/' && peek(1) == '*')
								nestedComments++;
							if (peek(2) == '*' && peek(3) == '/')
								nestedComments--;
						}
					}
					else return;
					break;
				}
				default: 
					return;
			}
		}
	}

	TokenType Scanner::checkKeyword(int tokenStart, int length, const char* rest, TokenType type)
	{
		if (current - start == tokenStart + (size_t)length && memcmp(start + tokenStart, rest, length) == 0) return type;
		return TokenType::IDENTIFIER;
	}

	TokenType Scanner::identifierType()
	{
		switch (start[0])
		{
		case 'a':
			if (current - start > 1)
			{
				switch (start[1])
				{
				case 'n':
					if (current - start > 2)
					{
						switch (start[2])
						{
						case 'd': return TokenType::AND;
						case 'y': return TokenType::ANY;
						}
					}
					break;
				case 'u':
					return checkKeyword(2, 2, "to", TokenType::AUTO);
				}
			}
			break;
		case 'd': return checkKeyword(1, 2, "ef", TokenType::DEF);
		case 'e': return checkKeyword(1, 3, "lse", TokenType::ELSE);
		case 'f':
			if (current - start > 1)
				switch(start[1])
				{
				case 'a': return checkKeyword(2, 3, "lse", TokenType::FALSE);
				case 'o': return checkKeyword(2, 1, "r", TokenType::FOR);
				}
			break;
		case 'i': return checkKeyword(1, 1, "f", TokenType::IF);
		case 'n': return checkKeyword(1, 3, "ull", TokenType::NULL_);
		case 'o': return checkKeyword(1, 1, "r", TokenType::OR);
		case 'r': return checkKeyword(1, 5, "eturn", TokenType::RETURN);
		case 't': return checkKeyword(1, 3, "rue", TokenType::TRUE);
		case 'w': return checkKeyword(1, 4, "hile", TokenType::WHILE);
		}
		return TokenType::IDENTIFIER;
	}
}