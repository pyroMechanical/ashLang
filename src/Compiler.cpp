#include "Compiler.h"
#include <string>

namespace ash
{
	bool Compiler::compile(const char* source)
	{
		Parser parser(source);

		auto ast = parser.parse();

		ast->print(0);

		return false;
	}

}