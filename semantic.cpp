#include "semantic.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

// 作用：构造记录类型的一个域节点，保存域名、域类型和域内偏移。
FieldChain::fieldChain(string idName, TypeIR* unitType, int off) :idName(idName), unitType(unitType), off(off), next(nullptr) {
}

// 作用：构造一个空的记录域链表节点，常用作临时头节点。
FieldChain::fieldChain() :next(nullptr), off(0) {
}

//类型内部结构


// 作用：默认构造类型信息对象。
TypeIR::typeIR() {}

// 作用：构造基本类型信息，并设置 integer、char、bool 的存储大小。
TypeIR::typeIR(DecKcate type) {
    typeKind = type;
    switch (type) {
    case IntegerK:size = 4;
        break;
    case CharK:size = 4;
        break;
    case BoolK:size = 4;
        break;
    }
}

// 作用：构造数组类型信息，记录上下界、元素类型并计算数组总大小。
TypeIR::typeIR(DecKcate type, int low, int up, TypeIR* elemTy) {
    typeKind = type;
    if (type != ArrayK) {
        cout << "type不是ArrayK" << endl;
        exit(1);
    }
    this->More.ArrayAttr.low = low;
    this->More.ArrayAttr.up = up;
    this->More.ArrayAttr.elemTy = elemTy;
    this->size = (up - low + 1) * elemTy->size;
}

// 作用：构造记录类型信息，保存域链表并累计记录整体大小。
TypeIR::typeIR(DecKcate type, FieldChain* bodyPtr) {
    typeKind = type;
    if (type != RecordK) {
        cout << "type不是RecordK" << endl;
        exit(1);
    }
    this->More.body = bodyPtr;
    int recordsize = 0;

    while (bodyPtr) {
        recordsize += bodyPtr->unitType->size;
        bodyPtr = bodyPtr->next;
    }
    this->size = recordsize;
}


//参数表

// 作用：构造过程形参表节点。
paramTable::paramTable() :entry(nullptr), next(nullptr) {}

//标识符信息项


// 作用：构造类型标识符的属性信息。
AttributeIR::attributeIR(TypeIR* idType, IdKind idKind) {
    if (idKind != typeKind) {
        cout << "attributeIr中IdKind不为typeKind" << endl;
        exit(1);
    }
    this->idType = idType;
    this->idKind = idKind;
}

// 作用：构造变量标识符的属性信息，包括访问方式、层号和偏移。
AttributeIR::attributeIR(TypeIR* idType, IdKind idKind, AccessKind access, int level, int off) {
    if (idKind != varKind) {
        cout << "attributeIr中IdKind不为varKind" << endl;
        exit(1);
    }
    this->idType = idType;
    this->idKind = idKind;
    this->More.VarAttr.access = access;
    this->More.VarAttr.level = level;
    this->More.VarAttr.off = off;
}

// 作用：构造过程标识符的属性信息，包括层号、参数表和活动记录偏移。
AttributeIR::attributeIR(TypeIR* idType, IdKind idKind, int level, ParamTable* param) {
    if (idKind != procKind) {
        cout << "attributeIr中IdKind不为procKind" << endl;
        exit(1);
    }
    this->idType = idType;
    this->idKind = idKind;
    this->More.ProcAttr.off = 0;
    this->More.ProcAttr.moff = 0;//level+1+当前层符号数
    this->More.ProcAttr.param = param;
    this->More.ProcAttr.level = level;
}

// 作用：默认构造标识符属性信息对象。
AttributeIR::attributeIR() {}


//SNL符号表



TypeIR* intPtr, * charPtr, *boolPtr;

// 作用：初始化系统内置的 integer、char、bool 类型对象。
void initType() {
    intPtr = new TypeIR(IntegerK);
    charPtr = new TypeIR(CharK);
    boolPtr = new TypeIR(BoolK);
}

vector<SymbolsTable*>scope;
int Leveloff=0;

