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

		ast = analyzer.findSymbols(ast);

		ast->print(0);

		pseudochunk result = precompile(ast);

		return false;
	}

	pseudochunk Compiler::precompile(std::shared_ptr<ProgramNode> ast)
	{

	}

}