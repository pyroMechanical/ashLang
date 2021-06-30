#include "Chunk.h"
#include "Debug.h"
#include "VM.h"

#include <iostream>

using namespace ash;

    int main()
    {
        VM vm;
        Chunk chunk;
        chunk.WriteFloat(1, -64.2f);
        chunk.WriteAB(OP_STORE, FLAG_HALF, 1, 0);
        chunk.WriteAB(OP_LOAD, FLAG_HALF, 0, 0);
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