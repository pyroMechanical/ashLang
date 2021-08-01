#include "Compiler.h"
#include "Semantics.h"
#include <string>

namespace ash
{
	bool Compiler::compile(const char* source)
	{
		Parser parser(source);

		auto ast = parser.parse();

		Semantics analyzer;

		ast = analyzer.findSymbols(std::move(ast));

		ast->print(0);

		return false;
	}

}