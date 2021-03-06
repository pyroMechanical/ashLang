#include "Chunk.h"
#include "Debug.h"
#include "VM.h"

#include <iostream>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#endif

//#define TEST_VM_OPERATIONS

using namespace ash;

    int main()
    {
        VM vm;
#ifdef TEST_VM_OPERATIONS
        Disassembler debug;
        Chunk chunk;
        chunk.WriteI8(1, 13);
        chunk.WriteU8(3, 0x88);
        chunk.WriteU8(4, 1);
        chunk.WriteU16(2, 16);
        chunk.WriteABC(OP_ALLOC_ARRAY, 2, 3, 5, 0);
        chunk.WriteABC(OP_ALLOC_ARRAY, 2, 4, 6, 0);
        chunk.WriteABC(OP_ARRAY_STORE, 6, 5, 1, 0);
        chunk.WriteU8(5, 0xF0);
        chunk.WriteA(OP_OUT, 0, 0);
        chunk.WriteOp(OP_RETURN);

        debug.disassembleChunk(&chunk, "test chunk");
        InterpretResult result = vm.interpret(&chunk);
        system("pause");
#else
        std::cout << "type 'exit' to exit REPL\n";
        std::string line;
        std::string source;
#ifdef _WIN32
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
#endif
        do
        {
            std::getline(std::cin, line);
            if (line == "multi")
            {
                line.clear();
                while (line != "exec" && line != "exit")
                {
                    source.append(line);
                    source.append("\n");
                    std::getline(std::cin, line);
                }
            }
            else
            {
                source = line;
                source.append("\n");
            }
            InterpretResult result = vm.interpret(source);
            source.clear();
           //if(result != InterpretResult::INTERPRET_OK)
           //{
           //    std::cout << "Error!";
           //
           //    return 64;
           //}
        } while (line != "exit");
#endif
        return 0;
    }