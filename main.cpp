#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "midcode.h"
#include "codegen.h"

#include <utility>

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int wCodePageID);
extern "C" __declspec(dllimport) int __stdcall SetConsoleCP(unsigned int wCodePageID);
constexpr unsigned int kUtf8CodePage = 65001;
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(kUtf8CodePage);
    SetConsoleCP(kUtf8CodePage);
#endif

    lexicial();
    Parser parser(std::move(tokenList));
    auto syntaxTree = parser.parse();
    parser.printTree(syntaxTree);

    auto analyzedTree = semanticAnalyze(std::move(syntaxTree));
    generateMidCode(analyzedTree.get());
    codeGen(std::move(analyzedTree));

    return 0;
}
