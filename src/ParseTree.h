#pragma once
#include "Scanner.h"

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>



namespace ash
{

	enum class category
	{
		Module, Function, Variable, Type
	};

	namespace util
	{
		static void spaces(int amount)
		{
			std::cout << std::string(amount*2, ' ');
		}

		static std::string categoryToString(category cat)
		{
			switch (cat)
			{
				case category::Module: return "Module";
				case category::Function: return "Function";
				case category::Variable: return "Variable";
				case category::Type: return "Type";
			}
			std::cout << "Category not recognized!";
			return "";
		}
	}



	struct Symbol
	{
		std::string name;
		category cat;
		Token type;
	};

	enum class NodeType
	{
		Program,
		Library,
		Import,
		Declaration,
		TypeDeclaration,
		FunctionDeclaration,
		VariableDeclaration,
		Statement,
		ForStatement,
		IfStatement,
		ReturnStatement,
		WhileStatement,
		Block,
		ExpressionStatement,
		Expression,

		//Scope nodes

		Scope
	};

	struct BlockNode;
	struct ExpressionNode;
	struct StatementNode;

	struct parameter
	{
		Token type;
		Token identifier;
	};

	struct ParseNode
	{
		virtual NodeType nodeType() = 0;

		ParseNode() = default;

		virtual ~ParseNode() = default;

		virtual void print(int depth) = 0;

		ParseNode& operator= (ParseNode&&) = default;
	};

	struct ScopeNode : public ParseNode
	{
		std::shared_ptr<ScopeNode> parentScope;
		std::unordered_map<std::string, Symbol> symbols;
		std::unordered_map<std::string, std::vector<parameter>> functionParameters;
		std::unordered_map<std::string, std::vector<parameter>> typeParameters;
		std::unordered_map<std::string, std::vector<size_t>> sortedType;
		size_t scopeIndex = 0;
		virtual NodeType nodeType() override { return NodeType::Scope; }

		virtual void print(int depth) override
		{
			int nextDepth = depth - 1;
			if (nextDepth < 0) nextDepth = 0;

			if (parentScope)
				parentScope->print(nextDepth);

			for (const auto& symbol : symbols)
			{
				util::spaces(depth);
				std::cout << "[ " << util::categoryToString(symbol.second.cat) << " " << symbol.second.type.string << " " << symbol.first;
				if (symbol.second.cat == category::Function)
				{
					auto& params = functionParameters.at(symbol.first);
					std::cout << "(";
					bool first = true;
					for (const auto& param : params)
					{
						if (!first) std::cout << ", ";
						std::cout << param.type.string << " " << param.identifier.string;
						first = false;
					}
					std::cout << ")";
				}
				else if (symbol.second.cat == category::Type)
				{
					auto& params = typeParameters.at(symbol.first);
					std::cout << "{";
					bool first = true;
					for (const auto& param : params)
					{
						if (!first) std::cout << ", ";
						std::cout << param.type.string << " " << param.identifier.string;
						first = false;
					}
					std::cout << "}";
				}
				std::cout << " ]" << std::endl;
			}
		}
	};

	struct LibraryNode : public ParseNode
	{
		Token libraryIdentifier;

		virtual NodeType nodeType() override { return NodeType::Library; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Library" << std::endl;
			util::spaces(depth);
			std::cout << "Identifier: " << libraryIdentifier.string;
		}
	};

	struct ImportNode : public ParseNode
	{
		std::vector<Token> usingList;
		Token fromIdentifier;

		virtual NodeType nodeType() override { return NodeType::Import; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Import" << std::endl;
			util::spaces(depth);
			std::cout << "Using: ";
			bool repeat = false;
			for (const auto& token : usingList)
			{
				if (repeat) std::cout << ", ";
				std::cout << token.string;
				repeat = true;
			}
			std::cout << " From: " << fromIdentifier.string << std::endl;
		}
	};

	struct DeclarationNode : public ParseNode
	{
		virtual NodeType nodeType() override { return NodeType::Declaration; }
		virtual void print(int depth) override {};
	};

	struct TypeDeclarationNode : public DeclarationNode
	{
		Token typeDefined;
		std::vector<parameter> fields;

		virtual NodeType nodeType() override { return NodeType::TypeDeclaration; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Type Declaration" << std::endl;
			util::spaces(depth);
			std::cout << "Type: " << typeDefined.string << std::endl;
			util::spaces(depth);
			std::cout << "Fields: ";
			bool repeat = false;
			for (const auto& parameter : fields)
			{
				if(repeat) std::cout << ", ";
				std::cout << parameter.type.string << " " << parameter.identifier.string;
				repeat = true;
			}
			std::cout << std::endl;
		}
	};