// 作用：创建新的作用域符号表，并初始化当前层变量偏移。
void CreateTable() {
    Leveloff = 28;
    SymbolsTable* add = new SymbolsTable();
    add->next = nullptr;
    scope.emplace_back(add);
}

// 作用：把一个标识符及其属性插入到指定符号表尾部，并返回新表项。
SymbolsTable* Enter(string idname, AttributeIR* attriP, SymbolsTable* tableToEnter) {

    SymbolsTable* add = new SymbolsTable();
    add->attrIR = attriP;
    add->idName = idname;
    add->next = nullptr;
    SymbolsTable* end = tableToEnter;
    while (end->next) {
        end = end->next;
    }
    end->next = add;
    return add;
}

// 作用：查找标识符；flag 为 1 时只查当前层，否则从内层向外层查找。
SymbolsTable* FindEntry(string idname, int flag) {
    
    if (flag == 1) {
        
        SymbolsTable* search = scope.back()->next;
        while (search != nullptr && search->idName != idname) {
            search = search->next;
        }
        return search;
    }
    else {
        int len = scope.size();
        for (int i = len - 1; i >= 0; i--) {
            SymbolsTable* search = scope[i]->next;
            while (search != nullptr && search->idName != idname)
            {
                search = search->next;
            }
            if (search != nullptr)return search;
        }
        return nullptr;
        
    }
    return nullptr;
}

// 作用：根据数组声明节点生成数组类型信息，并检查数组上下界和元素类型。
TypeIR* arrayType(Treenode* t, string name) {
    int low = t->attr.ArrayAttr.low, up = t->attr.ArrayAttr.up;
    if (low > up) {
        
        cout << "line " << t->lineno << ": " << name << " :array下标越界" << endl;
        exit(1);
    }

    TypeIR* elemTy = nullptr;
    if (t->attr.ArrayAttr.childType == IntegerK) {
        elemTy = intPtr;
    }
    else if (t->attr.ArrayAttr.childType == CharK) {
        elemTy = charPtr;
    }
    else {
        cout << "line " << t->lineno << ": " << name << " :array成员类型无法识别" << endl;
        exit(1);
    }
    return new TypeIR(ArrayK, low, up, elemTy);
}

// 作用：根据记录声明节点生成记录类型信息，建立记录域链表并计算各域偏移。
TypeIR* recordType(Treenode* t, string name) {
    TreenodePtr& tmp = t->child[0];

    int off = 0;
    FieldChain* search = new FieldChain(), * body = search;
    Treenode* recordMember=tmp.get();
    while (recordMember != nullptr) {

        if (recordMember->kind.dec == IntegerK) {

            for (string memName : recordMember->name) {
                search->next = new FieldChain(memName, intPtr, off);
                search = search->next;
                off += 4;
            }

        }
        else if (recordMember->kind.dec == CharK) {
            for (string memName : recordMember->name) {
                search->next = new FieldChain(memName, charPtr, off);
                search = search->next;
                off += 4;
            }
        }
        else if (recordMember->kind.dec == ArrayK) {
            for (string memName : recordMember->name) {
                search->next = new FieldChain(memName, arrayType(recordMember, memName), off);
                search = search->next;
                int size;
                switch (recordMember->attr.ArrayAttr.childType) {
                case IntegerK:size = 4;
                    break;
                case CharK:size = 4;
                    break;
                }
                off += (recordMember->attr.ArrayAttr.up - recordMember->attr.ArrayAttr.low + 1) * size;
            }
        }
        else {
            cout << "line " << t->lineno << ": " << name << " :record成员类型无法识别" << endl;
            exit(1);
        }

        recordMember = recordMember->sibling.get();
    }
    search = body;
    body = body->next;
    delete search;
    return new TypeIR(RecordK, body);
}

