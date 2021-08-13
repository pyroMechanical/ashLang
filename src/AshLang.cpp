#include "Chunk.h"
#include "Debug.h"
#include "VM.h"

#include <iostream>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#endif

#define TEST_VM_OPERATIONS

using namespace ash;

    int main()
    {
        VM vm;
#ifdef TEST_VM_OPERATIONS
        Chunk chunk;
        chunk.WriteU8(0, 16);
        chunk.WriteU8(3, 8);
        chunk.WriteAB(OP_ALLOC, 0, 1, 0);
        chunk.WriteAB(OP_STORE8, 0, 1, 0);
        chunk.WriteABC(OP_STORE8_OFFSET, 0, 1, 3, 0);
        chunk.WriteAB(OP_LOAD8, 2, 1, 0);
        chunk.WriteU8(1, 0);
        chunk.WriteAB(OP_ALLOC, 2, 1, 0);
        chunk.WriteOp(OP_RETURN);
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