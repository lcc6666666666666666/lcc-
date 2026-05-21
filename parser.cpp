#include "parser.h"

#include <fstream>
#include <iostream>
#include <stack>

// 获取当前正在分析的 Token。
MyToken& Parser::currentToken() { return tokens[currentIndex]; }

// 前进到下一个 Token；保留最后一个 Token，避免越界访问。
void Parser::consumeToken() { if (currentIndex < tokens.size() - 1) currentIndex++; }

// 统一记录语法错误，并将错误信息输出到控制台。
void Parser::logError(ErrorType type, const string& details) {
		errorLog << "Line " << currentToken().line << ": ";
		switch (type) {
		case ErrorType::MissingProgramHeader:
			errorLog << "Missing program header"; cout<<errorLog.str()<<endl; break;
		case ErrorType::MissingDeclaration:
			errorLog << "Missing declaration"; cout<<errorLog.str()<<endl; break;
		case ErrorType::MissingProgramBody:
			errorLog << "Missing program body"; cout<<errorLog.str()<<endl; break;
		default:
			errorLog << "Syntax error"; cout<<errorLog.str()<<endl; break;
		}
		if (!details.empty()) {errorLog << " (" << details << ")"; cout<<errorLog.str()<<endl;}
		errorLog << endl;
	}

// 构造语法分析器，保存词法分析阶段生成的 Token 序列。
Parser::Parser(vector<MyToken> tokenList) : tokens(move(tokenList)) {}

// 语法分析入口：从 program 非终结符开始，返回整棵语法树。
TreenodePtr Parser::parse() {
		return program();
	}

// 将词法类型转换成可读字符串，主要用于错误信息。
std::string Parser::lexTypeToString(LexType type) {
		switch (type) {
		case LexType::PROGRAM:    return "PROGRAM";
		case LexType::PROCEDURE:  return "PROCEDURE";
		case LexType::TYPE:       return "TYPE";
		case LexType::VAR:        return "VAR";
		case LexType::IF:         return "IF";
		case LexType::THEN:       return "THEN";
		case LexType::ELSE:       return "ELSE";
		case LexType::FI:         return "FI";
		case LexType::WHILE:      return "WHILE";
		case LexType::DO:         return "DO";
		case LexType::ENDWH:      return "ENDWH";
		case LexType::BEGIN:      return "BEGIN";
		case LexType::END:        return "END";
		case LexType::READ:       return "READ";
		case LexType::WRITE:      return "WRITE";
		case LexType::ARRAY:      return "ARRAY";
		case LexType::OF:         return "OF";
		case LexType::RECORD:     return "RECORD";
		case LexType::RETURN:     return "RETURN";
		case LexType::INTEGER:    return "INTEGER";
		case LexType::CHAR:       return "CHAR";
		case LexType::ID:         return "IDENTIFIER";
		case LexType::INTC:       return "INTEGER_CONST";
		case LexType::CHARC:      return "CHAR_CONST";
		case LexType::ASSIGN:    return ":=";
		case LexType::EQ:         return "=";
		case LexType::LT:         return "<";
		case LexType::PLUS:       return "+";
		case LexType::MINUS:      return "-";
		case LexType::TIMES:      return "*";
		case LexType::OVER:       return "/";
		case LexType::LPAREN:     return "(";
		case LexType::RPAREN:     return ")";
		case LexType::DOT:       return ".";
		case LexType::COLON:      return ":";
		case LexType::strI:       return ";";
		case LexType::COMMA:      return ",";
		case LexType::LMIDPAREN:  return "[";
		case LexType::RMIDPAREN:  return "]";
		case LexType::UNDERANGE:  return "..";
		case LexType::ENDFILE:   return "END_OF_FILE";
		default:                  return "UNKNOWN_TOKEN";
		}
	}

