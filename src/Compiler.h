#include "Parser.h"
#include "Chunk.h"
#include "Memory.h"
#include <vector>
#include <unordered_set>

namespace ash
{

	enum class Asm
	{
		Label,
		Jump,
		pseudocode,
		OneAddr,
		TwoAddr,
		ThreeAddr
	};

	struct assembly
	{
		virtual Asm type() = 0;
		virtual void print() = 0;
	};

	struct label : public assembly
	{
		size_t label;
		virtual Asm type() override { return Asm::Label; }
		virtual void print() override
		{
			std::cout << label << ":" << std::endl;
		}
	};

	struct pseudocode : public assembly
	{
		OpCodes op;
		virtual Asm type() override { return Asm::pseudocode; }
		virtual void print() override
		{
			std::cout << "    " << OpcodeNames[op] << std::endl;
		}
	};

	struct relativeJump : public pseudocode
	{
		size_t jumpLabel;
		virtual Asm type() override { return Asm::Jump; }
		virtual void print() override
		{
			std::cout << "    " << OpcodeNames[op] << " " << jumpLabel << std::endl;
		}
	};

	struct oneAddress : public pseudocode
	{
		Token A;
		virtual Asm type() override { return Asm::OneAddr; }
		virtual void print() override
		{
			std::cout << "    " << OpcodeNames[op] << " " << A.string << std::endl;
		}
	};
	struct twoAddress : public pseudocode
	{
		Token result;
		Token A;
		virtual Asm type() override { return Asm::TwoAddr; }
		virtual void print() override
		{
			std::cout << "    " << OpcodeNames[op] << " " << A.string << " " << result.string << std::endl;
		}
	};

	struct threeAddress : public pseudocode
	{
		Token result;
		Token A;
		Token B;
		virtual Asm type() override { return Asm::ThreeAddr; }
		virtual void print() override
		{
			std::cout << "    " << OpcodeNames[op] << " " << A.string << " " << B.string << " " << result.string << std::endl;
		}
	};

	struct pseudochunk
	{
		std::vector<std::shared_ptr<assembly>> code;
	};
	
	struct controlFlowNode
	{
		std::vector<std::shared_ptr<assembly>> block;
		std::shared_ptr<controlFlowNode> trueBlock;
		std::shared_ptr<controlFlowNode> falseBlock = nullptr;
		bool traversed = false;
	};

	struct controlFlowGraph
	{
		std::vector<std::shared_ptr<controlFlowNode>> procedures;
	};

	struct Local
	{
		Token name;
		int depth;

		Local(Token name, int depth)
			:name(name), depth(depth) {}
	};

	class Compiler
	{
		std::vector<Local> locals;
		int scopeDepth;
		std::shared_ptr<ScopeNode> currentScope;
		std::vector<std::string> typeNames;
		std::unordered_map<std::string, size_t> typeIDs;
		std::vector<std::shared_ptr<TypeMetadata>> types;
		Chunk* currentChunk = nullptr;
		size_t temporaries = 0;
		size_t jumpLabels = 0;
	public:
		Compiler()
			:scopeDepth(0) {}

		bool compile(const char* source);

		pseudochunk precompile(std::shared_ptr<ProgramNode> ast);
		pseudochunk optimize(pseudochunk chunk) {};
		controlFlowGraph analyzeControlFlow(pseudochunk chunk);
		pseudochunk allocateRegisters(pseudochunk chunk);
		std::vector<std::unordered_set<std::string>> liveVariables(std::shared_ptr<controlFlowNode> block);

		std::vector<std::shared_ptr<assembly>> compileNode(ParseNode* node, Token* result);
	};

}