// 作用：处理类型名引用，检查该标识符是否已声明且确实是类型标识符。
TypeIR* nameType(Treenode* t, string name) {
    SymbolsTable* symbol = FindEntry(name, 0);
    if (symbol == nullptr) {
        cout << "line " << t->lineno << ": 标识符" << name << "不存在" << endl;
        exit(1);
    }
    else if (symbol->attrIR->idKind != typeKind) {
        cout << "line " << t->lineno << ": " << name << "不是类型标识符" << endl;
        exit(1);

    }
    return symbol->attrIR->idType;
}

// 作用：统一分派类型处理逻辑，返回基本、数组、记录或类型名引用对应的 TypeIR。
TypeIR* TypeProcess(Treenode* t, DecKcate typekind, string name) {

    switch (typekind) {
    case IntegerK:return intPtr;
        break;
    case CharK:return charPtr;
        break;
    case ArrayK:return arrayType(t, name);
        break;
    case RecordK:return recordType(t, name);
        break;
    case IdK:
        return nameType(t, name);
        break;
    }
    return nullptr;
}

// 作用：处理类型声明部分，将每个新类型登记到当前作用域符号表。
void ProcessType(Treenode* t) {
    //TypeK Node :t
    TreenodePtr& tmp = t->child[0];
    Treenode* typePart=tmp.get();
    while (typePart != nullptr) {

        if (FindEntry(typePart->name[0], 1) != nullptr) {
            cout << "line " << t->lineno << ": " << typePart->name[0] << " :重复声明了一个类型" << endl;
            exit(1);
        }
        SymbolsTable* fill = Enter(typePart->name[0], new AttributeIR(TypeProcess(typePart, typePart->kind.dec, typePart->type_name),
            typeKind), scope[scope.size() - 1]);
        t->table.emplace_back(fill);
        typePart = typePart->sibling.get();
    }
}

// 作用：处理变量声明部分，将变量登记到当前作用域并分配当前层偏移。
void ProcessVar(Treenode* t) {
    //Vark Node:t

    TreenodePtr& tmp = t->child[0];
    Treenode* varPart=tmp.get();
    while (varPart) {
        for (string varName : varPart->name) {
            SymbolsTable* symbol = nullptr;
            symbol = FindEntry(varName, 1);
            if (symbol != nullptr) {
                if (symbol->attrIR->idKind != typeKind) {
                    cout << "line " << t->lineno << ": " << varName << " :重复定义" << endl;
                    exit(1);
                }
            }
            else {

                TypeIR* addTypeIR = TypeProcess(varPart, varPart->kind.dec, varPart->type_name);
                AttributeIR* addAttri = new AttributeIR(addTypeIR, varKind, dir, scope.size() - 1, Leveloff);
                SymbolsTable* fill = Enter(varName, addAttri, scope[scope.size() - 1]);
                Leveloff += addTypeIR->size;
                varPart->table.emplace_back(fill);

            }

        }
        varPart = varPart->sibling.get();
    }

}

// 作用：处理过程头部，登记过程名，创建过程作用域，并处理形参符号表项。
SymbolsTable* HeadProcess(Treenode* t) {
    string procName = t->name[0];
    if (FindEntry(procName, 1) != nullptr) {
        cout << t->lineno << " :过程" << procName << "名已存在" << endl;
        exit(1);
    }
    TypeIR* procType = nullptr;
    paramTable* procParam = new paramTable();
    AttributeIR* procAttri = new AttributeIR(procType, procKind, scope.size() - 1, nullptr);
    SymbolsTable* procSymbol = Enter(procName, procAttri, scope[scope.size() - 1]);
    paramTable* search = procParam;
    t->table.emplace_back(procSymbol);
    CreateTable();
    TreenodePtr& tmp = t->child[0];
    Treenode* paramNode=tmp.get();
    while (paramNode != nullptr) {

        int paramCnt = paramNode->name.size();
        int start = 0;
        if (paramNode->kind.dec == IdK) {
            start = 1;
        }
        /*如果参数类型是已有类型名，例如：
        MyType a, b
        解析时 name 可能是：
        name[0] = "MyType"
        name[1] = "a"
        name[2] = "b"*/
        for (int i = start; i < paramCnt; i++) {
            string paramName = paramNode->name[i];

            if (FindEntry(paramName, 1)) {
                cout << "line " << t->lineno                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             << ": " << paramName << " :????????????????????" << endl;
                exit(1);
            }

            TypeIR* addTypeIR = TypeProcess(paramNode, paramNode->kind.dec, paramNode->type_name);
            AccessKind ac = paramNode->attr.paramt == ParamTcate::valparamtype ? dir : indir;
            AttributeIR* addAttri = new AttributeIR(addTypeIR, varKind, ac, scope.size() - 1, Leveloff);
            SymbolsTable* fill = Enter(paramName, addAttri, scope[scope.size() - 1]);
            Leveloff += (ac == dir ? addTypeIR->size : 4);//值参：占实际类型大小，变参：占 4，因为保存的是地址
            t->table.emplace_back(fill);
            search->next = new paramTable();
            search = search->next;
            search->entry = fill;
        }
        paramNode = paramNode->sibling.get();
    }
    procSymbol->attrIR->More.ProcAttr.param = procParam->next;
    delete procParam;
    return procSymbol;
}

