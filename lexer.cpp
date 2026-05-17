#include "lexer.h"

#include <cstdio>
#include <fstream>
#include <sstream>

unordered_map<string, LexType> reversedMap;
unordered_map<LexType, string> LexTypeMap;
unordered_map<string, LexType> sigLexTypeMap;
vector<MyToken> tokenList;
const char* srcAddr = "source.txt"; // 源代码文件
const char* tokenAddr = "token.txt"; // 词法分析结果文件
const char* errorAddr = "error.txt"; // 错误信息文件
const char* treeAddr = "tree.txt";
const char* objAddr = "mips.txt";

// MyToken构造函数实现
MyToken::MyToken(int line, string str, LexType lexType) {
	this->line = line;
	this->str = str;

	if (reversedMap.find(str) != reversedMap.end()) {
		this->lexType = reversedMap.find(str)->second;
		this->lexTypeString = LexTypeMap.find(this->lexType)->second;
	}
	else if (sigLexTypeMap.find(str) != sigLexTypeMap.end()) {
		this->lexType = sigLexTypeMap.find(str)->second;
		this->lexTypeString = LexTypeMap.find(this->lexType)->second;
	}
	else if (lexType == CHARC || lexType == ID || lexType == ERROR || lexType == INTC || lexType == ENDFILE) {
		this->lexType = lexType;
		this->lexTypeString = LexTypeMap.find(this->lexType)->second;
	}
}

// 初始化函数
static void init() {
	reversedMap.insert({ "program", PROGRAM });
	reversedMap.insert({ "type", TYPE });
	reversedMap.insert({ "var", VAR });
	reversedMap.insert({ "procedure", PROCEDURE });
	reversedMap.insert({ "begin", BEGIN });
	reversedMap.insert({ "end", END });
	reversedMap.insert({ "array", ARRAY });
	reversedMap.insert({ "of", OF });
	reversedMap.insert({ "record", RECORD });
	reversedMap.insert({ "if", IF });
	reversedMap.insert({ "then", THEN });
	reversedMap.insert({ "else", ELSE });
	reversedMap.insert({ "fi", FI });
	reversedMap.insert({ "while", WHILE });
	reversedMap.insert({ "do", DO });
	reversedMap.insert({ "endwh", ENDWH });
	reversedMap.insert({ "read", READ });
	reversedMap.insert({ "write", WRITE });
	reversedMap.insert({ "return", RETURN });
	reversedMap.insert({ "integer", INTEGER });
	reversedMap.insert({ "char", CHAR });

	LexTypeMap.insert({ ENDFILE, "ENDFILE" });
	LexTypeMap.insert({ ERROR, "ERROR" });
	LexTypeMap.insert({ PROGRAM, "PROGRAM" });
	LexTypeMap.insert({ PROCEDURE, "PROCEDURE" });
	LexTypeMap.insert({ TYPE, "TYPE" });
	LexTypeMap.insert({ VAR, "VAR" });
	LexTypeMap.insert({ IF, "IF" });
	LexTypeMap.insert({ THEN, "THEN" });
	LexTypeMap.insert({ ELSE, "ELSE" });
	LexTypeMap.insert({ FI, "FI" });
	LexTypeMap.insert({ WHILE, "WHILE" });
	LexTypeMap.insert({ DO, "DO" });
	LexTypeMap.insert({ ENDWH, "ENDWH" });
	LexTypeMap.insert({ BEGIN, "BEGIN" });
	LexTypeMap.insert({ END, "END" });
	LexTypeMap.insert({ READ, "READ" });
	LexTypeMap.insert({ WRITE, "WRITE" });
	LexTypeMap.insert({ ARRAY, "ARRAY" });
	LexTypeMap.insert({ OF, "OF" });
	LexTypeMap.insert({ RECORD, "RECORD" });
	LexTypeMap.insert({ RETURN, "RETURN" });
	LexTypeMap.insert({ INTEGER, "INTEGER" });
	LexTypeMap.insert({ CHAR, "CHAR" });
	LexTypeMap.insert({ ID, "ID" });
	LexTypeMap.insert({ INTC, "INTC" });
	LexTypeMap.insert({ CHARC, "CHARC" });
	LexTypeMap.insert({ ASSIGN, "ASSIGN" });
	LexTypeMap.insert({ EQ, "EQ" });
	LexTypeMap.insert({ LT, "LT" });
	LexTypeMap.insert({ PLUS, "PLUS" });
	LexTypeMap.insert({ MINUS, "MINUS" });
	LexTypeMap.insert({ TIMES, "TIMES" });
	LexTypeMap.insert({ OVER, "OVER" });
	LexTypeMap.insert({ LPAREN, "LPAREN" });
	LexTypeMap.insert({ RPAREN, "RPAREN" });
	LexTypeMap.insert({ DOT, "DOT" });
	LexTypeMap.insert({ COLON, "COLON" });
	LexTypeMap.insert({ strI, "strI" });
	LexTypeMap.insert({ COMMA, "COMMA" });
	LexTypeMap.insert({ LMIDPAREN, "LMIDPAREN" });
	LexTypeMap.insert({ RMIDPAREN, "RMIDPAREN" });
	LexTypeMap.insert({ UNDERANGE, "UNDERANGE" });

	sigLexTypeMap.insert({ "(", LPAREN });
	sigLexTypeMap.insert({ ")", RPAREN });
	sigLexTypeMap.insert({ "[", LMIDPAREN });
	sigLexTypeMap.insert({ "]", RMIDPAREN });
	sigLexTypeMap.insert({ ";", strI });
	sigLexTypeMap.insert({ ",", COMMA });
	sigLexTypeMap.insert({ "+", PLUS });
	sigLexTypeMap.insert({ "-", MINUS });
	sigLexTypeMap.insert({ "*", TIMES });
	sigLexTypeMap.insert({ "/", OVER });
	sigLexTypeMap.insert({ "<", LT });
	sigLexTypeMap.insert({ ".", DOT });
	sigLexTypeMap.insert({ "..", UNDERANGE });
	sigLexTypeMap.insert({ ":=", ASSIGN });
	sigLexTypeMap.insert({ "=", EQ });
}

