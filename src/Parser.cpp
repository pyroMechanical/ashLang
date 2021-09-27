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
		inExpression = 0;

		rules = {
		{FN(Parser::grouping),FN2(Parser::call),       Precedence::CALL},   //[PAREN]
		{nullptr,                       nullptr,       Precedence::NONE},   //[CLOSE_PAREN]
		{FN(Parser::constructor),       nullptr,       Precedence::CALL},   //[BRACE]
		{nullptr,                       nullptr,       Precedence::NONE},   //[CLOSE_BRACE]
		{nullptr,       FN2(Parser::arrayIndex),       Precedence::CALL},   //[BRACKET]
		{nullptr,                       nullptr,       Precedence::NONE},   //[CLOSE_BRACKET]
		{nullptr,                       nullptr,       Precedence::NONE},   //[COMMA]
		{nullptr,             FN2(Parser::call),       Precedence::CALL},   //[DOT]
		{FN(Parser::unary), FN2(Parser::binary),       Precedence::TERM},   //[MINUS]
		{nullptr,           FN2(Parser::binary),       Precedence::TERM},   //[PLUS]
		{nullptr,       FN2(Parser::assignment), Precedence::ASSIGNMENT},   //[EQUAL]
		{nullptr,                       nullptr,       Precedence::NONE},	//[SEMICOLON]
		{nullptr,           FN2(Parser::binary),     Precedence::FACTOR},   //[SLASH]
		{nullptr,           FN2(Parser::binary),     Precedence::FACTOR},   //[STAR]
		{FN(Parser::unary),             nullptr,       Precedence::NONE},   //[BANG]
		{nullptr,           FN2(Parser::binary),   Precedence::EQUALITY},   //[BANG_EQUAL]
		{nullptr,           FN2(Parser::binary),   Precedence::EQUALITY},   //[EQUAL_EQUAL]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},   //[LESS]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},   //[LESS_EQUAL]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},   //[GREATER]
		{nullptr,           FN2(Parser::binary), Precedence::COMPARISON},   //[GREATER_EQUAL]
		{nullptr,                       nullptr,       Precedence::NONE},	//[TYPE]
		{nullptr,                       nullptr,       Precedence::NONE},	//[AUTO]
		{nullptr,                       nullptr,       Precedence::NONE},	//[ANY]
		{nullptr,                       nullptr,       Precedence::NONE},	//[MULTI]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[IDENTIFIER]
		{nullptr,                       nullptr,       Precedence::NONE},	//[DEF]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[TRUE]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[FALSE]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[FLOAT]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[DOUBLE]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[CHAR]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[INT]
		{FN(Parser::literal),           nullptr,       Precedence::NONE},	//[STRING]
		{nullptr,                       nullptr,       Precedence::NONE},	//[IF]
		{nullptr,                       nullptr,       Precedence::NONE},	//[WHILE]
		{nullptr,                       nullptr,       Precedence::NONE},	//[FOR]
		{nullptr,                       nullptr,       Precedence::NONE},	//[RETURN]
		{nullptr,                       nullptr,       Precedence::NONE},	//[ELSE]
		{nullptr,           FN2(Parser::binary),        Precedence::AND},	//[AND]
		{nullptr,           FN2(Parser::binary),         Precedence::OR},	//[OR]
		{FN(Parser::unary),             nullptr,       Precedence::NONE},	//[NOT]
		{nullptr,           FN2(Parser::binary),    Precedence::BIT_AND},	//[BIT_AND]
		{nullptr,           FN2(Parser::binary),     Precedence::BIT_OR},	//[BIT_OR]
		{nullptr,           FN2(Parser::binary),      Precedence::SHIFT},	//[BIT_SHIFT_LEFT]
		{nullptr,           FN2(Parser::binary),      Precedence::SHIFT},   //[BIT_SHIFT_RIGHT]
		{nullptr,                       nullptr,       Precedence::NONE},	//[BREAK]
		{nullptr,                       nullptr,       Precedence::NONE},	//[NEWLINE]
		{nullptr,                       nullptr,       Precedence::NONE},	//[ERROR]
		{nullptr,                       nullptr,       Precedence::NONE},	//[EOF]
		};

		advance();
		advance();
	}

	void Parser::errorAt(Token* token, std::string message)
	{
		if (panicMode) return;
		panicMode = true;
		std::cerr << token->line << " Error ";

		if(token->type == TokenType::EOF_)
		{
			std::cerr << " at end: ";
		}
		else if (token->type == TokenType::ERROR)
		{
			
		}
		else
		{
			std::cerr << "at " << token->string << ": ";
		}

		std::cerr << message << std::endl;
		hadError = true;
	}

	void Parser::error(std::string message)
	{
		errorAt(&previous, message);
	}

	void Parser::errorAtCurrent(std::string message)
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
			TokenType::CLOSE_BRACE,
			TokenType::FOR,
			TokenType::DEF,
			TokenType::IF,
			TokenType::TYPE,
			TokenType::IDENTIFIER,
			TokenType::ELSE,
			TokenType::WHILE,
			TokenType::TRUE,
			TokenType::FALSE,
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
			if (inExpression > 0)
			{
				previousValid |= (previous.type == TokenType::CLOSE_BRACE);
			}
			bool nextValid = false;
			for (int i = 0; i < ValidNext.size(); i++)
			{
				nextValid |= (next.type == ValidNext[i]);
			}

			if (previousValid && nextValid)
			{
				current.type = TokenType::SEMICOLON;
				current.string = std::string(";");
			}
			else
			{
				current = next;

				for (;;)
				{
					next = scanner.scanToken();
					if (next.type != TokenType::ERROR) break;
					errorAtCurrent(next.string);
				}

				if (previous.type == TokenType::DEF && current.type == TokenType::IDENTIFIER)
					current.type = TokenType::TYPE;

				if (current.type == TokenType::IDENTIFIER && next.type == TokenType::IDENTIFIER)
					current.type = TokenType::TYPE;

				resolveNewlines();
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
			errorAtCurrent(next.string);
		}

		if (previous.type == TokenType::DEF && current.type == TokenType::IDENTIFIER)
			current.type = TokenType::TYPE;

		if (current.type == TokenType::IDENTIFIER && next.type == TokenType::IDENTIFIER)
			current.type = TokenType::TYPE;

		resolveNewlines();
	}

	void Parser::consume(TokenType type, std::string message)
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

	std::shared_ptr<ExpressionNode> Parser::binary(std::shared_ptr<ExpressionNode> lhs, bool canAssign)
	{
		auto node = std::make_shared<BinaryNode>();
		node->left = lhs;
		node->op = previous;
		ParseRule* rule = getRule(previous.type);
		node->right = ParsePrecedence((Precedence)((int)rule->precedence + 1));
		return node;
	}

	std::shared_ptr<ExpressionNode> Parser::assignment(std::shared_ptr<ExpressionNode> lhs, bool canAssign)
	{
		auto node = std::make_shared<AssignmentNode>();
		node->identifier = lhs;
		node->value = expression();
		return node;
	}

	std::shared_ptr<ExpressionNode> Parser::call(std::shared_ptr<ExpressionNode> lhs, bool canAssign)
	{
		if (previous.type == TokenType::PAREN)
		{
			auto node = std::make_shared<FunctionCallNode>();
			node->left = lhs;
			if (!match(TokenType::CLOSE_PAREN))
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
		else if (previous.type == TokenType::DOT)
		{
			auto node = std::make_shared<FieldCallNode>();
			node->left = lhs;
			if (check(TokenType::IDENTIFIER))
			{
				node->field = current;
			}
			consume(TokenType::IDENTIFIER, "expected type field after '.'");

			return node;
		}
		else
		{
			error("expected '(' or '.' for function or field call.");
		}
	}

	std::shared_ptr<ExpressionNode> Parser::arrayIndex(std::shared_ptr<ExpressionNode> lhs, bool canAssign)
	{
		auto node = std::make_shared<ArrayIndexNode>();
		node->left = lhs;
		if(!check(TokenType::CLOSE_BRACKET))
		node->index = expression();
		consume(TokenType::CLOSE_BRACKET, "expected ']' after array index.");
		return node;
	}

	std::shared_ptr<ExpressionNode> Parser::grouping(bool canAssign)
	{
		auto node = expression();
		consume(TokenType::CLOSE_PAREN, "Expected ')' after expresion.");
		return node;
	}

	std::shared_ptr<ExpressionNode> Parser::literal(bool canAssign)
	{
		auto node = std::make_shared<CallNode>();
		node->primary = previous;
		return node;
	}

	std::shared_ptr<ExpressionNode> Parser::unary(bool canAssign)
	{
		auto node = std::make_shared<UnaryNode>();
		node->op = previous;

		node->unary = ParsePrecedence(Precedence::UNARY);
		if(node->op.type == TokenType::MINUS)
		{
		if (node->unary->expressionType() == ExpressionNode::ExpressionType::Primary
			&& (((CallNode*)node->unary.get())->primary.type == TokenType::INT)
			|| (((CallNode*)node->unary.get())->primary.type == TokenType::FLOAT)
			|| (((CallNode*)node->unary.get())->primary.type == TokenType::DOUBLE))
			{
				auto primaryNode = std::dynamic_pointer_cast<CallNode>(node->unary);
				primaryNode->primary.string = std::string("-").append(primaryNode->primary.string);
				return primaryNode;
			}
		}
		else if (node->op.type == TokenType::NOT)
		{
			if (node->unary->expressionType() == ExpressionNode::ExpressionType::Primary)
			{
				auto primaryNode = std::dynamic_pointer_cast<CallNode>(node->unary);
				if (((CallNode*)node->unary.get())->primary.type == TokenType::TRUE)
				{
					primaryNode->primary.type = TokenType::FALSE;
					primaryNode->primary.string = std::string("false");
				}
				else if (((CallNode*)node->unary.get())->primary.type == TokenType::FALSE)
				{
					primaryNode->primary.type = TokenType::TRUE;
					primaryNode->primary.string = std::string("true");
				}

				return primaryNode;
			}
		}
		return node;
	}

	std::shared_ptr<ExpressionNode> Parser::constructor(bool canAssign)
	{
		auto node = std::make_shared<ConstructorNode>();
		if (!match(TokenType::CLOSE_BRACE))
		{
			while (!check(TokenType::CLOSE_BRACE))
			{
				node->arguments.push_back(expression());
				match(TokenType::SEMICOLON); //removes unwanted semicolons because of extra newlines in badly-formatted code
				if (!check(TokenType::CLOSE_BRACE))
				{
					consume(TokenType::COMMA, "expected ',' between arguments.");
				}
			}
			consume(TokenType::CLOSE_BRACE, "expected '}' after final argument.");
		}

		return node;
	}

	void Parser::synchronize()
	{
		panicMode = false;

		while (next.type != TokenType::EOF_)
		{
			resolveNewlines();
			if (current.type == TokenType::SEMICOLON) return;
			switch (current.type)
			{
			case TokenType::TYPE:
			case TokenType::DEF:
			case TokenType::FOR:
			case TokenType::IF:
			case TokenType::WHILE:
			case TokenType::RETURN:
				return;
			default:;
			}

			advance();
		}
	}

	std::shared_ptr<DeclarationNode> Parser::declaration()
	{
		
		if (match(TokenType::DEF))
		{
			std::shared_ptr<TypeDeclarationNode> node = std::make_shared<TypeDeclarationNode>();
			consume(TokenType::TYPE, "expected type after 'def.'");
			node->typeDefined = previous;
			consume(TokenType::BRACE, "expected '{' before type definition.");

			std::vector<parameter> fields;
			while (check(TokenType::TYPE) || check(TokenType::IDENTIFIER) || check(TokenType::SEMICOLON))
			{
				parameter param;
				consume(TokenType::TYPE, "Expected type before identifier.");
				param.type = previous;
				consume(TokenType::IDENTIFIER, "Expected identifier after type.");
				param.identifier = previous;
				fields.push_back(param);
				if (!match(TokenType::SEMICOLON)) break;
			}
			consume(TokenType::CLOSE_BRACE, "expected '}' after type definition.");
			node->fields = fields;
			return node;
		}
		//todo: modules
		else if (match(TokenType::TYPE))
		{
			bool usign = false;
			Token type = previous;
			Token identifier;
			std::shared_ptr<ExpressionNode> arraySize;
			if (previous.string.compare("unsigned") == 0)
			{
				usign = true;
				consume(TokenType::TYPE, "expected type after 'unsigned.'");
				type = previous;
			}
			
			consume(TokenType::IDENTIFIER, "expected identifier after type.");
			identifier = previous;
			if (match(TokenType::BRACKET))
			{
				arraySize = expression();
				consume(TokenType::CLOSE_BRACKET, "expected ']' after array size expression.");
			}
			{
				if (match(TokenType::PAREN))
				{
					std::shared_ptr<FunctionDeclarationNode> node = std::make_shared<FunctionDeclarationNode>();
					node->usign = usign;
					node->type = type;
					node->identifier = identifier;
					node->parameters = getParameters();
					consume(TokenType::CLOSE_PAREN, "expected ')' after function parameters.");
					consume(TokenType::BRACE, "expected '{' before block.");
					node->body = block();

					return node;
				}
				else 
				{
					std::shared_ptr<VariableDeclarationNode> node = std::make_shared<VariableDeclarationNode>();
					node->usign = usign;
					node->type = type;
					node->identifier = identifier;
					node->arraySize = arraySize;
					if (match(TokenType::EQUAL))
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
			std::shared_ptr<StatementNode> stmt = statement();
			return stmt;
		}
	}

	std::shared_ptr<StatementNode> Parser::statement()
	{
		if (match(TokenType::FOR))
		{
			consume(TokenType::PAREN, "expected '(' after 'for.'");
			std::shared_ptr<ForStatementNode> node = std::make_shared<ForStatementNode>();
			if (match(TokenType::SEMICOLON))
			{
				node->declaration = nullptr;
			}
			else if (match(TokenType::TYPE))
			{
				bool usign = false;
				Token type = previous;
				Token identifier;
				if (previous.string.compare("unsigned") == 0)
				{
					usign = true;
					consume(TokenType::TYPE, "expected type after 'unsigned.'");
					type = previous;
				}
				consume(TokenType::IDENTIFIER, "expected identifier after type.");
				identifier = previous;
				std::shared_ptr<ExpressionNode> arraySize;
				if (match(TokenType::BRACKET))
				{
					arraySize = expression();
					consume(TokenType::CLOSE_BRACKET, "expected ']' after array size expression.");
				}
				std::shared_ptr<VariableDeclarationNode> declaration = std::make_shared<VariableDeclarationNode>();
				declaration->usign = usign;
				declaration->type = type;
				declaration->identifier = identifier;
				declaration->arraySize = arraySize;
				if (match(TokenType::EQUAL))
				{
					declaration->value = expression();
				}
				node->declaration = declaration;

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
			std::shared_ptr<IfStatementNode> node = std::make_shared<IfStatementNode>();
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
			std::shared_ptr<ReturnStatementNode> node = std::make_shared<ReturnStatementNode>();
			node->returnValue = expression();
			consume(TokenType::SEMICOLON, "expected ';' after expression.");
			return node;
		}
		else if (match(TokenType::WHILE))
		{
			std::shared_ptr<WhileStatementNode> node = std::make_shared<WhileStatementNode>();
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
			std::shared_ptr<ExpressionStatement> node = std::make_shared<ExpressionStatement>();
			node->expression = expression();
			consume(TokenType::SEMICOLON, "expected ';' after expression");
			return node;
		}
	}

	std::shared_ptr<ExpressionNode> Parser::expression()
	{
		inExpression++;
		return ParsePrecedence(Precedence::ASSIGNMENT);
		inExpression--;
	}

	std::shared_ptr<ExpressionNode> Parser::ParsePrecedence(Precedence precedence)
	{
		advance();
		std::function<std::shared_ptr<ExpressionNode>(bool)> prefixRule = getRule(previous.type)->prefix;
		if (prefixRule == nullptr)
		{
			error("expected expression.");
			return nullptr;
		}

		bool canAssign = precedence <= Precedence::ASSIGNMENT;
		std::shared_ptr<ExpressionNode> node = prefixRule(canAssign);

		while (precedence <= getRule(current.type)->precedence)
		{
			advance();
			std::function<std::shared_ptr<ExpressionNode>(std::shared_ptr<ExpressionNode>, bool)> infixRule = getRule(previous.type)->infix;
			node = infixRule(node, canAssign);
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

	std::shared_ptr<BlockNode> Parser::block()
	{
		std::shared_ptr<BlockNode> node = std::make_shared<BlockNode>();
		while (!check(TokenType::CLOSE_BRACE) && !check(TokenType::EOF_))
		{
			node->declarations.push_back(declaration());
		}
		consume(TokenType::CLOSE_BRACE, "Expected '}' after block.");
		consume(TokenType::SEMICOLON, "this should never display, something has gone terribly wrong!");

		return node;
	}

	std::shared_ptr<ProgramNode> Parser::parse()
	{
		std::shared_ptr<ProgramNode> node = std::make_shared<ProgramNode>();

		/*if (match(TokenType::LIBRARY))
		std::shared_ptr<LibraryNode> library;
		consume(TokenType::IDENTIFIER, "Expected a library name.");
		library.libraryIdentifier = previous;
		node.library = library;
		*/
		
		while (!match(TokenType::EOF_))
		{
			node->declarations.push_back(declaration());

			if (panicMode) synchronize();
		}

		return node;
	}
}