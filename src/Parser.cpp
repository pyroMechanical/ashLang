#include "Parser.h"

#include <iostream>

namespace ash
{

	Parser::Parser(const char* source)
	:scanner(source)
	{
		
#define FN(fn) (std::bind(&fn, this, std::placeholders::_1))
#define	FN2(fn) (std::bind(&fn, this, std::placeholders::_1, std::placeholders::_2))
		hadError = false;
		panicMode = false;

		rules = {
		{FN(Parser::grouping),FN2(Parser::call), Precedence::CALL},    //[PAREN]
		{nullptr,                       nullptr, Precedence::NONE},    //[CLOSE_PAREN]
		{nullptr,                       nullptr, Precedence::NONE},    //[BRACE]
		{nullptr,                       nullptr, Precedence::NONE},    //[CLOSE_BRACE]
		{FN(Parser::grouping),          nullptr, Precedence::NONE},    //[BRACKET]
		{nullptr,                       nullptr, Precedence::NONE},    //[CLOSE_BRACKET]
		{nullptr,                       nullptr, Precedence::NONE},    //[COMMA]
		{nullptr,                       nullptr, Precedence::NONE},    //[DOT]
		{FN(Parser::unary), FN2(Parser::binary), Precedence::TERM},    //[MINUS]
		{nullptr,           FN2(Parser::binary), Precedence::TERM},    //[PLUS]
		{nullptr,                       nullptr, Precedence::NONE},    //[COLON]
		{nullptr,                       nullptr, Precedence::NONE},	   //[SEMICOLON]
		{nullptr,           FN2(Parser::binary), Precedence::FACTOR},  //[SLASH]
		{nullptr,           FN2(Parser::binary), Precedence::FACTOR},  //[STAR]
		{FN(Parser::unary), FN2(Parser::binary), Precedence::NONE},    //[BANG]
		{nullptr,           FN2(Parser::binary), Precedence::EQUALITY},    //[BANG_EQUAL]
		{nullptr,           FN2(Parser::binary), Precedence::EQUALITY},    //[EQUAL]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},    //[LESS]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},    //[LESS_EQUAL]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},    //[GREATER]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},    //[GREATER_EQUAL]
		{nullptr,                       nullptr, Precedence::NONE},	   //[TYPE]
		{nullptr,                       nullptr, Precedence::NONE},	   //[AUTO]
		{nullptr,                       nullptr, Precedence::NONE},	   //[ANY]
		{nullptr,                       nullptr, Precedence::NONE},	   //[MULTI]
		{FN(Parser::variable),          nullptr, Precedence::NONE},	   //[IDENTIFIER]
		{nullptr,                       nullptr, Precedence::NONE},	   //[DEF]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[TRUE]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[FALSE]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[NULL_]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[FLOAT]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[DOUBLE]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[CHAR]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[INT]
		{FN(Parser::literal),           nullptr, Precedence::NONE},	   //[STRING]
		{nullptr,                       nullptr, Precedence::NONE},	   //[IF]
		{nullptr,                       nullptr, Precedence::NONE},	   //[WHILE]
		{nullptr,                       nullptr, Precedence::NONE},	   //[FOR]
		{nullptr,                       nullptr, Precedence::NONE},	   //[RETURN]
		{nullptr,                       nullptr, Precedence::NONE},	   //[ELSE]
		{nullptr,           FN2(Parser::binary), Precedence::AND},	   //[AND]
		{nullptr,           FN2(Parser::binary), Precedence::OR},	   //[OR]
		{nullptr,                       nullptr, Precedence::NONE},	   //[BREAK]
		{nullptr,                       nullptr, Precedence::NONE},	   //[NEWLINE]
		{nullptr,                       nullptr, Precedence::NONE},	   //[ERROR]
		{nullptr,                       nullptr, Precedence::NONE},	   //[EOF]


		};

		advance();
		advance();
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

	void Parser::resolveNewlines()
	{
		std::vector<TokenType> ValidPrevious =
		{
			TokenType::CLOSE_PAREN,
			TokenType::CLOSE_BRACKET,
			TokenType::IDENTIFIER,
			TokenType::TRUE,
			TokenType::FALSE,
			TokenType::NULL_,
			TokenType::FLOAT,
			TokenType::DOUBLE,
			TokenType::CHAR,
			TokenType::INT,
			TokenType::STRING,
			TokenType::RETURN,
			TokenType::BREAK
		};

		std::vector<TokenType> ValidNext =
		{
			TokenType::PAREN,
			TokenType::FOR,
			TokenType::IF,
			TokenType::ELSE,
			TokenType::WHILE,
			TokenType::TRUE,
			TokenType::FALSE,
			TokenType::NULL_,
			TokenType::FLOAT,
			TokenType::DOUBLE,
			TokenType::CHAR,
			TokenType::INT,
			TokenType::STRING,
			TokenType::EOF_
		};

		if (current.type == TokenType::NEWLINE)
		{
			bool previousValid = false;
			for (int i = 0; i < ValidPrevious.size(); i++)
			{
				previousValid |= (previous.type == ValidPrevious[i]);
			}
			bool nextValid = false;
			for (int i = 0; i < ValidNext.size(); i++)
			{
				nextValid |= (next.type == ValidNext[i]);
			}

			if (previousValid && nextValid)
			{
				current.type = TokenType::SEMICOLON;
			}
			else
			{
				current = next;

				for (;;)
				{
					next = scanner.scanToken();
					if (next.type != TokenType::ERROR) break;
					errorAtCurrent(next.start);
				}

				if (previous.type == TokenType::DEF && current.type == TokenType::IDENTIFIER)
					current.type = TokenType::TYPE;

				if (current.type == TokenType::IDENTIFIER && next.type == TokenType::IDENTIFIER)
					current.type = TokenType::TYPE;
			}
		}
	}

	void Parser::advance()
	{
		previous = current;

		current = next;

		for (;;)
		{
			next = scanner.scanToken(); 
			if (next.type != TokenType::ERROR) break;
			errorAtCurrent(next.start);
		}

		if (previous.type == TokenType::DEF && current.type == TokenType::IDENTIFIER)
			current.type = TokenType::TYPE;

		if (current.type == TokenType::IDENTIFIER && next.type == TokenType::IDENTIFIER)
			current.type = TokenType::TYPE;

		resolveNewlines();
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

	ParseRule* Parser::getRule(TokenType type)
	{
		return &rules[(int)type];
	}

	std::unique_ptr<ExpressionNode> Parser::binary(std::unique_ptr<ExpressionNode> lhs, bool canAssign)
	{
		std::unique_ptr<BinaryNode> node = std::make_unique<BinaryNode>();
		node->left = std::move(lhs);
		node->op = previous;
		ParseRule* rule = getRule(previous.type);
		node->right = ParsePrecedence((Precedence)((int)rule->precedence + 1));
		return node;
	}

	std::unique_ptr<ExpressionNode> Parser::variable(bool canAssign)
	{
		std::unique_ptr<AssignmentNode> node = std::make_unique<AssignmentNode>();

		node->identifier = previous;

		if(canAssign && match(TokenType::COLON))
		{
			node->value = expression();
		}

		return node;
	}

	std::unique_ptr<ExpressionNode> Parser::call(std::unique_ptr<ExpressionNode> lhs, bool canAssign)
	{
		//TODO: field calls
		std::unique_ptr<FunctionCallNode> node = std::make_unique<FunctionCallNode>();
		node->left = std::move(lhs);
		if (!check(TokenType::CLOSE_PAREN))
		{
			while (!check(TokenType::CLOSE_PAREN))
			{
				node->arguments.push_back(expression());
				if (!check(TokenType::CLOSE_PAREN))
				{
					consume(TokenType::COMMA, "expected ',' between arguments.");
				}
			}
			consume(TokenType::CLOSE_PAREN, "expected ')' after final argument.");
		}

		return node;
	}

	std::unique_ptr<ExpressionNode> Parser::grouping(bool canAssign)
	{
		auto node = expression();
		consume(TokenType::CLOSE_PAREN, "Expected ')' after expresion.");
		return node;
	}

	std::unique_ptr<ExpressionNode> Parser::literal(bool canAssign)
	{
		std::unique_ptr<CallNode> node = std::make_unique<CallNode>();
		node->primary = previous;
		return node;
	}

	std::unique_ptr<ExpressionNode> Parser::unary(bool canAssign)
	{
		std::unique_ptr<UnaryNode> node = std::make_unique<UnaryNode>();
		node->op = previous;

		node->unary = ParsePrecedence(Precedence::UNARY);

		return node;
	}

	std::unique_ptr<DeclarationNode> Parser::declaration()
	{
		
		if (match(TokenType::DEF))
		{
			std::unique_ptr<TypeDeclarationNode> node = std::make_unique<TypeDeclarationNode>();
			consume(TokenType::TYPE, "expected type after 'def.'");
			node->typeDefined = previous;
			consume(TokenType::BRACE, "expected '{' before type definition.");
			node->fields = getParameters();
			consume(TokenType::CLOSE_BRACE, "expected '}' after type definition.");

			return node;
		}
		//todo: modules
		else if (match(TokenType::TYPE))
		{
			bool usign = false;
			Token type = previous;
			Token identifier;
			if (memcmp(previous.start, "unsigned", 8) == 0)
			{
				usign = true;
				consume(TokenType::TYPE, "expected type after 'unsigned.'");
				type = previous;
			}
			
			consume(TokenType::IDENTIFIER, "expected identifier after type.");
			identifier = previous;
			{
				if (match(TokenType::PAREN))
				{
					std::unique_ptr<FunctionDeclarationNode> node = std::make_unique<FunctionDeclarationNode>();
					node->usign = usign;
					node->type = type;
					node->identifier = identifier;
					node->parameters = getParameters();
					consume(TokenType::IDENTIFIER, "expected ')' after function parameters.");
					consume(TokenType::BRACE, "expected '{' before block.");
					node->body = block();

					return node;
				}
				else 
				{
					std::unique_ptr<VariableDeclarationNode> node = std::make_unique<VariableDeclarationNode>();
					node->usign = usign;
					node->type = type;
					node->identifier = identifier;
					if (match(TokenType::COLON))
					{
						node->value = expression();
					}
					consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");

					return node;
				}
			}
		}
		else
		{
			std::unique_ptr<StatementNode> stmt = statement();
			return stmt;
		}
	}

	std::unique_ptr<StatementNode> Parser::statement()
	{
		if (match(TokenType::FOR))
		{
			consume(TokenType::PAREN, "expected '(' after 'for.'");
			std::unique_ptr<ForStatementNode> node = std::make_unique<ForStatementNode>();
			if (match(TokenType::SEMICOLON))
			{
				node->declaration = nullptr;
			}
			else if (match(TokenType::TYPE))
			{
				bool usign = false;
				Token type = previous;
				Token identifier;
				if (previous.start == "unsigned")
				{
					usign = true;
					consume(TokenType::TYPE, "expected type after 'unsigned.'");
					type = previous;
				}
				consume(TokenType::IDENTIFIER, "expected identifier after type.");
				identifier = previous;
				std::unique_ptr<VariableDeclarationNode> declaration = std::make_unique<VariableDeclarationNode>();
				declaration->usign = usign;
				declaration->type = type;
				declaration->identifier = identifier;
				if (match(TokenType::COLON))
				{
					declaration->value = expression();
				}
				node->declaration = std::move(declaration);

				consume(TokenType::SEMICOLON, "expected ';' after variable declaration.");
			}
			else
			{
				node->declaration = expression();

				consume(TokenType::SEMICOLON, "expected ';' after expression.");
			}

			if (match(TokenType::SEMICOLON))
			{
				node->conditional = nullptr;
			}
			else
			{
				node->conditional = expression();

				consume(TokenType::SEMICOLON, "expected ';' after expression.");
			}

			if (match(TokenType::CLOSE_PAREN))
			{
				node->increment = nullptr;
			}
			else
			{
				node->increment = expression();

				consume(TokenType::CLOSE_PAREN, "expected ')' after expression.");
			}

			node->statement = statement();

			return node;
		}
		else if (match(TokenType::IF))
		{
			std::unique_ptr<IfStatementNode> node = std::make_unique<IfStatementNode>();
			consume(TokenType::PAREN, "expected '(' after 'if.'");
			node->condition = expression();
			consume(TokenType::CLOSE_PAREN, "expected')' after expression.");
			node->thenStatement = statement();
			if (match(TokenType::ELSE))
			{
				node->elseStatement = statement();
			}

			return node;
		}
		else if (match(TokenType::RETURN))
		{
			std::unique_ptr<ReturnStatementNode> node = std::make_unique<ReturnStatementNode>();
			node->returnValue = expression();
			consume(TokenType::SEMICOLON, "expected ';' after expression.");
			return node;
		}
		else if (match(TokenType::WHILE))
		{
			std::unique_ptr<WhileStatementNode> node = std::make_unique<WhileStatementNode>();
			consume(TokenType::PAREN, "expected '(' after 'while.'");
			node->condition = expression();
			consume(TokenType::CLOSE_PAREN, "expected')' after expression.");
			node->doStatement = statement();

			return node;
		}
		else if (match(TokenType::BRACE))
		{
			return block();
		}
		else
		{
			std::unique_ptr<ExpressionStatement> node = std::make_unique<ExpressionStatement>();
			node->expression = expression();
			consume(TokenType::SEMICOLON, "expected ';' after expression");
			return node;
		}
	}

	std::unique_ptr<ExpressionNode> Parser::expression()
	{
		return ParsePrecedence(Precedence::ASSIGNMENT);
	}

	std::unique_ptr<ExpressionNode> Parser::ParsePrecedence(Precedence precedence)
	{
		advance();
		std::function<std::unique_ptr<ExpressionNode>(bool)> prefixRule = getRule(previous.type)->prefix;
		if (prefixRule == nullptr)
		{
			error("expected expression.");
			return nullptr;
		}

		bool canAssign = precedence <= Precedence::ASSIGNMENT;
		std::unique_ptr<ExpressionNode> node = prefixRule(canAssign);

		while (precedence <= getRule(current.type)->precedence)
		{
			advance();
			std::function<std::unique_ptr<ExpressionNode>(std::unique_ptr<ExpressionNode>, bool)> infixRule = getRule(previous.type)->infix;
			node = infixRule(std::move(node), canAssign);
		}

		return node;
	}

	std::vector<parameter> Parser::getParameters()
	{
		std::vector<parameter> parameters;
		while (check(TokenType::TYPE) || check(TokenType::IDENTIFIER) || check(TokenType::COMMA))
		{
			parameter param;
			consume(TokenType::TYPE, "Expected type before identifier.");
			param.type = previous;
			consume(TokenType::IDENTIFIER, "Expected identifier after type.");
			param.identifier = previous;
			parameters.push_back(param);
			if (!match(TokenType::COMMA)) break;
		}
		return parameters;
	}

	std::unique_ptr<BlockNode> Parser::block()
	{
		std::unique_ptr<BlockNode> node = std::make_unique<BlockNode>();
		while (!check(TokenType::CLOSE_BRACE) && !check(TokenType::EOF_))
		{
			node->declarations.push_back(declaration());
		}
		consume(TokenType::CLOSE_BRACE, "Expected '}' after block.");

		return node;
	}

	std::unique_ptr<ProgramNode> Parser::parse()
	{
		std::unique_ptr<ProgramNode> node = std::make_unique<ProgramNode>();

		/*if (match(TokenType::LIBRARY))
		std::unique_ptr<LibraryNode> library;
		consume(TokenType::IDENTIFIER, "Expected a library name.");
		library.libraryIdentifier = previous;
		node.library = library;
		*/
		
		while (!match(TokenType::EOF_))
		{
			node->declarations.push_back(declaration());
		}

		return node;
	}
}