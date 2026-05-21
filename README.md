# SNL Compiler

这是一个 SNL 语言编译器课程设计项目，编译流程已经按阶段拆分为独立模块：

```text
source.txt
  -> 词法分析 lexer
  -> 语法分析 parser
  -> 语义分析 semantic
  -> 目标代码生成 codegen
  -> mips.txt
```

## 项目结构

```text
lexer.h / lexer.cpp       词法分析、Token 定义、token.txt/error.txt 输出
ast.h                     语法树节点和共享 AST 类型
parser.h / parser.cpp     递归下降语法分析、tree.txt 输出
semantic.h / semantic.cpp 符号表、类型信息、语义检查
codegen.h / codegen.cpp   MIPS 目标代码生成、mips.txt 输出
main.cpp                  编译流程入口

source.txt                源程序输入
token.txt                 词法分析输出
tree.txt                  语法树输出
mips.txt                  目标代码输出
error.txt                 错误信息输出
```

## 构建

```bash
g++ -std=c++17 -g main.cpp lexer.cpp parser.cpp semantic.cpp midcode.cpp codegen.cpp -o compiler
```

VS Code 的默认构建任务也已经改为编译上述多个源文件，并输出 `compiler.exe`。

## 运行

```bash
./compiler
```

程序会读取 `source.txt`，并生成 `token.txt`、`tree.txt`、`mips.txt` 和 `error.txt`。
