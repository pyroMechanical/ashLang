#include "Scanner.h"
#include "Chunk.h"
#include <vector>

namespace ash
{

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
		Chunk* currentChunk;

		Compiler(Chunk* chunk)
			:currentChunk(chunk), scopeDepth(0) {}
	};

}