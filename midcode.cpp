#include "midcode.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

using std::ofstream;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

namespace {

const string kNullArg = "NULL";

class MidCodeGenerator {
public:
    vector<MidCode> generate(const Treenode* root) {
        codes.clear();
        procLabels.clear();
        fallbackLabels.clear();
        tempCount = 0;
        labelCount = 0;

        collectProcedureLabels(root);
        genProgram(root);
        return codes;
    }

private:
    vector<MidCode> codes;
    unordered_map<const SymbolsTable*, string> procLabels;
    unordered_map<string, string> fallbackLabels;
    int tempCount = 0;
    int labelCount = 0;

    void emit(const string& op,
              const string& arg1 = kNullArg,
              const string& arg2 = kNullArg,
              const string& result = kNullArg) {
        codes.push_back({op, normalize(arg1), normalize(arg2), normalize(result)});
    }

    static string normalize(const string& value) {
        return value.empty() ? kNullArg : value;
    }

    string newTemp() {
        ++tempCount;
        return "temp" + to_string(tempCount);
    }

    string newLabel(const string& hint = "") {
        ++labelCount;
        if (hint.empty()) {
            return "L" + to_string(labelCount);
        }
        return "L" + to_string(labelCount) + "_" + hint;
    }

    static string firstName(const Treenode* node, const string& fallback = "") {
        if (node != nullptr && !node->name.empty()) {
            return node->name[0];
        }
        return fallback;
    }

    static const SymbolsTable* firstSymbol(const Treenode* node) {
        if (node == nullptr || node->table.empty()) {
            return nullptr;
        }
        return node->table[0];
    }

    static const TypeIR* symbolType(const Treenode* node) {
        const SymbolsTable* symbol = firstSymbol(node);
        if (symbol == nullptr || symbol->attrIR == nullptr) {
            return nullptr;
        }
        return symbol->attrIR->idType;
    }

    static string intArg(int value) {
        return to_string(value);
    }

    static int typeSize(const TypeIR* type) {
        return type != nullptr ? type->size : 4;
    }

    static const FieldChain* findField(const TypeIR* recordType, const string& fieldName) {
        if (recordType == nullptr || recordType->typeKind != RecordK) {
            return nullptr;
        }

        const FieldChain* field = recordType->More.body;
        while (field != nullptr) {
            if (field->idName == fieldName) {
                return field;
            }
            field = field->next;
        }
        return nullptr;
    }

    static string frameSize(const SymbolsTable* symbol) {
        if (symbol == nullptr || symbol->attrIR == nullptr || symbol->attrIR->idKind != procKind) {
            return "0";
        }
        return intArg(symbol->attrIR->More.ProcAttr.moff);
    }

    static string procLevel(const SymbolsTable* symbol) {
        if (symbol == nullptr || symbol->attrIR == nullptr || symbol->attrIR->idKind != procKind) {
            return "0";
        }
        return intArg(symbol->attrIR->More.ProcAttr.level);
    }

    string labelFor(const SymbolsTable* symbol, const string& fallbackName) {
        if (symbol != nullptr) {
            auto found = procLabels.find(symbol);
            if (found != procLabels.end()) {
                return found->second;
            }

            string label = newLabel(fallbackName);
            procLabels.emplace(symbol, label);
            return label;
        }

        auto found = fallbackLabels.find(fallbackName);
        if (found != fallbackLabels.end()) {
            return found->second;
        }

        string label = newLabel(fallbackName);
        fallbackLabels.emplace(fallbackName, label);
        return label;
    }

    void collectProcedureLabels(const Treenode* node) {
        while (node != nullptr) {
            if (node->nodekind == ProK) {
                const Treenode* head = node->child[0].get();
                labelFor(firstSymbol(head), firstName(head, "main"));
            } else if (node->nodekind == ProcDecK) {
                labelFor(firstSymbol(node), firstName(node, "proc"));
            }

            for (const auto& child : node->child) {
                collectProcedureLabels(child.get());
            }
            node = node->sibling.get();
        }
    }

    void genProgram(const Treenode* root) {
        if (root == nullptr || root->nodekind != ProK) {
            return;
        }

        genProcedureDeclarations(root->child[1].get());

        const Treenode* mainHead = root->child[0].get();
        const SymbolsTable* mainSymbol = firstSymbol(mainHead);
        emit("PENTRY", labelFor(mainSymbol, firstName(mainHead, "main")), frameSize(mainSymbol), procLevel(mainSymbol));
        genBody(root->child[2].get());
    }

