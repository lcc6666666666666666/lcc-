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
	// line 记录当前扫描到的源程序行号，error 记录本次扫描过程中是否出现词法错误。
	int line = 1;
	bool error = false;

	// 打开源文件，后面会从这个文件中一个字符一个字符地读取。
	FILE* fp = fopen(srcAddr, "r");
	if (fp == nullptr) {
		cout << "打开源文件失败" << endl;
		return true;
	}

	// ch 表示当前正在处理的字符，word 用来临时拼接一个完整的单词。
	char ch = fgetc(fp);
	string word = "";

	// 核心扫描循环：只要没有读到文件结束符 EOF，就持续分析当前字符。
	while (ch != EOF) {
		// 以字母开头：可能是标识符，也可能是保留字。
		// 例如 abc 会被识别成 ID，program 会在 MyToken 构造函数里被识别成 PROGRAM。
		if (isLetter(ch)) {
			word = "";
			word += ch;
			ch = fgetc(fp);

			// 标识符/保留字后面可以继续跟字母或数字，所以一直读到不满足条件为止。
			while (isLetter(ch) || isDigit(ch)) {
				word += ch;
				ch = fgetc(fp);
			}

			MyToken token(line, word);
			tokenList.push_back(token);
		}
		// 以数字开头：正常情况是整数常量；如果后面混入字母，则认为是错误单词。
		else if (isDigit(ch)) {
			word = "";
			word += ch;
			ch = fgetc(fp);

			// 这里先把连续的数字/字母全部读出来，后面再判断是否包含字母。
			while (isDigit(ch) || isLetter(ch)) {
				word += ch;
				ch = fgetc(fp);
			}

			bool flag = true;
			for (auto c : word) {
				if (isLetter(c)) {
					flag = false;
					error = true;
					// 例如 123abc 这种“数字开头又含有字母”的串不是合法整数。
					MyToken token(line, "数字开头的单词！", ERROR);
					tokenList.push_back(token);
					break;
				}
			}

			if (flag) {
				// 没有字母，说明是合法整数常量 INTC。
				MyToken token(line, word, INTC);
				tokenList.push_back(token);
			}
		}
		// 运算符：这里处理的是单字符运算符，如 +、-、*、/、<、=。
		else if (isOperator(ch)) {
			string tmp = "";
			tmp += ch;
			MyToken token(line, tmp);
			tokenList.push_back(token);
			ch = fgetc(fp);
		}
		// 分隔符或特殊符号：包括括号、分号、逗号、点、单引号、冒号等。
		else if (isSeparator(ch)) {
			word = "";

			if (ch == '{') {
				// 花括号里的内容当作注释，直接跳过；如果注释里换行，需要更新行号。
				while (ch != '}') {
					ch = fgetc(fp);
					if (ch == '\n') line++;
				}
				ch = fgetc(fp);
			}
			else if (ch == '.') {
				// 点号可能是单独的 "."，也可能和下一个点组成范围符号 ".."。
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
				// 单引号包起来的是字符常量，例如 'a'，这里只把中间的内容作为 CHARC。
				string tmp = "";
				while ((ch = fgetc(fp)) != '\'') {
					tmp += ch;
				}
				MyToken token(line, tmp, CHARC);
				tokenList.push_back(token);
				ch = fgetc(fp);
			}
			else if (ch == ':') {
				// 冒号必须和等号组成赋值符号 ":="，单独的 ":" 在这里认为是错误。
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
				// 其他普通分隔符直接生成对应 token，如 ;、,、(、)、[、]。
				string tmp = "";
				tmp += ch;
				MyToken token(line, tmp);
				tokenList.push_back(token);
				ch = fgetc(fp);
			}
		}
		// 空白字符不生成 token；换行需要增加 line，方便后续报错定位。
		else if (isBlank(ch)) {
			if (ch == '\n') {
				line++;
			}
			ch = fgetc(fp);
		}
		else {
			// 以上规则都无法识别的字符，统一记为未知字符错误。
			error = true;
			MyToken token(line, "未知字符", ERROR);
			tokenList.push_back(token);
			ch = fgetc(fp);
		}
	}

	// 文件读完后，手动补一个 ENDFILE token，告诉后续语法分析“输入结束了”。
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
