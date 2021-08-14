#include "Parser.h"
#include "Chunk.h"
#include <vector>

namespace ash
{

	struct pseudocode
	{
		OpCodes op;
	};

	struct oneaddress : public pseudocode
	{
		Token A;
	};

	struct twoaddress : public pseudocode
	{
		Token A;
		Token B;
	};
	struct threeaddress : public pseudocode
	{
		Token A;
		Token B;
		Token C;
	};

	struct pseudochunk
	{
		std::vector<pseudocode*> code;
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
	public:
		Compiler()
			:scopeDepth(0) {}

		bool compile(const char* source);

		pseudochunk precompile(std::shared_ptr<ProgramNode> ast);
	};

}