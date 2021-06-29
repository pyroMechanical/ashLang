#include "Chunk.h"
#include "Debug.h"
#include "VM.h"

#include <iostream>

using namespace ash;

    int main()
    {
        VM vm;
        Chunk chunk;
        chunk.WriteI64(1, -5);
        chunk.WriteDouble(3, -25.5);
        chunk.WriteAB(OP_INT_TO_DOUBLE, 1, 1, 0);
        chunk.WriteABC(OP_DOUBLE_DIV, 3, 1, 0, 0);
        chunk.WriteOp(OP_RETURN);
        Disassembler d;
        d.disassembleChunk(&chunk, "test chunk");
        InterpretResult result = vm.interpret(&chunk);
        if(result != InterpretResult::INTERPRET_OK)
        {
            std::cout << "Error!";

            return 64;
        }
        system("pause");
        return 0;
    }