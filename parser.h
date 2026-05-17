#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "ast.h"

#include <sstream>

class Parser {
private:
    vector<MyToken> tokens;
    size_t currentIndex = 0;
    stringstream errorLog;

    MyToken& currentToken();
    void consumeToken();
    void logError(ErrorType type, const string& details = "");

    TreenodePtr program();
    TreenodePtr programHead();
    TreenodePtr declarePart();
    TreenodePtr typeDecpart();
    TreenodePtr typeDec();
    TreenodePtr typeDecList();
    TreenodePtr typeDecMore();
    void typeId(TreenodePtr& t);
    void typeDef(TreenodePtr& t);
    void baseType(TreenodePtr& t);
    void structureType(TreenodePtr& t);
    void arrayType(TreenodePtr& t);
    void recType(TreenodePtr& t);
    TreenodePtr fieldDecList();
    TreenodePtr fieldDecMore();
    void idList(TreenodePtr& t);
    void idMore(TreenodePtr& t);
    TreenodePtr varDecpart();
    TreenodePtr varDec();
    TreenodePtr varDecList();
    TreenodePtr varDecMore();
    void varIdList(TreenodePtr& t);
    void varIdMore(TreenodePtr& t);
    TreenodePtr procDecpart();
    TreenodePtr procDec();
    void paramList(TreenodePtr& t);
    TreenodePtr paramDecList();
    TreenodePtr paramMore();
    TreenodePtr param();
    void formList(TreenodePtr& t);
    void fidMore(TreenodePtr& t);
    TreenodePtr procDecPart();
    TreenodePtr procBody();
    TreenodePtr programBody();
    TreenodePtr stmList();
    TreenodePtr stmMore();
    TreenodePtr stm();
    void assCall(TreenodePtr& t);
    TreenodePtr conditionalStm();
    TreenodePtr loopStm();
    TreenodePtr returnStm();
    TreenodePtr inputStm();
    TreenodePtr outputStm();
    void assignmentRest(TreenodePtr& t);
    void callStmRest(TreenodePtr& t);
    TreenodePtr actParamList();
    TreenodePtr actParamMore();
    TreenodePtr expression();
    TreenodePtr simpleExp();
    TreenodePtr term();
    TreenodePtr factor();
    TreenodePtr variable();
    void variMore(TreenodePtr& t);
    TreenodePtr fieldVar();
    void fieldVarMore(TreenodePtr& t);
    bool match(LexType expected);

public:
    explicit Parser(vector<MyToken> tokenList);
    TreenodePtr parse();
    string lexTypeToString(LexType type);
    void printTree(const TreenodePtr& root);
};

#endif /* COMPILER_PARSER_H */
