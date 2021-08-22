#include "Parser.h"
#include "Chunk.h"
#include <vector>

namespace ash
{

	enum class Asm
	{
		Label,
		Jump,
		OneAddr,
		TwoAddr,
		ThreeAddr
	};

	struct assembly
	{
		virtual Asm type() = 0;
	};

	struct label : public assembly
	{
		size_t label;
		virtual Asm type() override { return Asm::Label; }
	};

	struct pseudocode : public assembly
	{
		uint8_t op;
	};

	struct relativeJump : public pseudocode
	{
		size_t jumpLabel;
		virtual Asm type() override { return Asm::Jump; }
	};

	struct oneAddress : public pseudocode
	{
		Token A;
		virtual Asm type() override { return Asm::OneAddr; }
	};
	struct twoAddress : public pseudocode
	{
		Token result;
		Token A;
		virtual Asm type() override { return Asm::TwoAddr; }
	};
	struct threeaddress : public pseudocode
	{
		Token result;
		Token A;
		Token B;
		virtual Asm type() override { return Asm::ThreeAddr; }
	};

	struct pseudochunk
	{
		std::vector<std::shared_ptr<assembly>> code;
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
		Chunk* currentChunk = nullptr;
		size_t temporaries = 0;
		size_t jumpLabels = 0;
	public:
		Compiler()
			:scopeDepth(0) {}

		bool compile(const char* source);

		pseudochunk precompile(std::shared_ptr<ProgramNode> ast);

		std::vector<std::shared_ptr<assembly>> compileNode(ParseNode* node);
	};

}