// 以深度优先方式遍历语法树，并同时输出到文件和控制台。
void Parser::printTree(const TreenodePtr& root) {
		std::ofstream outTree(treeAddr);
		if (!outTree.is_open()) {
			std::cerr << "Error: Cannot open output file " << treeAddr << std::endl;
			return;
		}

		std::stack<Treenode*> nodeStack;
		root->level = 0;
		nodeStack.push(root.get());

		while (!nodeStack.empty()) {
			Treenode* current = nodeStack.top();
			nodeStack.pop();

			// 打印缩进
			for (int i = 0; i < current->level; ++i) {
				outTree << "  │  ";
				std::cout << "  │  ";
			}

			// 打印节点基本信息
			outTree << NodeKind_str[current->nodekind] << " ";
			std::cout << NodeKind_str[current->nodekind] << " ";

			// 处理不同节点类型
			switch (current->nodekind) {
			case Treenodecate::DecK:
				outTree << Deck_str[current->kind.dec] << " ";
				std::cout << Deck_str[current->kind.dec] << " ";
				if (current->kind.dec == DecKcate::ArrayK) {
					outTree << "[" << current->attr.ArrayAttr.low << ".."
						<< current->attr.ArrayAttr.up << "] ";
					std::cout << "[" << current->attr.ArrayAttr.low << ".."
						<< current->attr.ArrayAttr.up << "] ";
				}
				break;

			case Treenodecate::StmtK:
				outTree << StmtK_str[current->kind.stmt] << " ";
				std::cout << StmtK_str[current->kind.stmt] << " ";
				break;

			case Treenodecate::ExpK:
				outTree << ExpK_str[current->kind.exp] << " ";
				std::cout << ExpK_str[current->kind.exp] << " ";
				if (current->kind.exp == ExpKcate::OpK) {
					outTree << LexType_str[current->attr.ExpAttr.op] << " ";
					std::cout << LexType_str[current->attr.ExpAttr.op] << " ";
				}
				else if (current->kind.exp == ExpKcate::ConstK) {
					outTree << current->attr.ExpAttr.val << " ";
					std::cout << current->attr.ExpAttr.val << " ";
				}
				break;
			}

			// 打印标识符
			for (const auto& name : current->name) {
				outTree << name << " ";
				std::cout << name << " ";
			}

			// 打印层级
			outTree << "(Level: " << current->level << ")\n";
			std::cout << "(Level: " << current->level << ")\n";

			// 兄弟节点入栈（相同层级）
			if (current->sibling) {
				current->sibling->level = current->level;
				nodeStack.push(current->sibling.get());
			}

			// 子节点逆序入栈（保证正序处理）
			for (int i = 2; i >= 0; --i) {
				if (current->child[i]) {
					current->child[i]->level = current->level + 1;
					nodeStack.push(current->child[i].get());
				}
			}
		}

		outTree.close();
	}

// 解析完整程序：程序头、声明部分、程序体，最后匹配结束点号。
TreenodePtr Parser::program() {
		auto root = Treenode::create(Treenodecate::ProK, currentToken().line);

		TreenodePtr head = programHead();
		if (!head) logError(ErrorType::MissingProgramHeader);
		root->addChild(0, move(head));

		TreenodePtr decls = declarePart();
		if (!decls) logError(ErrorType::MissingDeclaration);
		root->addChild(1, move(decls));

		TreenodePtr body = programBody();
		if (!body) logError(ErrorType::MissingProgramBody);
		root->addChild(2, move(body));

		match(DOT);
		return root;
	}

// 解析程序头：PROGRAM 后跟程序名标识符。
TreenodePtr Parser::programHead() {
		auto head = Treenode::create(Treenodecate::PheadK, currentToken().line);
		match(PROGRAM);
		if (currentToken().lexType == ID) {
			head->addName(currentToken().str);
		}
		match(ID);
		return head;
	}

// 解析声明部分，并按类型声明、变量声明、过程声明的顺序组织兄弟节点。
TreenodePtr Parser::declarePart() {
		TreenodePtr typeP = nullptr, varP = nullptr;

		// 类型声明部分 TypeDecpart
		int line = currentToken().line;
		TreenodePtr tp1 = typeDecpart();
		if (tp1) {
			typeP = Treenode::create(Treenodecate::TypeK, line);
			typeP->addChild(0, move(tp1)); // TypeK 节点的第一个子节点是类型定义
		}

		// 变量声明部分 VarDecpart
		line = currentToken().line;
		TreenodePtr tp2 = varDecpart();
		if (tp2) {
			varP = Treenode::create(Treenodecate::VarK, line);
			varP->addChild(0, move(tp2)); // VarK 节点的第一个子节点是变量定义
		}

		// 过程声明部分 ProcDecpart
		TreenodePtr s = procDecpart();

		// 组合兄弟节点链
		if (!varP) {
			if (!typeP) {
				typeP = move(s); // 只有过程声明
			}
			else {
				typeP->addSibling(move(s)); // TypeK -> ProcDecK
			}
		}
		else {
			varP->addSibling(move(s));      // VarK -> ProcDecK
			if (!typeP) {
				typeP = move(varP);         // 只有变量和过程声明
			}
			else {
				typeP->addSibling(move(varP)); // TypeK -> VarK -> ProcDecK
			}
		}

		return typeP;
	}

// 解析可选的类型声明部分；如果遇到 VAR/PROCEDURE/BEGIN，说明类型声明为空。
TreenodePtr Parser::typeDecpart() {
		TreenodePtr t = nullptr;

		if (currentToken().lexType == TYPE) {
			// 匹配 TYPE 声明
			t = typeDec(); // 假设 typeDec() 返回 TreenodePtr
		}
		else if (currentToken().lexType == VAR ||
			currentToken().lexType == PROCEDURE ||
			currentToken().lexType == BEGIN) {
			// 允许无类型声明，直接返回空指针
		}
		else {
			// 非法 Token 报错
			logError(ErrorType::SyntaxError,
				"Unexpected token " + lexTypeToString(currentToken().lexType) + " in type declaration");
		}

		return t;
	}

// 匹配 TYPE 关键字并解析类型声明列表。
TreenodePtr Parser::typeDec() {
		match(TYPE);
		TreenodePtr t = typeDecList();
		if (!t) {
			logError(ErrorType::MissingDeclaration, "Empty type declarations after TYPE");
		}
		return t;
	}

