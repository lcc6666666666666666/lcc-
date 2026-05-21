#ifndef COMPILER_SEMANTIC_H
#define COMPILER_SEMANTIC_H

#include "ast.h"

#include <string>
#include <vector>

// 标识符类别：类型名、变量名、过程名。
typedef enum { typeKind, varKind, procKind } IdKind;

// 变量访问方式：直接访问普通变量，间接访问通常用于 var 形参。
typedef enum { indir, dir } AccessKind;

typedef struct typeIR TypeIR;

// record 类型的字段链表节点，记录字段名、字段类型和字段偏移。
typedef struct fieldChain {
    string idName;
    TypeIR* unitType;
    int off;
    fieldChain* next;
    fieldChain(string idName, TypeIR* unitType, int off);
    fieldChain();
} FieldChain;

// 类型内部表示：描述 integer/char/array/record 等类型的大小和附加信息。
struct typeIR {
    int size;
    DecKcate typeKind;

    // 数组类型使用 ArrayAttr；记录类型使用 body 字段链。
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

// 过程形参表，每个参数指向其对应的符号表项。
typedef struct paramTable {
    SymbolsTable* entry;
    paramTable* next;
    paramTable();
} ParamTable;

// 符号表项属性：保存标识符的类型、种类以及变量/过程专有信息。
typedef struct attributeIR {
    TypeIR* idType;
    IdKind idKind;

    // More 保存不同种类标识符的专有属性。
    // 使用 union 是因为一个符号表项只会是变量或过程中的一种，
    // idKind == varKind 时使用 VarAttr，idKind == procKind 时使用 ProcAttr。
    union {
        struct {
            AccessKind access; // 变量访问方式：dir 表示直接存放变量值，indir 表示间接存放变量地址，主要用于 var 形参。
            int level;         // 变量所在的作用域层号，用于嵌套过程访问外层变量时定位活动记录。
            int off;           // 变量在所属活动记录 AR 中的偏移量，代码生成阶段用它计算变量地址。
        } VarAttr;
        struct {
            int level;         // 过程所在的作用域层号，代码生成阶段用它维护 display 表。
            ParamTable* param; // 过程形参表，按声明顺序连接每个形参的符号表入口。
            int off;           // 过程体内局部变量区域的起始偏移，也作为 display 区相关偏移使用。
            int moff;          // 过程活动记录 AR 的总大小，调用过程时据此为栈帧分配空间。
        } ProcAttr;
    } More;
    attributeIR();
    attributeIR(TypeIR* idType, IdKind idKind);
    attributeIR(TypeIR* idType, IdKind idKind, AccessKind access, int level, int off);
    attributeIR(TypeIR* idType, IdKind idKind, int level, ParamTable* param);
} AttributeIR;

// 单个符号表项；同一作用域内的符号表项通过 next 串成链表。
struct SymbolsTable {
    string idName;
    AttributeIR* attrIR;
    SymbolsTable* next;
};

// 内置类型的共享 TypeIR 指针，便于语义检查时直接比较类型。
extern TypeIR* intPtr;
extern TypeIR* charPtr;
extern TypeIR* boolPtr;

// 作用域栈：每一层保存一张符号表；Leveloff 记录当前层变量偏移。
extern vector<SymbolsTable*> scope;
extern int Leveloff;

// 初始化内置类型。
void initType();

// 创建新的符号表作用域。
void CreateTable();

// 向指定符号表插入标识符。
SymbolsTable* Enter(string idname, AttributeIR* attriP, SymbolsTable* tableToEnter);

// 查找符号表项；flag 通常用于区分只查当前层还是向外层查找。
SymbolsTable* FindEntry(string idname, int flag);

// 语义分析入口：遍历语法树，建立符号表并做语义检查。
TreenodePtr semanticAnalyze(TreenodePtr&& t);

#endif /* COMPILER_SEMANTIC_H */