// 前置声明：statement 与 Expr 在后续语义检查中会相互间接调用。
void statement(Treenode* t);

// 前置声明：表达式类型检查函数，供数组和记录成员访问检查使用。
TypeIR* Expr(Treenode* t);

// 作用：检查数组变量访问是否合法，并返回数组元素类型。
TypeIR* arrayVar(Treenode* t) {
    SymbolsTable* symbol = FindEntry(t->name[0], 0);
    t->table.emplace_back(symbol);
    if (symbol == nullptr) {
        cout << "line " << t->lineno << " : 标识符" << t->name[0] << "未定义" << endl;
        exit(1);
    }
    if (symbol->attrIR->idKind != varKind) {
        cout << "line " << t->lineno << " : " << t->name[0] << "不是变量" << endl;
        exit(1);
    }
    if (symbol->attrIR->idType->typeKind != ArrayK) {
        cout << "line " << t->lineno << " : " << t->name[0] << "不是数组" << endl;
        exit(1);
    }
    // 比较的是“数组元素类型”和“下标表达式类型”。
    if (symbol->attrIR->idType->More.ArrayAttr.elemTy != Expr(t->child[0].get())) {
        cout << "line " << t->lineno << " :" << t->name[0] << "下标类型不符" << endl;
        exit(1);
    }
    return symbol->attrIR->idType->More.ArrayAttr.elemTy;
}

// 作用：在记录类型的域链表中查找指定域名。
FieldChain* FindField(string idname, FieldChain* head) {
    while (head && idname != head->idName) {
        head = head->next;
    }
    return head != nullptr ? head : nullptr;
}

// 作用：检查记录变量成员访问是否合法，并返回成员域的类型。
TypeIR* recordVar(Treenode* t) {
    SymbolsTable* symbol = FindEntry(t->name[0], 0);
    t->table.emplace_back(symbol);
    if (symbol == nullptr) {
        cout << "line " << t->lineno << " : 标识符" << t->name[0] << "未定义" << endl;
        exit(1);
    }
    if (symbol->attrIR->idKind != varKind) {
        cout << "line " << t->lineno << " : " << t->name[0] << "不是变量" << endl;
        exit(1);
    }
    if (symbol->attrIR->idType->typeKind != RecordK) {
        cout << "line " << t->lineno << " : " << t->name[0] << "不是记录变量" << endl;
        exit(1);
    }
    string fieldName = t->child[0]->name[0];
    FieldChain* fieldType = FindField(fieldName, symbol->attrIR->idType->More.body);
    if (fieldType == nullptr) {
        cout << "line " << t->lineno << " :" << t->name[0] << "域名" << fieldName << "不存在" << endl;
        exit(1);
    }
    return fieldType->unitType;
}

