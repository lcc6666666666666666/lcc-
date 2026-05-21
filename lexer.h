#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <iostream>      
#include <string>         
#include <unordered_map>  
#include <vector>         

using namespace std;

// LexType 表示词法分析能够识别出来的所有单词类型。
typedef enum {
	ENDFILE, ERROR,                                 // 文件结束标记、词法错误标记
	PROGRAM, PROCEDURE, TYPE, VAR, IF,             // 保留字：program、procedure、type、var、if
	THEN, ELSE, FI, WHILE, DO, ENDWH,              // 保留字：then、else、fi、while、do、endwh
	BEGIN, END, READ, WRITE, ARRAY, OF,            // 保留字：begin、end、read、write、array、of
	RECORD, RETURN,                                // 保留字：record、return
	INTEGER, CHAR,                                 // 基本类型关键字：integer、char
	ID, INTC, CHARC,                               // 标识符、整数常量、字符常量
	ASSIGN, EQ, LT, PLUS, MINUS,                   // 运算符：:=、=、<、+、-
	TIMES, OVER, LPAREN, RPAREN, DOT,              // 运算符或分隔符：*、/、(、)、.
	COLON, strI, COMMA, LMIDPAREN, RMIDPAREN,      // 分隔符：:、;、,、[、]
	UNDERANGE                                     // 数组下标范围符号：..
} LexType;

// MyToken 表示词法分析输出的一个单词。
class MyToken {
public:
	int line;              // 该 token 在源程序中的行号
	string str;            // token 的原始字符串内容
	LexType lexType;       // token 的内部枚举类型
	string lexTypeString;  // token 类型对应的字符串形式，主要用于输出 token.txt

	// 构造一个 token；默认按标识符 ID 处理，构造函数内部会再判断关键字和符号。
	MyToken(int line, string str, LexType lexType = ID);
};

// 全局变量：这些变量在 lexer.cpp 中定义，在其他编译阶段通过 extern 使用。

extern unordered_map<string, LexType> reversedMap;   // 保留字到 LexType 的映射，例如 "program" -> PROGRAM
extern unordered_map<LexType, string> LexTypeMap;    // LexType 到字符串名称的映射，例如 PROGRAM -> "PROGRAM"
extern unordered_map<string, LexType> sigLexTypeMap; // 符号到 LexType 的映射，例如 ":=" -> ASSIGN
extern vector<MyToken> tokenList;                    // 词法分析得到的 token 序列，后续传给 Parser
extern const char* srcAddr;                          
extern const char* tokenAddr;                        
extern const char* errorAddr;                        
extern const char* treeAddr;                         
extern const char* objAddr;                          

bool lexicalAnalysis(); // 执行真正的词法扫描，读取 source.txt 并填充 tokenList
void printErrorMsg();   // 把词法错误输出到 error.txt
void printTokenList();  // 把 tokenList 输出到 token.txt
void lexicial();        // 词法分析总入口：初始化映射、扫描源文件、输出结果

#endif /* COMPILER_LEXER_H */