    void genProcedureDeclarations(const Treenode* declNode) {
        while (declNode != nullptr) {
            if (declNode->nodekind == ProcDecK) {
                genProcedure(declNode);
            }
            declNode = declNode->sibling.get();
        }
    }

    void genProcedure(const Treenode* procNode) {
        if (procNode == nullptr) {
            return;
        }

        genProcedureDeclarations(procNode->child[1].get());

        const SymbolsTable* procSymbol = firstSymbol(procNode);
        emit("PENTRY", labelFor(procSymbol, firstName(procNode, "proc")), frameSize(procSymbol), procLevel(procSymbol));
        genBody(procNode->child[2].get());
        emit("ENDPROC");
    }

    void genBody(const Treenode* bodyNode) {
        if (bodyNode == nullptr) {
            return;
        }

        if (bodyNode->nodekind == StmLK) {
            genStmtList(bodyNode->child[0].get());
        } else if (bodyNode->nodekind == StmtK) {
            genStmtList(bodyNode);
        }
    }

    void genStmtList(const Treenode* stmtNode) {
        while (stmtNode != nullptr) {
            if (stmtNode->nodekind == StmtK) {
                genStatement(stmtNode);
            }
            stmtNode = stmtNode->sibling.get();
        }
    }

    void genStatement(const Treenode* stmtNode) {
        if (stmtNode == nullptr) {
            return;
        }

        switch (stmtNode->kind.stmt) {
        case AssignK:
            genAssign(stmtNode);
            break;
        case IfK:
            genIf(stmtNode);
            break;
        case WhileK:
            genWhile(stmtNode);
            break;
        case ReadK:
            genRead(stmtNode);
            break;
        case WriteK:
            genWrite(stmtNode);
            break;
        case CallK:
            genCall(stmtNode);
            break;
        case ReturnK:
            emit("RETURN");
            break;
        default:
            break;
        }
    }

    void genAssign(const Treenode* stmtNode) {
        string left = genVariable(stmtNode->child[0].get());
        string right = genExpr(stmtNode->child[1].get());
        emit("ASSIG", right, left);
    }

    void genIf(const Treenode* stmtNode) {
        string elseLabel = newLabel("else");
        string outLabel = newLabel("endif");
        string condition = genExpr(stmtNode->child[0].get());

        emit("JUMP0", condition, elseLabel);
        genStmtList(stmtNode->child[1].get());
        emit("JUMP", outLabel);
        emit("LABEL", elseLabel);
        genStmtList(stmtNode->child[2].get());
        emit("LABEL", outLabel);
    }

    void genWhile(const Treenode* stmtNode) {
        string startLabel = newLabel("while");
        string outLabel = newLabel("endwhile");

        emit("LABEL", startLabel);
        string condition = genExpr(stmtNode->child[0].get());
        emit("JUMP0", condition, outLabel);
        genStmtList(stmtNode->child[1].get());
        emit("JUMP", startLabel);
        emit("LABEL", outLabel);
    }

    void genRead(const Treenode* stmtNode) {
        string target = firstName(stmtNode);
        emit("READC", target);
    }

    void genWrite(const Treenode* stmtNode) {
        string value = genExpr(stmtNode->child[0].get());
        emit("WRITEC", value);
    }

    void genCall(const Treenode* stmtNode) {
        const Treenode* procNameNode = stmtNode->child[0].get();
        const SymbolsTable* procSymbol = firstSymbol(stmtNode);
        ParamTable* formal = nullptr;
        if (procSymbol != nullptr && procSymbol->attrIR != nullptr && procSymbol->attrIR->idKind == procKind) {
            formal = procSymbol->attrIR->More.ProcAttr.param;
        }

        const Treenode* actual = stmtNode->child[1].get();
        int paramIndex = 0;
        while (actual != nullptr) {
            string actualArg = genExpr(actual);
            string offset = intArg(paramIndex * 4);
            string size = "4";
            bool byRef = false;

            if (formal != nullptr && formal->entry != nullptr && formal->entry->attrIR != nullptr) {
                offset = intArg(formal->entry->attrIR->More.VarAttr.off);
                size = intArg(typeSize(formal->entry->attrIR->idType));
                byRef = formal->entry->attrIR->More.VarAttr.access == indir;
                formal = formal->next;
            }

            emit(byRef ? "VARACT" : "VALACT", actualArg, offset, size);
            ++paramIndex;
            actual = actual->sibling.get();
        }

        emit("CALL",
             labelFor(procSymbol, firstName(procNameNode, "proc")),
             frameSize(procSymbol),
             procLevel(procSymbol));
    }