// 解析一条类型声明：类型名 = 类型定义 ;。
TreenodePtr Parser::typeDecList() {
		auto t = Treenode::create(Treenodecate::DecK, currentToken().line);
		typeId(t);
		match(EQ);
		typeDef(t);
		match(strI);//分号
		TreenodePtr p = typeDecMore();
		if (p)t->addSibling(move(p));
		return t;
	}

// 解析后续类型声明；多个声明通过 sibling 链接。
TreenodePtr Parser::typeDecMore() {
		TreenodePtr t = nullptr;

		if (currentToken().lexType == ID) {
			// 递归解析后续类型声明
			t = typeDecList();
		}
		else if (currentToken().lexType == VAR ||
			currentToken().lexType == PROCEDURE ||
			currentToken().lexType == BEGIN) {
			// 允许无更多声明，不报错
		}
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token " + lexTypeToString(currentToken().lexType) + " after type declaration");
		}

		return t;
	}

// 解析类型标识符，并记录到当前声明节点的 name 列表中。
void Parser::typeId(TreenodePtr& t) {
		if (currentToken().lexType == ID && t != nullptr)t->addName(currentToken().str);
		match(ID);
	}

// 解析类型定义，支持基本类型、数组/记录结构类型和已有类型名。
void Parser::typeDef(TreenodePtr& t) {
		if (!t) {
			logError(ErrorType::SyntaxError, "TypeDef called with null node");
			return;
		}

		// 处理基础类型 (INTEGER/CHAR)
		if (currentToken().lexType == LexType::INTEGER || currentToken().lexType == LexType::CHAR) {
			baseType(t);
		}
		// 处理结构类型 (ARRAY/RECORD)
		else if (currentToken().lexType == LexType::ARRAY || currentToken().lexType == LexType::RECORD) {
			structureType(t);
		}
		// 处理用户定义类型 (ID)
		else if (currentToken().lexType == LexType::ID) {
			t->kind.dec = DecKcate::IdK;  // 设置声明类型为标识符
			t->addName(currentToken().str);
			t->type_name = currentToken().str;  // 记录类型名
			match(LexType::ID);
		}
		// 非法 Token 类型
		else {
			logError(ErrorType::SyntaxError,
				"Invalid type definition token: " + lexTypeToString(currentToken().lexType));
		}
	}

// 解析基本类型 INTEGER 或 CHAR，并设置声明节点的具体类型。
void Parser::baseType(TreenodePtr& t) {
		if (currentToken().lexType == LexType::INTEGER) {
			t->kind.dec = DecKcate::IntegerK;  // 假设枚举值正确映射
			match(LexType::INTEGER);
		}
		else if (currentToken().lexType == LexType::CHAR) {
			t->kind.dec = DecKcate::CharK;     // 假设枚举值正确映射
			match(LexType::CHAR);
		}
	}

// 解析结构类型入口，根据当前 Token 分派到数组或记录类型。
void Parser::structureType(TreenodePtr& t) {
		if (currentToken().lexType == LexType::ARRAY) {
			arrayType(t); 
		}
		else if (currentToken().lexType == LexType::RECORD) {
			t->kind.dec = DecKcate::RecordK;  // 先设置类型再处理子结构
			recType(t);   
		}
	}

// 解析数组类型：ARRAY [下界..上界] OF 基本类型。
void Parser::arrayType(TreenodePtr& t) {
		match(LexType::ARRAY);
		match(LexType::LMIDPAREN);
		if (currentToken().lexType == LexType::INTC) {
			t->attr.ArrayAttr.low = stoi(currentToken().str);
		}
		match(INTC);
		match(UNDERANGE);
		if (currentToken().lexType == LexType::INTC) {
			t->attr.ArrayAttr.up = stoi(currentToken().str);
		}
		match(INTC);
		match(RMIDPAREN);
		match(OF);
		baseType(t);
		t->attr.ArrayAttr.childType = t->kind.dec; // 保存元素类型
		t->kind.dec = DecKcate::ArrayK;
	}

// 解析记录类型：RECORD 字段声明列表 END。
void Parser::recType(TreenodePtr& t) {
		match(LexType::RECORD);
		TreenodePtr p = fieldDecList();
		if (p) {
			t->addChild(0, move(p));
		}
		else {
			logError(ErrorType::MissingDeclaration, "Empty field list in RECORD");
		}
		match(LexType::END);
	}

// 解析记录中的字段声明列表，字段可以是基本类型或数组类型。
TreenodePtr Parser::fieldDecList() {
		auto t = Treenode::create(Treenodecate::DecK, currentToken().line);
		TreenodePtr p = nullptr;
		if (currentToken().lexType == LexType::INTEGER ||
			currentToken().lexType == LexType::CHAR) {

			baseType(t);     // 设置类型为 IntegerK/CharK
			idList(t);       // 解析标识符列表

			match(LexType::strI);
			p = fieldDecMore();  // 解析后续字段声明

		}
		else if (currentToken().lexType == LexType::ARRAY) {

			arrayType(t);    // 设置类型为 ArrayK 并解析数组属性
			idList(t);       // 解析标识符列表
			match(LexType::strI);
			p = fieldDecMore();
		}
		else {
			logError(ErrorType::SyntaxError,
				"Invalid field type: " + lexTypeToString(currentToken().lexType));
		}
		// 连接兄弟节点
		if (p) {
			t->addSibling(move(p));
		}
		return t;
	}

