#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

typedef enum {
	ENDFILE, ERROR,
	PROGRAM, PROCEDURE, TYPE, VAR, IF,
	THEN, ELSE, FI, WHILE, DO, ENDWH,
	BEGIN, END, READ, WRITE, ARRAY, OF,
	RECORD, RETURN,
	INTEGER, CHAR,
	ID, INTC, CHARC,
	ASSIGN, EQ, LT, PLUS, MINUS,
	TIMES, OVER, LPAREN, RPAREN, DOT,
	COLON, strI, COMMA, LMIDPAREN, RMIDPAREN,
	UNDERANGE
} LexType;

class MyToken {
public:
	int line;
	string str;
	LexType lexType;
	string lexTypeString;

	MyToken(int line, string str, LexType lexType = ID);
};

// 全局变量

extern unordered_map<string, LexType> reversedMap;
extern unordered_map<LexType, string> LexTypeMap;
extern unordered_map<string, LexType> sigLexTypeMap;
extern vector<MyToken> tokenList;
extern const char* srcAddr;
extern const char* tokenAddr;
extern const char* errorAddr;
extern const char* treeAddr;
extern const char* objAddr;

bool lexicalAnalysis();
void printErrorMsg();
void printTokenList();
void lexicial();

#endif /* COMPILER_LEXER_H */