// 作用：对表达式做语义检查，推导并返回表达式类型。
TypeIR* Expr(Treenode* t) {
    
    switch (t->kind.exp) {
    case OpK:
        TypeIR* Eptr;
        if (Expr(t->child[0].get()) != Expr(t->child[1].get())) { //递归检查左右两个子表达式的类型
            cout << "line " << t->lineno << " :" << "表达式类型不兼容" << endl;
            exit(1);
        }
        switch (t->attr.ExpAttr.op) {
        case LT:
        case EQ:
            Eptr = boolPtr;
            break;  
        case PLUS:
        case MINUS:
        case TIMES:
        case OVER:
            Eptr = intPtr;
            break;  
        }
        return Eptr;
        break;
    case ConstK:return intPtr;
        break;
    case IdEK:
        if (t->child[0] != nullptr) {
            //array OR record
            if (t->attr.ExpAttr.varkind == Varkind::ArrayMembV) {
                return arrayVar(t);
            }
            else if (t->attr.ExpAttr.varkind == Varkind::FieldMembV) {
                return recordVar(t);
            }
            else {
                cout << "line " << t->lineno << " :" << t->name[0] << "是非法的成员访问类型" << endl;
                exit(1);
            }
        }
        else {
            SymbolsTable* symbol = FindEntry(t->name[0], 0);
            if (symbol == nullptr) {
                cout << "line " << t->lineno << " : " << t->name[0] << "未声明的标识符" << endl;
                exit(1);
            }
            if (symbol->attrIR->idKind != varKind) {
                cout << "line " << t->lineno << " : " << t->name[0] << "标识符不是变量" << endl;
                exit(1);
            }
            
            t->table.emplace_back(symbol);

            return symbol->attrIR->idType;
        }
        break;
    }
    return nullptr;
}

// 作用：检查 if 语句条件类型，并递归检查 then/else 分支语句。
void IfStatement(Treenode* t) {
    // Stmt NODE:t
    TreenodePtr& conditionNode = t->child[0];
    TreenodePtr& tmp1 = t->child[1];
    Treenode* thenNode=tmp1.get();
    TreenodePtr& tmp2 = t->child[2];
    Treenode* elseNode=tmp2.get();
    TypeIR* conditionType = Expr(conditionNode.get());
    if (conditionType->typeKind != BoolK) {
        cout << "line " << t->lineno << " :" << "if语句判断条件不是bool类型" << endl;
        exit(1);
    }
    while (thenNode) {
        statement(thenNode);
        thenNode = thenNode->sibling.get();
    }
    while (elseNode) {
        statement(elseNode);
        elseNode = elseNode->sibling.get();
    }
}

// 作用：检查 while 语句条件类型，并递归检查循环体语句。
void WhileStatement(Treenode* t) {
    //WhileK Node:t
    TreenodePtr& conditionNode = t->child[0];
    TreenodePtr& tmp = t->child[1];
    TypeIR* conditionType = Expr(conditionNode.get());
    Treenode* loopNode=tmp.get();
    if (conditionType->typeKind != BoolK) {
        cout << "line " << t->lineno << " :" << "while语句判断条件不是bool类型" << endl;
        exit(1);
    }
    while (loopNode) {
        statement(loopNode);
        loopNode = loopNode->sibling.get();
    }
}