    string genExpr(const Treenode* exprNode) {
        if (exprNode == nullptr) {
            return kNullArg;
        }

        switch (exprNode->kind.exp) {
        case ConstK:
            return intArg(exprNode->attr.ExpAttr.val);
        case IdEK:
            return genVariable(exprNode);
        case OpK:
            return genOp(exprNode);
        default:
            return kNullArg;
        }
    }

    string genOp(const Treenode* exprNode) {
        string left = genExpr(exprNode->child[0].get());
        string right = genExpr(exprNode->child[1].get());
        string temp = newTemp();
        emit(opName(exprNode->attr.ExpAttr.op), left, right, temp);
        return temp;
    }

    static string opName(LexType op) {
        switch (op) {
        case PLUS:
            return "ADD";
        case MINUS:
            return "SUB";
        case TIMES:
            return "MULT";
        case OVER:
            return "DIV";
        case LT:
            return "LT";
        case EQ:
            return "EQ";
        default:
            return "OP";
        }
    }

    string genVariable(const Treenode* varNode) {
        if (varNode == nullptr) {
            return kNullArg;
        }

        if (!varNode->child[0]) {
            return firstName(varNode);
        }

        if (varNode->attr.ExpAttr.varkind == Varkind::ArrayMembV) {
            return genArrayVariable(varNode);
        }

        if (varNode->attr.ExpAttr.varkind == Varkind::FieldMembV) {
            return genFieldVariable(varNode);
        }

        return firstName(varNode);
    }

    string genArrayVariable(const Treenode* varNode) {
        const TypeIR* arrayType = symbolType(varNode);
        int low = 0;
        int elemSize = 4;

        if (arrayType != nullptr && arrayType->typeKind == ArrayK) {
            low = arrayType->More.ArrayAttr.low;
            elemSize = typeSize(arrayType->More.ArrayAttr.elemTy);
        }

        return genArrayAddress(firstName(varNode), varNode->child[0].get(), low, elemSize);
    }

    string genFieldVariable(const Treenode* varNode) {
        const Treenode* fieldNode = varNode->child[0].get();
        string fieldName = firstName(fieldNode);
        const FieldChain* field = findField(symbolType(varNode), fieldName);
        int fieldOffset = field != nullptr ? field->off : 0;

        string fieldBase = newTemp();
        emit("AADD", firstName(varNode), intArg(fieldOffset), fieldBase);

        if (fieldNode != nullptr && fieldNode->child[0]) {
            const TypeIR* fieldType = field != nullptr ? field->unitType : nullptr;
            int low = 0;
            int elemSize = 4;
            if (fieldType != nullptr && fieldType->typeKind == ArrayK) {
                low = fieldType->More.ArrayAttr.low;
                elemSize = typeSize(fieldType->More.ArrayAttr.elemTy);
            }
            return genArrayAddress(fieldBase, fieldNode->child[0].get(), low, elemSize);
        }

        return fieldBase;
    }

    string genArrayAddress(const string& baseArg, const Treenode* indexNode, int low, int elemSize) {
        string indexArg = genExpr(indexNode);
        string shiftedIndex = newTemp();
        string byteOffset = newTemp();
        string address = newTemp();

        emit("SUB", indexArg, intArg(low), shiftedIndex);
        emit("MULT", shiftedIndex, intArg(elemSize), byteOffset);
        emit("AADD", baseArg, byteOffset, address);
        return address;
    }
};

} // namespace

vector<MidCode> buildMidCode(const Treenode* root) {
    MidCodeGenerator generator;
    return generator.generate(root);
}

void writeMidCode(const vector<MidCode>& codes, const string& outputPath) {
    ofstream out(outputPath);
    if (!out.is_open()) {
        return;
    }

    out << std::left << std::setw(6) << "No"
        << std::setw(12) << "Op"
        << std::setw(18) << "Arg1"
        << std::setw(18) << "Arg2"
        << std::setw(18) << "Result" << '\n';

    for (size_t i = 0; i < codes.size(); ++i) {
        out << std::left << std::setw(6) << i
            << std::setw(12) << codes[i].op
            << std::setw(18) << codes[i].arg1
            << std::setw(18) << codes[i].arg2
            << std::setw(18) << codes[i].result << '\n';
    }
}

void generateMidCode(const Treenode* root, const string& outputPath) {
    writeMidCode(buildMidCode(root), outputPath);
}