// 解析更多字段声明；遇到 END 表示记录字段声明结束。
TreenodePtr Parser::fieldDecMore() {
		TreenodePtr t = nullptr;

		if (currentToken().lexType != LexType::END) {
			// 检查是否还有字段声明
			if (currentToken().lexType == LexType::INTEGER ||
				currentToken().lexType == LexType::CHAR ||
				currentToken().lexType == LexType::ARRAY) {

				t = fieldDecList();  // 递归解析字段声明
			}
			else {
				logError(ErrorType::SyntaxError,
					"Unexpected token in field declaration: " +
					lexTypeToString(currentToken().lexType));
			}
		}

		return t;
	}

// 解析普通标识符列表，用于记录字段声明等位置。
void Parser::idList(TreenodePtr& t) {
		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);  // 使用 addName 方法
		}
		match(LexType::ID);
		idMore(t);  // 处理后续标识符
	}

// 解析逗号分隔的后续标识符。
void Parser::idMore(TreenodePtr& t) {
		if (currentToken().lexType == LexType::strI) {
			return;  // 分号由上层函数处理
		}
		// 逗号分隔的更多标识符
		else if (currentToken().lexType == COMMA) {
			match(COMMA);
			idList(t);
		}
		// 非法 Token 类型
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token after identifier: " +
				lexTypeToString(currentToken().lexType));
		}
	}

// 解析可选的变量声明部分；遇到 PROCEDURE/BEGIN 表示变量声明为空。
TreenodePtr Parser::varDecpart() {
		TreenodePtr t = nullptr;

		if (currentToken().lexType == LexType::PROCEDURE ||
			currentToken().lexType == LexType::BEGIN) {
			// 允许无变量声明，不处理
		}
		else if (currentToken().lexType == LexType::VAR) {  // 尝试匹配 VAR
			t = varDec();  // 解析变量声明
		}
		else {
			logError(ErrorType::MissingDeclaration,
				"Expected VAR/PROCEDURE/BEGIN but got " +
				lexTypeToString(currentToken().lexType));
		}

		return t;
	}

// 匹配 VAR 关键字并解析变量声明列表。
TreenodePtr Parser::varDec() {
		match(LexType::VAR);
		TreenodePtr t = varDecList();
		if (!t)logError(ErrorType::MissingDeclaration, "Empty variable declarations after VAR");
		return t;
	}

// 解析一条变量声明：类型定义 变量名列表 ;。
TreenodePtr Parser::varDecList() {
		auto t = Treenode::create(Treenodecate::DecK, currentToken().line);
		TreenodePtr p = nullptr;
		typeDef(t);
		varIdList(t);
		match(strI);
		p = varDecMore();
		t->addSibling(move(p));
		return t;
	}

// 解析后续变量声明；多个变量声明通过 sibling 链接。
TreenodePtr Parser::varDecMore() {
		TreenodePtr t = nullptr;

		if (currentToken().lexType == LexType::PROCEDURE ||
			currentToken().lexType == LexType::BEGIN) {
			// 允许无更多声明
		}
		// 检查是否为新变量声明的起始 Token
		else if (currentToken().lexType == LexType::INTEGER ||
			currentToken().lexType == LexType::CHAR ||
			currentToken().lexType == LexType::ARRAY ||
			currentToken().lexType == LexType::RECORD ||
			currentToken().lexType == LexType::ID) {
			t = varDecList(); // 递归解析变量声明
		}
		// 非法 Token 类型
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token in variable declaration: " +
				lexTypeToString(currentToken().lexType));
		}

		return t;
	}

// 解析变量名列表的第一个标识符。
void Parser::varIdList(TreenodePtr& t) {
		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);  // 自动管理 idnum 和 name
			match(LexType::ID);
		}
		else {
			logError(ErrorType::SyntaxError,
				"Missing identifier in variable declaration");
		}
		varIdMore(t);  // 处理后续标识符
	}

// 解析逗号分隔的后续变量名。
void Parser::varIdMore(TreenodePtr& t) {
		if (currentToken().lexType == LexType::strI) {
			return;
		}
		// 逗号分隔更多标识符
		else if (currentToken().lexType == LexType::COMMA) {
			match(LexType::COMMA);
			varIdList(t);
		}
		// 非法 Token 类型
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token in variable list: " +
				lexTypeToString(currentToken().lexType));
		}
	}

// 解析可选的过程声明部分；遇到 BEGIN 表示没有更多过程声明。
TreenodePtr Parser::procDecpart() {
		TreenodePtr t = nullptr;

		if (currentToken().lexType == LexType::BEGIN) {
			// 允许无过程声明，直接返回空
		}
		else if (currentToken().lexType == LexType::PROCEDURE) {  // 尝试匹配 PROCEDURE
			t = procDec();  // 解析过程声明
		}
		else {
			logError(ErrorType::MissingDeclaration,
				"Expected PROCEDURE/BEGIN but got " +
				lexTypeToString(currentToken().lexType));
		}

		return t;
	}

