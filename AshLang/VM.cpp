#include "VM.h"


#define INSTRUCTIONMASK = 0x00FFFFFF;
namespace ash
{
	VM::VM()
	{
		
	}

	VM::~VM()
	{
		
	}

	InterpretResult VM::interpret(std::string source)
	{
		//ip = 
		InterpretResult result = run();
	}

	InterpretResult VM::run()
	{
		return INTERPRET_OK;
	}
}