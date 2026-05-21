#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "ast.h"

#include <sstream>

// 递归下降语法分析器：接收词法分析得到的 Token 序列，
// 按 SNL/类 Pascal 语法构造语法树，并在分析过程中记录语法错误。
class Parser {
private:
    // 词法分析器输出的 Token 列表。
    vector<MyToken> tokens;

    // 当前正在分析的 Token 下标。
    size_t currentIndex = 0;

    // 语法错误日志，用于记录缺失声明、缺失程序体等错误信息。
    stringstream errorLog;

    // 返回当前 Token。
    MyToken& currentToken();

    // 消耗当前 Token，将分析指针移动到下一个 Token。
    void consumeToken();

    // 记录语法错误类型及补充说明。
    void logError(ErrorType type, const string& details = "");

    // program -> programHead declarePart programBody .
    TreenodePtr program();

    // programHead -> PROGRAM ID
    TreenodePtr programHead();

    // 声明部分：依次分析类型声明、变量声明和过程声明。
    TreenodePtr declarePart();

    // 类型声明可选部分：TYPE typeDecList 或空。
    TreenodePtr typeDecpart();

    // 类型声明入口，匹配 TYPE 关键字。
    TreenodePtr typeDec();

    // 一条类型声明：typeId = typeDef ;
    TreenodePtr typeDecList();

    // 后续类型声明，形成兄弟节点链。
    TreenodePtr typeDecMore();

    // 解析类型名标识符。
    void typeId(TreenodePtr& t);

    // 解析类型定义：基本类型、结构类型或用户自定义类型。
    void typeDef(TreenodePtr& t);

    // 解析基本类型：INTEGER 或 CHAR。
    void baseType(TreenodePtr& t);

    // 解析结构类型：ARRAY 或 RECORD。
    void structureType(TreenodePtr& t);

    // 解析数组类型：ARRAY [low..up] OF baseType。
    void arrayType(TreenodePtr& t);

    // 解析记录类型：RECORD fieldDecList END。
    void recType(TreenodePtr& t);

    // 解析记录字段声明列表。
    TreenodePtr fieldDecList();

    // 解析更多记录字段声明。
    TreenodePtr fieldDecMore();

    // 解析标识符列表：ID {, ID}。
    void idList(TreenodePtr& t);

    // 解析标识符列表中的后续标识符。
    void idMore(TreenodePtr& t);

    // 变量声明可选部分：VAR varDecList 或空。
    TreenodePtr varDecpart();

    // 变量声明入口，匹配 VAR 关键字。
    TreenodePtr varDec();

    // 一条变量声明：typeDef varIdList ;
    TreenodePtr varDecList();

    // 后续变量声明，形成兄弟节点链。
    TreenodePtr varDecMore();

    // 解析变量名列表。
    void varIdList(TreenodePtr& t);

    // 解析变量名列表中的后续变量名。
    void varIdMore(TreenodePtr& t);

    // 过程声明可选部分：PROCEDURE procDec 或空。
    TreenodePtr procDecpart();

    // 解析过程声明，包括过程头、参数、局部声明和过程体。
    TreenodePtr procDec();

    // 解析形参列表并挂到过程声明节点。
    void paramList(TreenodePtr& t);

    // 解析形参声明列表。
    TreenodePtr paramDecList();

    // 解析更多形参声明。
    TreenodePtr paramMore();

    // 解析单组形参声明，区分值参和变参。
    TreenodePtr param();

    // 解析同一类型下的形参名列表。
    void formList(TreenodePtr& t);

    // 解析形参名列表中的后续标识符。
    void fidMore(TreenodePtr& t);

    // 解析过程内部的声明部分。
    TreenodePtr procDecPart();

    // 解析过程体，复用程序体 BEGIN...END 的分析逻辑。
    TreenodePtr procBody();

    // 程序体或过程体：BEGIN stmList END。
    TreenodePtr programBody();

    // 解析语句序列。
    TreenodePtr stmList();

    // 解析语句序列中的后续语句。
    TreenodePtr stmMore();

    // 根据当前 Token 分派到具体语句分析函数。
    TreenodePtr stm();

    // 区分以 ID 开头的赋值语句和过程调用语句。
    void assCall(TreenodePtr& t);

    // 解析条件语句：IF expression THEN stmList [ELSE stmList] FI。
    TreenodePtr conditionalStm();

    // 解析循环语句：WHILE expression DO stmList ENDWH。
    TreenodePtr loopStm();

    // 解析返回语句：RETURN。
    TreenodePtr returnStm();

    // 解析输入语句：READ(ID)。
    TreenodePtr inputStm();

    // 解析输出语句：WRITE(expression)。
    TreenodePtr outputStm();

    // 解析赋值语句中 ID 后面的变量后缀和右侧表达式。
    void assignmentRest(TreenodePtr& t);

    // 解析过程调用中的实参列表。
    void callStmRest(TreenodePtr& t);

    // 解析实参列表。
    TreenodePtr actParamList();

    // 解析更多实参。
    TreenodePtr actParamMore();

    // 解析表达式，支持比较运算 < 和 =。
    TreenodePtr expression();

    // 解析简单表达式，处理加减运算。
    TreenodePtr simpleExp();

    // 解析项，处理乘除运算。
    TreenodePtr term();

    // 解析因子：常量、变量或括号表达式。
    TreenodePtr factor();

    // 解析变量引用。
    TreenodePtr variable();

    // 解析变量后缀：数组下标或记录字段访问。
    void variMore(TreenodePtr& t);

    // 解析记录字段变量。
    TreenodePtr fieldVar();

    // 解析记录字段后的数组下标等后缀。
    void fieldVarMore(TreenodePtr& t);

    // 匹配期望的 Token 类型，成功则前进，失败则记录语法错误。
    bool match(LexType expected);

public:
    // 构造语法分析器，传入完整 Token 序列。
    explicit Parser(vector<MyToken> tokenList);

    // 启动语法分析，返回语法树根节点。
    TreenodePtr parse();

    // 将词法类型转换为可读字符串，主要用于错误提示。
    string lexTypeToString(LexType type);

    // 将语法树打印到输出文件和控制台。
    void printTree(const TreenodePtr& root);
};

#endif /* COMPILER_PARSER_H */