// 解析过程声明，包括过程名、形参、局部声明和过程体。
TreenodePtr Parser::procDec() {
		auto t = Treenode::create(Treenodecate::ProcDecK, currentToken().line);
		match(LexType::PROCEDURE);
		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);  // 存储过程名
			match(LexType::ID);
		}
		match(LexType::LPAREN);//左括号
		paramList(t);    //参数列表，解析括号里的参数，并把参数声明挂到 t 的第 0 个子节点
		match(LexType::RPAREN);//右括号
		match(LexType::strI);
		t->addChild(1, procDecPart());  // 第二个子节点
		// 解析过程体 (BEGIN-END 块)
		t->addChild(2, procBody());     // 第三个子节点
		t->addSibling(move(procDecpart()));
		return t;
	}

// 解析过程形参列表，并挂接为过程声明节点的第 0 个子节点。
void Parser::paramList(TreenodePtr& t) {
		TreenodePtr p = nullptr;

		if (currentToken().lexType == LexType::RPAREN) {
			// 空参数列表，无需处理
		}
		// 检查有效参数起始 Token
		else if (currentToken().lexType == LexType::INTEGER ||
			currentToken().lexType == LexType::CHAR ||
			currentToken().lexType == LexType::ARRAY ||
			currentToken().lexType == LexType::RECORD ||
			currentToken().lexType == LexType::ID ||
			currentToken().lexType == LexType::VAR) {

			p = paramDecList();  // 解析参数声明链
			t->addChild(0, move(p)); // 参数列表作为第一个子节点

		}
		else {
			logError(ErrorType::SyntaxError,
				"Invalid parameter type: " + lexTypeToString(currentToken().lexType));
		}
	}

// 解析形参声明列表；多组形参之间用分号分隔。
TreenodePtr Parser::paramDecList() {
		TreenodePtr t = param();         // 第一个参数
		TreenodePtr p = paramMore();     // 更多参数

		if (p) {
			t->addSibling(move(p));  // 连接兄弟节点
		}

		return t;
	}

// 解析后续形参声明；遇到右括号表示形参列表结束。
TreenodePtr Parser::paramMore() {
		TreenodePtr t = nullptr;
		if (currentToken().lexType == LexType::RPAREN) {
			// 空参数列表，无需处理
		}
		else if (currentToken().lexType == LexType::strI) {  // 分号分隔参数
			match(LexType::strI);
			t = paramDecList();        // 递归解析后续参数
			if (!t) {
				logError(ErrorType::MissingDeclaration,
					"Missing parameter after ;");
			}
		}

		return t;
	}

// 解析一组形参声明，并区分值参和 VAR 变参。
TreenodePtr Parser::param() {
		auto t = Treenode::create(Treenodecate::DecK, currentToken().line);
		if (currentToken().lexType == LexType::INTEGER ||
			currentToken().lexType == LexType::CHAR ||
			currentToken().lexType == LexType::ARRAY ||
			currentToken().lexType == LexType::RECORD ||
			currentToken().lexType == LexType::ID) {
			t->attr.paramt = ParamTcate::valparamtype;
			typeDef(t);
			formList(t);
		}
		else if (currentToken().lexType == LexType::VAR) {
			match(LexType::VAR);
			t->attr.paramt = ParamTcate::varparamtype;
			typeDef(t);
			formList(t);
		}
		return t;
	}

// 解析同一类型下的形参名列表。
void Parser::formList(TreenodePtr& t) {
		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);  // 存储参数名
			match(LexType::ID);
		}
		fidMore(t);  // 处理更多参数
	}

// 解析逗号分隔的后续形参名。
void Parser::fidMore(TreenodePtr& t) {
		if (currentToken().lexType == LexType::strI ||
			currentToken().lexType == LexType::RPAREN) {
			return;  // 由上层函数处理
		}
		else if (currentToken().lexType == LexType::COMMA) {
			match(LexType::COMMA);
			formList(t);
		}
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token in parameter list: " +
				lexTypeToString(currentToken().lexType));
		}
	}

// 解析过程内部的声明部分，复用普通声明部分逻辑。
TreenodePtr Parser::procDecPart() {
		return declarePart();
	}

// 解析过程体，内部结构与主程序体相同。
TreenodePtr Parser::procBody() {
		TreenodePtr t = programBody();  // 复用 programBody 逻辑

		if (!t) {
			logError(ErrorType::MissingProgramBody,
				"Procedure body is incomplete or missing");
		}

		return t;
	}

// 解析 BEGIN...END 包围的语句序列节点。
TreenodePtr Parser::programBody() {
		auto t = Treenode::create(Treenodecate::StmLK, currentToken().line);
		match(LexType::BEGIN);
		t->addChild(0, move(stmList()));
		match(LexType::END);
		return t;
	}

// 解析语句序列，第一条语句作为链表头，后续语句挂到 sibling。
TreenodePtr Parser::stmList() {
		TreenodePtr t = stm();          // 解析第一个语句
		TreenodePtr p = stmMore();      // 解析后续语句

		if (p) {
			t->addSibling(move(p));     // 将后续语句链接为兄弟节点
		}

		return t;
	}