	struct ExpressionNode : public ParseNode
	{
		enum class ExpressionType
		{
			Assignment,
			Binary,
			Unary,
			FunctionCall,
			Constructor,
			FieldCall,
			ArrayIndex,
			Primary,
		};

		virtual NodeType nodeType() override { return NodeType::Expression; }
		virtual ExpressionType expressionType() = 0;

		virtual Token typeToken() = 0;

		virtual int line() = 0;

		virtual void print(int depth) override {};
	};

	struct StatementNode : public DeclarationNode
	{
		virtual NodeType nodeType() override { return NodeType::Statement; }

		virtual void print(int depth) override {};
	};

	struct ExpressionStatement : public StatementNode
	{
		std::shared_ptr<ExpressionNode> expression;

		virtual NodeType nodeType() override { return NodeType::ExpressionStatement; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Expression Statement" << std::endl;
			util::spaces(depth);
			std::cout << "Expression: " << std::endl;
			if (expression != nullptr)
				expression->print(depth + 1);
		}
	};

	struct ForStatementNode : public StatementNode
	{
		std::shared_ptr<ParseNode> declaration; //can only be VariableDeclarationNode or ExpressionNode
		std::shared_ptr<ExpressionNode> conditional;
		std::shared_ptr<ExpressionNode> increment;
		std::shared_ptr<StatementNode> statement;
		std::shared_ptr<ScopeNode> forScope;

		virtual NodeType nodeType() override { return NodeType::ForStatement; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "For Statement" << std::endl;

			if (declaration != nullptr)
			{
				util::spaces(depth);
				std::cout << "Setup: " << std::endl;
				declaration->print(depth + 1);
			}
			if (conditional != nullptr)
			{
				util::spaces(depth);
				std::cout << "Condition: " << std::endl;
				conditional->print(depth + 1);
			}
			if (increment != nullptr)
			{
				util::spaces(depth);
				std::cout << "Increment: " << std::endl;
				increment->print(depth + 1);
			}
			if (statement != nullptr)
			{
				util::spaces(depth);
				std::cout << "Statement: " << std::endl;
				statement->print(depth + 1);
			}
		}
	};

	struct IfStatementNode : public StatementNode
	{
		std::shared_ptr<ExpressionNode> condition;
		std::shared_ptr<StatementNode> thenStatement;
		std::shared_ptr<StatementNode> elseStatement;
		std::shared_ptr<ScopeNode> ifScope;

		virtual NodeType nodeType() override { return NodeType::IfStatement; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "If Statement" << std::endl;
			if (condition)
			{
				util::spaces(depth);
				std::cout << "Condition: " << std::endl;
				condition->print(depth + 1);
			}
			if (thenStatement)
			{
				util::spaces(depth);
				std::cout << "Then: " << std::endl;
				thenStatement->print(depth + 1);
			}
			if (elseStatement)
			{
				util::spaces(depth);
				std::cout << "Else: " << std::endl;
				elseStatement->print(depth + 1);
			}
		}
	};

	struct ReturnStatementNode : public StatementNode
	{
		std::shared_ptr<ExpressionNode> returnValue;

		virtual NodeType nodeType() override { return NodeType::ReturnStatement; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Return Statement" << std::endl;
			if (returnValue)
			{
				util::spaces(depth);
				std::cout << "Value: " << std::endl;
				returnValue->print(depth + 1);
			}
		}
	};

	struct WhileStatementNode : public StatementNode
	{
		std::shared_ptr<ExpressionNode> condition;
		std::shared_ptr<StatementNode> doStatement;
		std::shared_ptr<ScopeNode> whileScope;

		virtual NodeType nodeType() override { return NodeType::WhileStatement; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "While Statement" << std::endl;
			if (condition)
			{
				util::spaces(depth);
				std::cout << "Condition: " << std::endl;
				condition->print(depth + 1);
			}
			if (doStatement)
			{
				util::spaces(depth);
				std::cout << "Do: " << std::endl;
				doStatement->print(depth + 1);
			}
		}
	};

	struct BlockNode : public StatementNode
	{
		std::vector<std::shared_ptr<DeclarationNode>> declarations;

		std::shared_ptr<ScopeNode> scope;

		virtual NodeType nodeType() override { return NodeType::Block; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Block" << std::endl;
			for (const auto& declaration : declarations)
			{
				declaration->print(depth + 1);
			}

			if(scope)
				scope->print(depth + 1);
		}
	};

	struct FunctionDeclarationNode : public DeclarationNode
	{
		bool usign = false;
		Token type;
		Token identifier;
		std::vector<parameter> parameters;
		std::shared_ptr<BlockNode> body;