// 判断函数
static bool isDigit(char ch) {
	return ch >= '0' && ch <= '9';
}

static bool isLetter(char ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static bool isBlank(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

static bool isOperator(char ch) {
	return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '<' || ch == '=';
}

static bool isSeparator(char ch) {
	return ch == ';' || ch == ',' || ch == '{' || ch == '}' || ch == '[' ||
		ch == ']' || ch == '(' || ch == ')' || ch == '.' || ch == '\'' || ch == ':';
}

// 词法分析主函数
bool lexicalAnalysis() {
	int line = 1;
	bool error = false;
	FILE* fp = fopen(srcAddr, "r");
	if (fp == nullptr) {
		cout << "打开源文件失败" << endl;
		return true;
	}

	char ch = fgetc(fp);
	string word = "";

	while (ch != EOF) {
		if (isLetter(ch)) {
			word = "";
			word += ch;
			ch = fgetc(fp);

			while (isLetter(ch) || isDigit(ch)) {
				word += ch;
				ch = fgetc(fp);
			}

			MyToken token(line, word);
			tokenList.push_back(token);
		}
		else if (isDigit(ch)) {
			word = "";
			word += ch;
			ch = fgetc(fp);

			while (isDigit(ch) || isLetter(ch)) {
				word += ch;
				ch = fgetc(fp);
			}

			bool flag = true;
			for (auto c : word) {
				if (isLetter(c)) {
					flag = false;
					error = true;
					MyToken token(line, "数字开头的单词！", ERROR);
					tokenList.push_back(token);
					break;
				}
			}

			if (flag) {
				MyToken token(line, word, INTC);
				tokenList.push_back(token);
			}
		}
		else if (isOperator(ch)) {
			string tmp = "";
			tmp += ch;
			MyToken token(line, tmp);
			tokenList.push_back(token);
			ch = fgetc(fp);
		}
		else if (isSeparator(ch)) {
			word = "";

			if (ch == '{') {
				while (ch != '}') {
					ch = fgetc(fp);
					if (ch == '\n') line++;
				}
				ch = fgetc(fp);
			}
			else if (ch == '.') {
				word += ch;
				if ((ch = fgetc(fp)) == '.') {
					word += ch;
					MyToken token(line, word);
					tokenList.push_back(token);
					ch = fgetc(fp);
				}
				else {
					MyToken token(line, word);
					tokenList.push_back(token);
				}
			}
			else if (ch == '\'') {
				string tmp = "";
				while ((ch = fgetc(fp)) != '\'') {
					tmp += ch;
				}
				MyToken token(line, tmp, CHARC);
				tokenList.push_back(token);
				ch = fgetc(fp);
			}
			else if (ch == ':') {
				word += ch;
				if ((ch = fgetc(fp)) == '=') {
					word += ch;
					MyToken token(line, word);
					tokenList.push_back(token);
					ch = fgetc(fp);
				}
				else {
					error = true;
					MyToken token(line, "=出现错误", ERROR);
					tokenList.push_back(token);
					ch = fgetc(fp);
				}
			}
			else {
				string tmp = "";
				tmp += ch;
				MyToken token(line, tmp);
				tokenList.push_back(token);
				ch = fgetc(fp);
			}
		}
		else if (isBlank(ch)) {
			if (ch == '\n') {
				line++;
			}
			ch = fgetc(fp);
		}
		else {
			error = true;
			MyToken token(line, "未知字符", ERROR);
			tokenList.push_back(token);
			ch = fgetc(fp);
		}
	}

	MyToken token(++line, "ENDFILE", ENDFILE);
	tokenList.push_back(token);
	fclose(fp);
	return error;
}

// 打印错误信息
void printErrorMsg() {
	ofstream output(errorAddr);
	for (MyToken t : tokenList) {
		if (t.lexType == ERROR) {
			cout << t.line << "行" << '\t' << t.str << endl;
			output << t.line << "行" << '\t' << t.str << '\n';
		}
	}
	output.close();
}

// 打印Token列表
void printTokenList() {
	ofstream output(tokenAddr);
	for (MyToken t : tokenList) {
		cout << t.line << '\t' << t.lexTypeString << '\t' << t.str << endl;
		output << t.line << '\t' << t.lexTypeString << '\t' << t.str << '\n';
	}
	output.close();
}

// 词法分析主流程
void lexicial() {
	init();
	if (lexicalAnalysis()) {
		printErrorMsg();
		cout << "词法分析发现错误，程序即将退出!" << endl;
	}
	else {
		printTokenList();
		cout << "词法分析完成，结果已输出到文件!" << endl;
	}
}