// 解析语句序列中的后续语句；遇到块结束符时停止。
TreenodePtr Parser::stmMore() {
		TreenodePtr t = nullptr;

		// 允许语句列表结束的情况
		if (currentToken().lexType == LexType::END ||
			currentToken().lexType == LexType::ENDWH ||
			currentToken().lexType == LexType::ELSE ||
			currentToken().lexType == LexType::FI) {
			// 不生成节点，直接返回空
		}
		else if (currentToken().lexType == LexType::strI) {
			match(LexType::strI);
			t = stmList();
		}
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token after statement: " +
				lexTypeToString(currentToken().lexType));
		}

		return t;
	}

// 根据当前 Token 判断具体语句类型，并创建对应语句节点。
TreenodePtr Parser::stm() {
		TreenodePtr t = nullptr;
		switch (currentToken().lexType) {
		case LexType::IF:
			t = conditionalStm();
			break;
		case LexType::WHILE:
			t = loopStm();
			break;
		case LexType::RETURN:
			t = returnStm();
			break;
		case LexType::READ:
			t = inputStm();
			break;
		case LexType::WRITE:
			t = outputStm();
			break;
		case LexType::ID: {
			auto f = Treenode::create(Treenodecate::StmtK, currentToken().line);
			auto t1 = Treenode::create(Treenodecate::ExpK, currentToken().line);
			t1->kind.exp = ExpKcate::IdEK;
			t1->attr.ExpAttr.varkind = Varkind::IdV;
			t1->addName(currentToken().str); // 存储标识符名称
			f->addChild(0, move(t1));
			assCall(f);
			t = move(f);
			break;
		}
		default:
			logError(ErrorType::SyntaxError,
				"Unexpected token in statement: " +
				lexTypeToString(currentToken().lexType));
			break;
		}

		return t;
	}

// 对 ID 开头的语句进行二次判定：赋值语句或过程调用语句。
void Parser::assCall(TreenodePtr& t) {
		match(LexType::ID);
		if (currentToken().lexType == LexType::ASSIGN ||
			currentToken().lexType == LexType::LMIDPAREN ||
			currentToken().lexType == LexType::DOT) {
			assignmentRest(t);
			t->kind.stmt = StmtKcate::AssignK;
		}
		else if (currentToken().lexType == LexType::LPAREN) {
			// 处理过程调用
			callStmRest(t);
			t->kind.stmt = StmtKcate::CallK;

		}
		else {
			logError(ErrorType::SyntaxError,
				"Expected assignment or call after identifier, got: " +
				lexTypeToString(currentToken().lexType));
		}
	}

// 解析 IF 条件语句，child[0] 为条件，child[1] 为 THEN 分支，child[2] 为 ELSE 分支。
TreenodePtr Parser::conditionalStm() {
		auto t = Treenode::create(Treenodecate::StmtK, currentToken().line);
		t->kind.stmt = StmtKcate::IfK;
		match(LexType::IF);
		t->addChild(0, expression());
		match(LexType::THEN);
		t->addChild(1, stmList());
		if (currentToken().lexType == LexType::ELSE) {
			match(LexType::ELSE);
			t->addChild(2, stmList());
		}
		match(LexType::FI);
		return t;
	}

// 解析 WHILE 循环语句，child[0] 为循环条件，child[1] 为循环体。
TreenodePtr Parser::loopStm() {
		auto t = Treenode::create(Treenodecate::StmtK, currentToken().line);
		t->kind.stmt = StmtKcate::WhileK;
		match(LexType::WHILE);  // 必须匹配 WHILE
		// 解析循环条件 (第一个子节点)
		t->addChild(0, expression());
		match(LexType::DO);
		// 解析循环体 (第二个子节点)
		t->addChild(1, stmList());
		match(LexType::ENDWH);
		return t;
	}

// 解析 RETURN 语句；当前实现不解析返回值表达式。
TreenodePtr Parser::returnStm() {
		auto t = Treenode::create(Treenodecate::StmtK, currentToken().line);
		t->kind.stmt = StmtKcate::ReturnK;

		match(LexType::RETURN);

		// 注意：根据实际语法，可能需要解析返回值表达式
		// 此处根据原代码逻辑，RETURN 后无表达式
		return t;
	}

// 解析 READ 输入语句，并记录被读取的变量名。
TreenodePtr Parser::inputStm() {
		auto t = Treenode::create(Treenodecate::StmtK, currentToken().line);
		t->kind.stmt = StmtKcate::ReadK;
		match(LexType::READ);
		match(LexType::LPAREN);
		// 解析变量名
		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);
		}
		match(LexType::ID);
		match(LexType::RPAREN);
		return t;
	}

// 解析 WRITE 输出语句，输出内容作为表达式子节点保存。
TreenodePtr Parser::outputStm() {
		auto t = Treenode::create(Treenodecate::StmtK, currentToken().line);
		t->kind.stmt = StmtKcate::WriteK;
		match(LexType::WRITE);
		match(LexType::LPAREN);
		// 解析输出表达式 (第一个子节点)
		t->addChild(0, expression());
		match(LexType::RPAREN);
		return t;
	}

