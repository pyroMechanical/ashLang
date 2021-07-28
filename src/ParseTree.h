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

		static std::string tokenstring(Token token)
		{
			std::string out = std::string(token.start);
			out.resize(token.length);
			return out;
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
				std::cout << "[ " << util::categoryToString(symbol.second.cat) << " " << util::tokenstring(symbol.second.type) << " " << symbol.first;
				if (symbol.second.cat == category::Function)
				{
					auto& params = functionParameters.at(symbol.first);
					std::cout << "(";
					bool first = true;
					for (const auto& param : params)
					{
						if (!first) std::cout << ", ";
						std::cout << util::tokenstring(param.type) << " " << util::tokenstring(param.identifier);
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
						std::cout << util::tokenstring(param.type) << " " << util::tokenstring(param.identifier);
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
			std::cout << "Identifier: " << util::tokenstring(libraryIdentifier);
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
				std::cout << util::tokenstring(token);
				repeat = true;
			}
			std::cout << " From: " << util::tokenstring(fromIdentifier) << std::endl;
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
			std::cout << "Type: " << util::tokenstring(typeDefined) << std::endl;
			util::spaces(depth);
			std::cout << "Fields: ";
			bool repeat = false;
			for (const auto& parameter : fields)
			{
				if(repeat) std::cout << ", ";
				std::cout << util::tokenstring(parameter.type) << " " << util::tokenstring(parameter.identifier);
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
			FieldCall,
			Primary,
		};

		virtual NodeType nodeType() override { return NodeType::Expression; }
		virtual ExpressionType expressionType() = 0;

		virtual void print(int depth) override {};
	};

	struct StatementNode : public DeclarationNode
	{
		virtual NodeType nodeType() override { return NodeType::Statement; }

		virtual void print(int depth) override {};
	};

	struct ExpressionStatement : public StatementNode
	{
		std::unique_ptr<ExpressionNode> expression;

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
		std::unique_ptr<ParseNode> declaration; //can only be VariableDeclarationNode or ExpressionNode
		std::unique_ptr<ExpressionNode> conditional;
		std::unique_ptr<ExpressionNode> increment;
		std::unique_ptr<StatementNode> statement;

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
		std::unique_ptr<ExpressionNode> condition;
		std::unique_ptr<StatementNode> thenStatement;
		std::unique_ptr<StatementNode> elseStatement;

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
		std::unique_ptr<ExpressionNode> returnValue;

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
		std::unique_ptr<ExpressionNode> condition;
		std::unique_ptr<StatementNode> doStatement;

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
		std::vector<std::unique_ptr<DeclarationNode>> declarations;

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
		std::unique_ptr<BlockNode> body;

		virtual NodeType nodeType() override { return NodeType::FunctionDeclaration; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Function Declaration" << std::endl;
			util::spaces(depth);
			std::cout << "Function: " << util::tokenstring(type) << " " << util::tokenstring(identifier) << std::endl;
			util::spaces(depth);
			std::cout << "Parameters: ";
			bool repeat = false;
			for (const auto& parameter : parameters)
			{
				if (repeat) std::cout << ", ";
				std::cout << util::tokenstring(parameter.type) << " " << util::tokenstring(parameter.identifier);
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
		std::unique_ptr<ExpressionNode> value;

		virtual NodeType nodeType() override { return NodeType::VariableDeclaration; }

		virtual VariableDeclarationNode& operator= (const VariableDeclarationNode&) = default;

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Variable Declaration" << std::endl;
			util::spaces(depth);
			std::cout << "Variable: " << util::tokenstring(type) << " " << util::tokenstring(identifier) << std::endl;
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
		//std::unique_ptr<LibraryNode> library;
		//std::vector<std::unique_ptr<ImportNode>> imports;
		std::vector<std::unique_ptr<DeclarationNode>> declarations;

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
	};



	struct CallNode : public ExpressionNode
	{
		Token primary;

		virtual ExpressionType expressionType() override { return ExpressionType::Primary; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Literal: " << util::tokenstring(primary) << std::endl;
		}
	};

	struct FieldCallNode : public ExpressionNode
	{
		std::unique_ptr<ExpressionNode> left;
		Token field;

		virtual ExpressionType expressionType() override { return ExpressionType::FieldCall; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Field Call" << std::endl;
			util::spaces(depth);
			std::cout << "Called: " << std::endl;
			left->print(depth + 1);
			util::spaces(depth);
			std::cout << "Field: " << util::tokenstring(field) << std::endl;
		}
	};

	struct BinaryNode : public ExpressionNode
	{
		std::unique_ptr<ExpressionNode> left;
		Token op;
		std::unique_ptr<ExpressionNode> right;
		virtual ExpressionType expressionType() override { return ExpressionType::Binary; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Binary Expression" << std::endl;
			left->print(depth + 1);
			util::spaces(depth);
			std::cout << "Operator: " << util::tokenstring(op) << std::endl;
			right->print(depth + 1);
		}
	};


	struct AssignmentNode : public ExpressionNode
	{
		std::unique_ptr<ExpressionNode> identifier;
		std::unique_ptr<ExpressionNode> value;
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

		std::string resolveIdentifier()
		{
			switch (identifier->expressionType())
			{
				case ExpressionNode::ExpressionType::Primary:
				{
					CallNode* node = (CallNode*)identifier.get();
					return util::tokenstring(node->primary);
				}
				case ExpressionNode::ExpressionType::FieldCall:
				{
					FieldCallNode* node = (FieldCallNode*)identifier.get();
					std::string result = "." + util::tokenstring(node->field);
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
							result = "." + util::tokenstring(node->field) + result;
						}
						else if (node->left->expressionType() == ExpressionNode::ExpressionType::Primary)
						{
							CallNode* primaryNode = (CallNode*)node->left.get();
							result = util::tokenstring(primaryNode->primary) + result;
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
		std::unique_ptr<ExpressionNode> unary; //Unary or Call

		virtual ExpressionType expressionType() override { return ExpressionType::Unary; }

		virtual void print(int depth) override
		{
			util::spaces(depth);
			std::cout << "Unary Expression" << std::endl;
			util::spaces(depth);
			std::cout << "Operator: " << util::tokenstring(op);
			unary->print(depth + 1);
		}
	};

	struct FunctionCallNode : public ExpressionNode
	{
		std::unique_ptr<ExpressionNode> left;
		std::vector<std::unique_ptr<ExpressionNode>> arguments;

		virtual ExpressionType expressionType() override { return ExpressionType::FunctionCall; }
		
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
	};

}