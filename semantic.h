#ifndef COMPILER_SEMANTIC_H
#define COMPILER_SEMANTIC_H

#include "ast.h"

#include <string>
#include <vector>

typedef enum { typeKind, varKind, procKind } IdKind;
typedef enum { indir, dir } AccessKind;

typedef struct typeIR TypeIR;

typedef struct fieldChain {
    string idName;
    TypeIR* unitType;
    int off;
    fieldChain* next;
    fieldChain(string idName, TypeIR* unitType, int off);
    fieldChain();
} FieldChain;

struct typeIR {
    int size;
    DecKcate typeKind;
    union {
        struct {
            int low;
            int up;
            TypeIR* elemTy;
        } ArrayAttr;
        FieldChain* body;
    } More;
    typeIR(DecKcate type);
    typeIR(DecKcate type, int low, int up, TypeIR* elemTy);
    typeIR(DecKcate type, FieldChain* bodyPtr);
    typeIR();
};

struct SymbolsTable;

typedef struct paramTable {
    SymbolsTable* entry;
    paramTable* next;
    paramTable();
} ParamTable;

typedef struct attributeIR {
    TypeIR* idType;
    IdKind idKind;
    union {
        struct {
            AccessKind access;
            int level;
            int off;
        } VarAttr;
        struct {
            int level;
            ParamTable* param;
            int off;
            int moff;
            int code;
            int size;
        } ProcAttr;
    } More;
    attributeIR();
    attributeIR(TypeIR* idType, IdKind idKind);
    attributeIR(TypeIR* idType, IdKind idKind, AccessKind access, int level, int off);
    attributeIR(TypeIR* idType, IdKind idKind, int level, ParamTable* param, int off);
} AttributeIR;

struct SymbolsTable {
    string idName;
    AttributeIR* attrIR;
    SymbolsTable* next;
};

extern TypeIR* intPtr;
extern TypeIR* charPtr;
extern TypeIR* boolPtr;
extern vector<SymbolsTable*> scope;
extern int Leveloff;

void initType();
void CreateTable();
SymbolsTable* Enter(string idname, AttributeIR* attriP, SymbolsTable* tableToEnter);
SymbolsTable* FindEntry(string idname, int flag);
TreenodePtr semanticAnalyze(TreenodePtr&& t);

#endif /* COMPILER_SEMANTIC_H */