		virtual NodeType nodeType() override { return NodeType::FunctionDeclaration; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Function Declaration" << std::endl;
			util::spaces(depth);
			std::cout << "Function: " << type.string << " " << identifier.string << std::endl;
			util::spaces(depth);
			std::cout << "Parameters: ";
			bool repeat = false;
			for (const auto& parameter : parameters)
			{
				if (repeat) std::cout << ", ";
				std::cout << parameter.type.string << " " << parameter.identifier.string;
				repeat = true;
			}
			std::cout << std::endl;
			util::spaces(depth);
			std::cout << "Body: " << std::endl;
			body->print(depth + 1);
		}
	};

	struct VariableDeclarationNode : public DeclarationNode
	{
		bool usign = false;
		Token type;
		Token identifier;
		std::shared_ptr<ExpressionNode> arraySize;
		std::shared_ptr<ExpressionNode> value;

		virtual NodeType nodeType() override { return NodeType::VariableDeclaration; }

		virtual VariableDeclarationNode& operator= (const VariableDeclarationNode&) = default;

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Variable Declaration" << std::endl;
			util::spaces(depth);
			std::cout << "Variable: " << type.string << " " << identifier.string << std::endl;
			if(arraySize)
			{
				util::spaces(depth);
				std::cout << "Array Size: " << std::endl;
				arraySize->print(depth + 1);
			}
			if (value)
			{
				util::spaces(depth);
				std::cout << "Value: " << std::endl;
				value->print(depth + 1);
			}
		}
	};

	struct ProgramNode : public ParseNode
	{
		//std::shared_ptr<LibraryNode> library;
		//std::vector<std::shared_ptr<ImportNode>> imports;
		std::vector<std::shared_ptr<DeclarationNode>> declarations;

		std::shared_ptr<ScopeNode> globalScope;

		bool hadError = false;

		ProgramNode() = default;

		virtual NodeType nodeType() override { return NodeType::Program; }

		virtual void print(int depth) override
		{
			/*
			if(library)
				library->print(0);
			for (const auto& _import : imports)
			{
				_import->print(0);
			}*/

			for (const auto& declaration : declarations)
			{
				declaration->print(0);
			}
			if (globalScope)
				globalScope->print(0);
		}

		std::shared_ptr<FunctionDeclarationNode> convertToFunctionNode()
		{
			std::shared_ptr<FunctionDeclarationNode> result = std::make_shared<FunctionDeclarationNode>();
			result->type = Token{ TokenType::TYPE, "void", 0 };
			result->identifier = Token{ TokenType::IDENTIFIER, "<program>", 0 };
			result->usign = false;
			std::shared_ptr<BlockNode> programBody = std::make_shared<BlockNode>();
			programBody->declarations = declarations;
			programBody->scope = globalScope;
			result->body = programBody;
			return result;
		}
	};

	struct CallNode : public ExpressionNode
	{
		Token primary;
		Token primaryType;

		virtual Token typeToken() override { return primaryType; }

		virtual ExpressionType expressionType() override { return ExpressionType::Primary; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Literal: " << primary.string << std::endl;
		}

		virtual int line() override { return primary.line; }
	};

	struct FieldCallNode : public ExpressionNode
	{
		std::shared_ptr<ExpressionNode> left;
		Token field;
		Token fieldType;

		virtual Token typeToken() override { return fieldType; }

		virtual ExpressionType expressionType() override { return ExpressionType::FieldCall; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Field Call" << std::endl;
			util::spaces(depth);
			std::cout << "Called: " << std::endl;
			left->print(depth + 1);
			util::spaces(depth);
			std::cout << "Field: " << field.string << std::endl;
		}

		virtual int line() override { return left->line(); }
	};

	struct ArrayIndexNode : public ExpressionNode
	{
		std::shared_ptr<ExpressionNode> left;
		std::shared_ptr<ExpressionNode> index;
		Token arrayType;

		virtual Token typeToken() override { return arrayType; }

		virtual ExpressionType expressionType() override { return ExpressionType::ArrayIndex; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Array Index" << std::endl;
			util::spaces(depth);
			std::cout << "Array: " << std::endl;
			left->print(depth + 1);
			util::spaces(depth);
			if (index)
			{
				std::cout << "Index: " << std::endl;
				index->print(depth + 1);
			}
		}

		virtual int line() override { return left->line(); }
	};

	struct BinaryNode : public ExpressionNode
	{
		std::shared_ptr<ExpressionNode> left;
		Token leftType;
		Token op;
		std::shared_ptr<ExpressionNode> right;
		Token rightType;

		Token binaryType;
		virtual ExpressionType expressionType() override { return ExpressionType::Binary; }

		virtual Token typeToken() override { return binaryType; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Binary Expression" << std::endl;
			left->print(depth + 1);
			util::spaces(depth);
			std::cout << "Operator: " << op.string << std::endl;
			right->print(depth + 1);
		}

		virtual int line() override { return left->line(); }
	};

	struct AssignmentNode : public ExpressionNode
	{
		std::shared_ptr<ExpressionNode> identifier;
		Token assignmentType;
		std::shared_ptr<ExpressionNode> value;

		virtual Token typeToken() override { return assignmentType; }

		virtual ExpressionType expressionType() override { return ExpressionType::Assignment; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Assignment Expression" << std::endl;
			if (identifier)
			{
				util::spaces(depth);
				std::cout << "Identifier: " << std::endl;
				identifier->print(depth + 1);
			}
			if (value)
			{
				util::spaces(depth);
				std::cout << "Value: " << std::endl;
				value->print(depth + 1);
			}
		}

		virtual int line() override { return identifier->line(); }

		std::string resolveIdentifier()
		{
			switch (identifier->expressionType())
			{
				case ExpressionNode::ExpressionType::Primary:
				{
					CallNode* node = (CallNode*)identifier.get();
					return node->primary.string;
				}
				case ExpressionNode::ExpressionType::FieldCall:
				{
					FieldCallNode* node = (FieldCallNode*)identifier.get();
					std::string result = "." + node->field.string;
					bool complete = false;
					while (!complete)
					{
						if (node->left->expressionType() != ExpressionNode::ExpressionType::FieldCall
							&& node->left->expressionType() != ExpressionNode::ExpressionType::Primary)
						{
							return "";
						}
						else if (node->left->expressionType() == ExpressionNode::ExpressionType::FieldCall)
						{
							node = (FieldCallNode*)node->left.get();
							result = "." + node->field.string + result;
						}
						else if (node->left->expressionType() == ExpressionNode::ExpressionType::Primary)
						{
							CallNode* primaryNode = (CallNode*)node->left.get();
							result = primaryNode->primary.string + result;
							complete = true;
						}
					}

					return result;
				}
				default:
				{
					return "";
				}
			}
		}
	};

	struct UnaryNode : public ExpressionNode
	{
		Token op;
		std::shared_ptr<ExpressionNode> unary; //Unary or Call
		Token unaryType;

		virtual Token typeToken() override { return unaryType; }

		virtual ExpressionType expressionType() override { return ExpressionType::Unary; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Unary Expression" << std::endl;
			util::spaces(depth);
			std::cout << "Operator: " << op.string << std::endl;
			unary->print(depth + 1);
		}

		virtual int line() override { return op.line; }
	};

	struct FunctionCallNode : public ExpressionNode
	{
		std::shared_ptr<ExpressionNode> left;
		std::vector<std::shared_ptr<ExpressionNode>> arguments;
		Token functionType;

		virtual Token typeToken() override { return functionType; }

		virtual ExpressionType expressionType() override { return ExpressionType::FunctionCall; }

		std::string resolveName()
		{
			ExpressionNode* node = left.get();
			std::string result;
			while (node != nullptr)
			{
				switch (left->expressionType())
				{
					case ExpressionNode::ExpressionType::Primary:
					{
						CallNode* callNode = (CallNode*)node;

						result += callNode->primary.string;
						node = nullptr;
						break;
					}
					case ExpressionNode::ExpressionType::FieldCall:
					{
						FieldCallNode* fieldCallNode = (FieldCallNode*)node;

						result += fieldCallNode->field.string;
						result += ".";
						node = fieldCallNode->left.get();
						break;
					}
				}
			}

			return result;
		}
		
		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Function Call" << std::endl;
			util::spaces(depth);
			std::cout << "Called: " << std::endl;
			left->print(depth + 1);
			util::spaces(depth);
			std::cout << "Arguments: " << std::endl;
			for (const auto& args : arguments)
			{
				args->print(depth + 1);
			}
		}

		virtual int line() override { return left->line(); }
	};

	struct ConstructorNode : public ExpressionNode
	{
		std::vector<std::shared_ptr<ExpressionNode>> arguments;
		Token ConstructorType;
		int constructorLine = 0;

		virtual Token typeToken() override { return ConstructorType; }

		virtual ExpressionType expressionType() override { return ExpressionType::Constructor; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Constructor Expression" << std::endl;
			util::spaces(depth);
			std::cout << "Arguments: " << std::endl;
			for (const auto& expression : arguments)
			{
				expression->print(depth + 1);
			}
		}

		virtual int line() override { return constructorLine; }
	};
}