// 作用：检查赋值语句左值是否合法，并保证左右两侧类型一致。
void AssignStatement(Treenode* t) {
    TreenodePtr& child1 = t->child[0];
    TreenodePtr& child2 = t->child[1];
    TypeIR* eptr1, * eptr2;
    SymbolsTable* child1Symbol = FindEntry(child1->name[0], 0);
    if (child1Symbol == nullptr) {
        cout << "line " << t->lineno << " :左标识符" << child1->name[0] << "未找到" << endl;
        exit(1);
    }
    if (child1Symbol->attrIR->idKind != varKind) {
        cout << "line " << t->lineno << " :左值不是变量" << endl;
        exit(1);
    }

    if (child1->kind.exp == IdEK) {
        if (child1->child[0] != nullptr) {
            if (child1->attr.ExpAttr.varkind == Varkind::ArrayMembV) {
                eptr1 = arrayVar(child1.get());
            }
            else if (child1->attr.ExpAttr.varkind == Varkind::FieldMembV) {
                eptr1 = recordVar(child1.get());
            }
        }
        else
        {
            child1->table.emplace_back(child1Symbol);
            //SymTable *symbol= FindEntry(child1->name[0],0);
            if (child1Symbol == nullptr) {
                cout << "line " << child1->lineno << " : 嵌套标识符" << child1->name[0] << "未找到" << endl;
                exit(1);

            }
            if (child1Symbol->attrIR->idKind != varKind) {
                cout << "line " << child1->lineno << " : " << child1->name[0] << "不是变量" << endl;
                exit(1);
            }
            eptr1 = child1Symbol->attrIR->idType;
        }
    }
    else {
        cout << "line " << t->lineno << " :非法的赋值左值表达式" << endl;
        exit(1);
    }
    eptr2 = Expr(child2.get());
    if (eptr1 != eptr2) {
        cout << "line " << t->lineno << " : " << "赋值类型不匹配" << endl;
        exit(1);
    }
}

// 作用：检查 read 语句读取对象是否已声明且为变量。
void ReadStatement(Treenode* t) {
    // ReadK Node:t
    SymbolsTable* symbol = FindEntry(t->name[0], 0);
    if (symbol == nullptr) {
        cout << "line " << t->lineno << " :读入的" << t->name[0] << "未声明" << endl;
        exit(1);

    }
    if (symbol->attrIR->idKind != varKind) {
        cout << "line " << t->lineno << " :read读入对象" << t->name[0] << "不是变量" << endl;
        exit(1);
    }
    t->table.emplace_back(symbol);
}

// 作用：检查 write 语句输出表达式类型是否为 integer 或 char。
void WriteStatement(Treenode* t) {
    //WriteK Node:t
    TypeIR* writeType = Expr(t->child[0].get());
    if (writeType != charPtr && writeType != intPtr) {
        cout << "line " << t->lineno << " :" << "write语句表达式类型必须是integer或char" << endl;
        exit(1);
    }
}

// 作用：检查过程调用是否合法，包括过程名存在性和实参与形参类型、个数匹配。
void CallStatement(Treenode* t) {
    //CallK Node:t
    string procName = t->child[0]->name[0];
    SymbolsTable* symbol = FindEntry(procName, 0);
    if (symbol == nullptr) {
        cout << "line " << t->lineno << " : 过程名" << procName << "未声明" << endl;
        exit(1);
    }
    if (symbol->attrIR->idKind != procKind) {
        cout << "line " << t->lineno << " : " << procName << "不是过程名" << endl;
        exit(1);
    }
    t->table.emplace_back(symbol);
    
    TreenodePtr& tmp = t->child[1];
    Treenode* param=tmp.get();
    
    ParamTable* procParam = symbol->attrIR->More.ProcAttr.param;
    while (param != nullptr && procParam != nullptr) {
        TypeIR* callParamType = Expr(param);
        if (callParamType->typeKind == ArrayK) {
            if (procParam->entry->attrIR->idType->typeKind != ArrayK) {
                cout << "line " << t->lineno << " : " << procName << "数组类型与普通类型不匹配" << endl;
                exit(1);
            }
            if (procParam->entry->attrIR->idType->More.ArrayAttr.elemTy != callParamType->More.ArrayAttr.elemTy) {
                cout << "line " << t->lineno << " : " << procName << "数组元素类型不匹配" << endl;
                exit(1);
            }
            if ((procParam->entry->attrIR->idType->More.ArrayAttr.up != callParamType->More.ArrayAttr.up) || (procParam->entry->attrIR->idType->More.ArrayAttr.low != callParamType->More.ArrayAttr.low)) {
                cout << "line " << t->lineno << " : " << procName << "数组上下界不匹配" << endl;
                exit(1);
            }
        }
        else if (callParamType != procParam->entry->attrIR->idType) {
            cout << "line " << t->lineno << " : " << procName << "普通类型不匹配" << endl;
            exit(1);
        }

        procParam = procParam->next;
        param = param->sibling.get();
    }
    if (param != nullptr || procParam != nullptr) {
        cout << "line " << t->lineno << " : " << procName << "参数个数不匹配" << endl;
        exit(1);
    }
}

