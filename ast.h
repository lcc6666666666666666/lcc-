#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include "lexer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std;

enum Treenodecate {	//语法树节点类型
	ProK, PheadK, TypeK, VarK, ProcDecK, StmLK, DecK, StmtK, ExpK
};

enum DecKcate {	//语法树节点的声明类型
	ArrayK, CharK, IntegerK, RecordK, IdK, BoolK
};

enum StmtKcate {	//语法树节点的语句类型
	IfK, WhileK, AssignK, ReadK, WriteK, CallK, ReturnK
};

enum ExpKcate {	//语法树节点的表达式类型
	OpK, ConstK, IdEK
};

enum class ParamTcate {	//valparamtype或者varparamtype,表示过程的参数是值参还是变参
	valparamtype, varparamtype
};

enum class Varkind {	//变量的类别
	IdV , ArrayMembV, FieldMembV
};//普通变量

enum class ExpType {	//语法树节点的检查类型
	Void, Integer, Boolean
};

enum class ErrorType {
	MissingProgramHeader, MissingDeclaration, MissingProgramBody, SyntaxError
};

struct SymbolsTable;	//符号表

struct ArrayAttrx {	//数组类型的属性
	int low;		//整数类型变量，记录数组的下界。
	int up;			//整数类型变量，记录数组的上界。
	DecKcate childType;	//记录数组的成员类型。
};

struct ExpAttrx {
	LexType op;		//记录语法树节点的运算符单词，为单词类型
	int val;		//记录语法树节点的数值，当语法树节点为“数字因子"对应的语法树节点时有效,为整数类型
	Varkind varkind;//记录变量的类别
	ExpType type;	//记录语法树节点的检查类型,取值 Void,Integer, Boolean,为类型检查ExpType类型
};

struct Attr {
	ArrayAttrx ArrayAttr;	//记录数组类型的属性
	ParamTcate paramt;		//记录过程的参数类型
	ExpAttrx ExpAttr;		//记录表达式的属性
};

class Treenode;
using TreenodePtr = unique_ptr<Treenode>;
const string NodeKind_str[] = { "ProK","PheadK","TypeK","VarK","ProcDecK","StmLK","DecK","StmtK","ExpK" };
const string Deck_str[] = { "ArrayK","CharK","IntegerK","RecordK","IdK" };
const string StmtK_str[] = { "IfK","WhileK","AssignK","ReadK","WriteK","CallK","ReturnK" };
const string ExpK_str[] = { "OpK","ConstK","IdK" };
const string LexType_str[] = { "endfile","error","program","procedure","type","var","if"
		,"then","else","fi","while","do","endwh","begin","end","read","write","array","of",
							  "record","return","integer","char","id","intc","charc",":=","=","<","+",
							  "-","*","/","(",")",".","colon","stri",",","[","]",".." };

class Treenode {
public:
	TreenodePtr child[3]; //指向子语法树节点指针，为语法树节点指针类型
	TreenodePtr sibling;	//指向兄弟语法树节点指针，为语法树节点指针类型
	int lineno;			//记录源程序行号，为整数类型
	Treenodecate nodekind;//记录语法树节点类型
	union {
		DecKcate dec;	//记录语法树节点的声明类型，当nodekind=DecK时 有效
		StmtKcate stmt;	//记录语法树节点的语句类型，当nodekind=StmtK时有效
		ExpKcate exp;	//记录语法树节点的表达式类型，当nodekind=ExpK时有效
	}kind;				//记录语法树节点的具体类型，为共用体结构
	int idnum = 0;		//记录一个节点中的标志符的个数
	int level = 0;
	std::vector<std::string> name;	//字符串数组。数组成员是节点中的标志符的名字
	std::vector<SymbolsTable*> table;	//指针数组，数组成员是节点中的各个标志符在符号表中的入口
	string type_name;	//记录类型名，当节点为声明类型，且类型是由类型标志符表示时有效
	Attr attr;			//成员attr记录语法树节点其他属性，为结构体类型

	Treenode() = default;

	static TreenodePtr create(Treenodecate kind, int lineno) {
		auto node = make_unique<Treenode>();
		node->nodekind = kind;
		node->lineno = lineno;
		return node;
	}

	void addChild(int index, TreenodePtr children) {
		if (index >= 0 && index < 3) child[index] = move(children);
	}

	void addSibling(TreenodePtr sib) {
		Treenode* current = this;
		while (current->sibling) current = current->sibling.get();
		current->sibling = move(sib);
	}

	void addName(const string& id) {
		idnum++;
		name.push_back(id);
	}
};

#endif /* COMPILER_AST_H */