// 解析赋值语句余部：变量后缀、赋值符号以及右侧表达式。
void Parser::assignmentRest(TreenodePtr& t) {
		variMore(t->child[0]);  // 假设 variMore 已实现

		// 匹配赋值符号 :=
		match(LexType::ASSIGN);
		// 解析右部表达式作为第二个子节点
		t->addChild(1, expression());
	}

// 解析过程调用余部：括号中的实参列表。
void Parser::callStmRest(TreenodePtr& t) {
		match(LexType::LPAREN);
		// 解析实际参数列表作为第二个子节点
		t->addChild(1, actParamList());

		match(LexType::RPAREN);
	}

// 解析实参列表；多个实参通过 sibling 链接。
TreenodePtr Parser::actParamList() {
		TreenodePtr t = nullptr;
		if (currentToken().lexType == LexType::RPAREN) {
			// 空参数列表
		}
		// 检查有效参数起始 Token
		else if (currentToken().lexType == LexType::ID ||
			currentToken().lexType == LexType::INTC) {
			t = expression();  // 解析第一个参数
			// 处理更多参数
			if (t) {
				t->addSibling(move(actParamMore()));
			}
		}
		else {
			logError(ErrorType::SyntaxError,
				"Invalid actual parameter: " +
				lexTypeToString(currentToken().lexType));
		}
		return t;
	}

// 解析逗号分隔的后续实参。
TreenodePtr Parser::actParamMore() {
		TreenodePtr t = nullptr;
		if (currentToken().lexType == LexType::RPAREN) {
			// 空参数列表
		}
		else if (currentToken().lexType == LexType::COMMA) {  // 逗号分隔参数
			match(LexType::COMMA);
			t = actParamList();         // 递归解析后续参数
		}
		return t;
	}

// 解析表达式：simpleExp 或 simpleExp 比较运算符 simpleExp。
TreenodePtr Parser::expression() {
		int line = currentToken().line;
		TreenodePtr t = simpleExp();  // 解析左操作数

		// 检查比较运算符
		if (currentToken().lexType == LexType::LT ||
			currentToken().lexType == LexType::EQ) {

			auto p = Treenode::create(Treenodecate::ExpK, line);
			p->kind.exp = ExpKcate::OpK;
			p->attr.ExpAttr.op = currentToken().lexType;  // 存储操作符类型
			p->addChild(0, move(t));

			// 消费操作符
			match(currentToken().lexType);

			// 解析右操作数作为第二个子节点
			p->addChild(1, simpleExp());

			t = move(p);  // 更新当前根节点
		}
		return t;
	}

// 解析简单表达式，按左结合方式处理连续的加减运算。
TreenodePtr Parser::simpleExp() {
		int line = currentToken().line;
		TreenodePtr t = term();  // 解析第一个项

		// 循环处理连续的操作符
		while (currentToken().lexType == LexType::PLUS ||
			currentToken().lexType == LexType::MINUS) {

			auto p = Treenode::create(Treenodecate::ExpK, line);
			p->kind.exp = ExpKcate::OpK;
			p->attr.ExpAttr.op = currentToken().lexType;
			p->addChild(0, move(t));

			// 消费操作符
			match(currentToken().lexType);

			// 解析右项作为第二个子节点
			p->addChild(1, term());

			t = move(p);  // 更新当前根节点
			line = t->lineno;  // 保持行号一致
		}
		return t;
	}

// 解析项，按左结合方式处理连续的乘除运算。
TreenodePtr Parser::term() {
		int line = currentToken().line;
		TreenodePtr t = factor();  // 解析第一个因子

		// 处理连续乘除运算
		while (currentToken().lexType == LexType::TIMES ||
			currentToken().lexType == LexType::OVER) {

			auto p = Treenode::create(Treenodecate::ExpK, line);
			p->kind.exp = ExpKcate::OpK;
			p->attr.ExpAttr.op = currentToken().lexType;
			p->addChild(0, move(t));

			// 消费操作符
			match(currentToken().lexType);

			// 解析右操作数作为第二个子节点
			p->addChild(1, factor());

			t = move(p);  // 更新当前根节点
			line = t->lineno;  // 保持行号更新
		}
		return t;
	}

// 解析因子：整型常量、变量引用或括号表达式。
TreenodePtr Parser::factor() {
		TreenodePtr t = nullptr;
		int line = currentToken().line;

		switch (currentToken().lexType) {
		case LexType::INTC: {  // 整型常量
			t = Treenode::create(Treenodecate::ExpK, line);
			t->kind.exp = ExpKcate::ConstK;
			t->attr.ExpAttr.val = stoi(currentToken().str);
			match(LexType::INTC);
			break;
		}
		case LexType::ID:      // 变量或函数调用
			t = variable();   
			break;
		case LexType::LPAREN:  // 括号表达式
			match(LexType::LPAREN);
			t = expression();
			match(LexType::RPAREN);
			break;
		default:
			logError(ErrorType::SyntaxError,
				"Invalid factor: " + lexTypeToString(currentToken().lexType));
			break;
		}
		return t;
	}