// 作用：检查 return 语句是否合法，当前实现禁止在主程序中使用 return。
void ReturnStatement(Treenode* t) {
    if (scope.size() == 1) {
        cout << "line " << t->lineno<< " : " << "主程序中不能使用return语句" << endl;
        exit(1);
    }
}

// 作用：根据语句种类分派到对应的语义检查函数。
void statement(Treenode* t) {
    //StmtK NODE:t
    switch (t->kind.stmt) {
    case IfK:
        IfStatement(t);
        break;
    case WhileK:
        WhileStatement(t);
        break;
    case AssignK:
        AssignStatement(t);
        break;
    case ReadK:
        ReadStatement(t);
        break;
    case WriteK:
        WriteStatement(t);
        break;
    case CallK:
        CallStatement(t);
        break;
    case ReturnK:
        ReturnStatement(t);
        break;
    default:
        cout << "line " << t->lineno << " :非法的语句类型" << endl;
        exit(1);
        break;
    }
}

// 作用：遍历语句序列节点，逐条进行语义检查。
void Body(Treenode* t) {
    //StmLK NODE: t
    TreenodePtr& tmp = t->child[0];
    Treenode* stmtList=tmp.get();
    while (stmtList != nullptr) {

        statement(stmtList);
        stmtList = stmtList->sibling.get();
    }
}

// 作用：退出当前作用域，销毁作用域栈顶的符号表。
void DestroyTable() {
    SymbolsTable* search = scope.back();
    delete search;
    scope.pop_back();

}


// 作用：处理过程声明链，包括局部声明、嵌套过程、过程体和作用域回收。
void ProcessProc(Treenode* t) {
    //ProcDeck Node:t
    int Savedoff = 0;
    SymbolsTable* proc = nullptr;
    while (t != nullptr) {
        proc = HeadProcess(t);
        // 保存处理完形参后的当前层偏移；若本过程没有局部变量，这就是最终局部空间大小。
        Savedoff = Leveloff;
        TreenodePtr& tmp1 = t->child[1];
        TreenodePtr& tmp2 = t->child[2];
        Treenode* DecNode=tmp1.get();
        Treenode* BodyNode=tmp2.get();
        if (DecNode != nullptr && DecNode->nodekind == TypeK) {
            ProcessType(DecNode);
            DecNode = DecNode->sibling.get();
        }
        if (DecNode != nullptr && DecNode->nodekind == VarK) {
            ProcessVar(DecNode);
            // 局部变量会继续增加 Leveloff，因此这里更新为“形参 + 局部变量”后的偏移。
            // 后续递归处理子过程可能修改全局 Leveloff，所以父过程的偏移要先保存下来。
            Savedoff = Leveloff;
            DecNode = DecNode->sibling.get();
        }
        if (DecNode != nullptr && DecNode->nodekind == ProcDecK) {
            ProcessProc(DecNode);
            DecNode = DecNode->sibling.get();
        }

        if (proc != nullptr) {
            proc->attrIR->More.ProcAttr.off = Savedoff;
            proc->attrIR->More.ProcAttr.moff = Savedoff + (scope.size() - 1) * 4;
        }
        Body(BodyNode);
        DestroyTable();
        t = t->sibling.get();
    }
}

// 作用：将标识符种类枚举转换为便于打印的字符串。
string toIdKind(int enumId) {
    switch (enumId) {
    case typeKind:return "typeKind";
    case varKind:return "varKind";
    case procKind:return "procKind";
    default:cout << "IdKind异常，不存在类型，变量，过程之外的kind，返回空字符" << endl;
        exit(1);
    }
    return "";
}

