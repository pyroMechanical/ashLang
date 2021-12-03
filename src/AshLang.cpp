#include "Chunk.h"
#include "Debug.h"
#include "VM.h"

#include <iostream>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#endif

using namespace ash;

   int main()
   {
       VM vm;
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
       return 0;
   }