// 解析变量引用，并继续识别数组下标或记录字段访问。
TreenodePtr Parser::variable() {
		auto t = Treenode::create(Treenodecate::ExpK, currentToken().line);
		t->kind.exp = ExpKcate::IdEK;  // 默认为基本标识符

		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);  // 存储变量名
			match(LexType::ID);
			variMore(t);  // 处理数组/记录访问
		}
		else {
			logError(ErrorType::SyntaxError, "Expected identifier");
		}
		return t;
	}

// 解析变量后缀：数组成员访问 [exp] 或记录成员访问 .field。
void Parser::variMore(TreenodePtr& t) {
		// 检查可能的结束符号（不需要处理的情况）
		if (currentToken().lexType == LexType::ASSIGN ||
			currentToken().lexType == LexType::TIMES ||
			currentToken().lexType == LexType::EQ||
			currentToken().lexType == LexType::LT ||
			currentToken().lexType == LexType::PLUS ||
			currentToken().lexType == LexType::MINUS||
			currentToken().lexType == LexType::OVER ||
			currentToken().lexType == LexType::RPAREN ||
			currentToken().lexType == LexType::strI||
			currentToken().lexType == LexType::COMMA ||
			currentToken().lexType == LexType::THEN ||
			currentToken().lexType == LexType::RMIDPAREN ||
			currentToken().lexType == LexType::ELSE ||
			currentToken().lexType == LexType::FI ||
			currentToken().lexType == LexType::DO ||
			currentToken().lexType == LexType::ENDWH ||
			currentToken().lexType == LexType::END
			){ }
		
		else if (currentToken().lexType == LexType::LMIDPAREN) {  // 数组访问 [exp]
			match(LexType::LMIDPAREN);

			auto indexExp = expression();  // 解析下标表达式
			t->addChild(0, move(indexExp));
			t->attr.ExpAttr.varkind = Varkind::ArrayMembV;
			t->child[0]->attr.ExpAttr.varkind = Varkind::IdV;
			match(LexType::RMIDPAREN);

		}
		else if (currentToken().lexType == LexType::DOT) {  // 记录访问 .field
			match(LexType::DOT);
			t->addChild(0, move(fieldVar()));
			t->attr.ExpAttr.varkind = Varkind::FieldMembV;
			t->child[0]->attr.ExpAttr.varkind = Varkind::IdV;
		}
		else {
			logError(ErrorType::SyntaxError,
				"Unexpected token after variable: " +
				lexTypeToString(currentToken().lexType));
		}
	}

// 解析记录字段变量名，并处理字段后可能出现的数组下标。
TreenodePtr Parser::fieldVar() {
		auto t = Treenode::create(Treenodecate::ExpK, currentToken().line);
		t->kind.exp = ExpKcate::IdEK;
		if (currentToken().lexType == LexType::ID) {
			t->addName(currentToken().str);  // 存储字段名
			match(LexType::ID);
			fieldVarMore(t);  // 处理可能的数组访问后缀
		}
		else {
			logError(ErrorType::SyntaxError, "Missing field name after .");
		}
		return t;
	}

// 解析记录字段后的后缀；当前主要支持字段数组成员访问。
void Parser::fieldVarMore(TreenodePtr& t) {
		if (currentToken().lexType == LexType::ASSIGN ||
			currentToken().lexType == LexType::TIMES ||
			currentToken().lexType == LexType::EQ ||
			currentToken().lexType == LexType::LT ||
			currentToken().lexType == LexType::PLUS ||
			currentToken().lexType == LexType::MINUS ||
			currentToken().lexType == LexType::OVER ||
			currentToken().lexType == LexType::RPAREN ||
			currentToken().lexType == LexType::strI ||
			currentToken().lexType == LexType::COMMA ||
			currentToken().lexType == LexType::THEN ||
			currentToken().lexType == LexType::RMIDPAREN ||
			currentToken().lexType == LexType::ELSE ||
			currentToken().lexType == LexType::FI ||
			currentToken().lexType == LexType::DO ||
			currentToken().lexType == LexType::ENDWH ||
			currentToken().lexType == LexType::END
			) { }
		else if (currentToken().lexType == LexType::LMIDPAREN) {
			match(LexType::LMIDPAREN);

			// 解析下标表达式作为第一个子节点
			t->addChild(0, expression());
			t->child[0]->attr.ExpAttr.varkind = Varkind::ArrayMembV;
			match(LexType::RMIDPAREN);
		}
		else{
			logError(ErrorType::SyntaxError,
				"Unexpected token after field access: " +
				lexTypeToString(currentToken().lexType));
		}
	}

// 匹配期望的词法类型；匹配成功则消费 Token，失败则报语法错误。
bool Parser::match(LexType expected) {
		if (currentToken().lexType == expected) {
			consumeToken();
			return true;
		}
		logError(ErrorType::SyntaxError,
			"Expected " + lexTypeToString(expected) +
			" but got " + lexTypeToString(currentToken().lexType));
		return false;
	}