// 作用：将变量访问方式枚举转换为便于打印的字符串。
string toAccessKind(int enumId) {
    if (enumId == indir) {
        return "indir";
    }
    else if (enumId == dir) {
        return "dir";
    }
    else {
        cout << "访问方式异常，不存在indir和dir之外的访问" << endl;
        exit(1);
    }
    return "";
}

// 作用：将类型种类枚举转换为便于打印的字符串。
string toTypeKind(int enumId) {
    switch (enumId) {
    case IntegerK:return "IntegerK";
    case CharK:return "CharK";
    case ArrayK:return "ArrayK";
    case RecordK:return "RecordK";
        //case boolTy:return "boolTy";
    default:cout << "TypeKind异常，不存在int,char,array,record，bool之外的类型,返回空字符" << endl;
        exit(1);
    }
    return "";
}

// 作用：遍历语法树并打印各节点关联的符号表信息。
void PrintSymTable(Treenode* t) {
    while (t != nullptr) {
        if (t->table.size() != 0) {

            for (SymbolsTable* symbol : t->table) {
                if (symbol->attrIR->idKind == procKind) {
                    printf("==============   Level %d:             %s   %s   %d   ACSIZE=%d\n", symbol->attrIR->More.ProcAttr.level, symbol->idName.c_str(), toIdKind(symbol->attrIR->idKind).c_str(), symbol->attrIR->More.ProcAttr.off, symbol->attrIR->More.ProcAttr.moff);

                }
                else if (symbol->attrIR->idKind == varKind) {
                    printf("==============   Level %d:             %s   %s   %s   %d\n", symbol->attrIR->More.VarAttr.level, symbol->idName.c_str(), toAccessKind(symbol->attrIR->More.VarAttr.access).c_str(), toIdKind(symbol->attrIR->idKind).c_str(), symbol->attrIR->More.VarAttr.off);
                }
                else {
                    printf("==============   TypeKind:             %s   %s  %s                               \n", symbol->idName.c_str(), toIdKind(symbol->attrIR->idKind).c_str(), toTypeKind(symbol->attrIR->idType->typeKind).c_str());

                }
            }
        }
        for (int i = 0; i < 3; i++)
        {
            PrintSymTable(t->child[i].get());
        }
        t = t->sibling.get();
    }
}

// 作用：语义分析入口，初始化全局类型和符号表，处理主程序声明与语句体。
TreenodePtr semanticAnalyze(TreenodePtr&& t)
{
	initType();
	CreateTable();
	int Savedoff = Leveloff;
    TreenodePtr& MainProc = t->child[0];
    TreenodePtr& tmp = t->child[1];
    TreenodePtr& BodyNode = t->child[2];
    SymbolsTable* MainProSymbol = Enter(MainProc->name[0], new AttributeIR(nullptr, procKind, 0, nullptr), scope[0]);
    MainProc->table.emplace_back(MainProSymbol);
    CreateTable();
    Treenode* DecNode=tmp.get();
    if (DecNode && DecNode->nodekind == TypeK) {
    	ProcessType(DecNode);

        DecNode = DecNode->sibling.get();
    }
    if (DecNode && DecNode->nodekind == VarK) {
        ProcessVar(DecNode);
        Savedoff = Leveloff;
        DecNode = DecNode->sibling.get();
    }
    if (DecNode && DecNode->nodekind == ProcDecK) {
        ProcessProc(DecNode);

        DecNode = DecNode->sibling.get();
    }

    MainProSymbol->attrIR->More.ProcAttr.off = Savedoff;                //从何处开始存储过程体内局部变量
    MainProSymbol->attrIR->More.ProcAttr.moff = Savedoff + (scope.size() - 1) * 4;  //为后续递归或者嵌套调用需要留出的栈空间，除主过程之外的嵌套层数，每个子程序预留4单位空间
    Body(BodyNode.get());

    printf("\n\n");
    PrintSymTable(t.get());
    return move(t);
}
