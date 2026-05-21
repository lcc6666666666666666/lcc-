#include "codegen.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#define CODESEG ".text"
#define DATASEG ".data"
#define SP "$sp"
#define T0 "$t0"
#define T1 "$t1"
#define T2 "$t2"
#define T3 "$t3"
#define T4 "$t4"
#define T5 "$t5"
#define T6 "$t6"
#define T7 "$t7"
#define A0 "$a0"
#define A1 "$a1"
#define A2 "$a2"
#define A3 "$a3"
#define ZERO "$0"
#define RA "$ra"
#define V0 "$v0"
#define V1 "$v1"
#define READ 5
#define WRITESTR 4
#define WRITEINT 1
#define EXIT 10
vector<string> mipsCode;
int curLine=0;
int highLine=0; //记录最新行索引
int curAddr = 0x00400000;//相对偏移
int highAddr = 0x00400000;//最新地址

/* 功能：在目标代码中预留num条指令位置，用于之后回填跳转指令等代码。 */
int  emitSkip(int num) {
    int i = curLine;
    while (num != 0) {
        mipsCode.emplace_back("");
        num--;
        curLine++;
        curAddr += 4;
    }
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return i;
}

/* 功能：向目标MIPS代码中写入一行注释，不增加目标指令地址。 */
void emitComment(string c) {
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    mipsCode[curLine] = "#" + c;
    curLine++;
    highLine = max(curLine, highLine);
}

/* 功能：生成一个指定名称的MIPS标签，并返回该标签所在代码行。 */
int genLabel(string label) {
    string mips = label + ":";
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;
}

/* 功能：生成寄存器-内存格式指令，如 lw/sw reg, offset(base)。 */
int objMips_RM(string op, string rs, string rt, int immediate, string comment) {
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + rs + ", " + to_string(immediate) + "(" + rt + ")" + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;//返回生成的代码行
}

/* 功能：临时把代码生成位置回退到指定行号和地址，供回填使用。 */
void emitBackup(int loc, int addr) {
    curAddr = addr;
    curLine = loc;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
}

/* 功能：生成无条件跳转类指令，如 j、jal、jr。 */
int objMips_UnConj(string op, string target, string comment) {
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + target + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;//返回生成的代码行
}

/* 功能：回填完成后恢复到目前生成过的最高代码位置。 */
void emitRestore() {
    curAddr = highAddr;
    curLine = highLine;
}

/* 功能：生成两个操作数的寄存器指令，如 move、li、mflo 等。 */
int objMips_Regs(string op, string rd, string rs, string comment)
{
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + rd + ", " + rs + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;
}

/* 功能：生成寄存器和立即数参与的三操作数指令，如 addi rd, rs, imm。 */
int objMips_RegIm(string op, string rd, string rs, string immediate, string comment)
{
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + rd + ", " + rs + ", " + immediate + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;
}

/* 功能：生成三个寄存器操作数的指令，如 add/sub rd, rs, rt。 */
int objMips_Regs(string op, string rd, string rs, string rt, string comment)
{
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + rd + ", " + rs + ", " + rt + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;
}

/* 功能：生成单寄存器操作数指令，如 mflo reg 或部分伪指令。 */
int objMips_Regs(string op, string rs, string comment)
{
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + rs + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;
}

/*返回值t0，表示所要访问活动记录的基址*/
/* 功能：根据变量所在层号，通过display表找到对应活动记录基址，结果放入$t0。 */
void getARBase(int varLevel) {
    /*var变量要对对应level-1的过程活动记录找！！！！！！！！*/
    /*先假定 t0是存放display表相对应基址的偏移的吧, 假定t2存当前的基址*/
    /*先求该变量层数在AR中的位置，其中varLevel表示变量所在层*/
    /*t4存当前活动记录的基址*/
    //lw $t2,24($t4)  //把display偏移值放到t2里，4*6
    objMips_RM("lw", "$t0", "$t4", -24, "#把display偏移值放到t0里，4*6");
    //addi $t1,$t2,varLevel*4 //找到display里的记录相对于基址的偏移
    objMips_RegIm("addi", "$t1", "$t0", to_string((varLevel - 1) * 4), "#找到display里的记录相对于基址的偏移");
    /*在求一下相对于基址的绝对偏移,现在t1里存放的就是varLevel层的AR的基址,是个地址*/
    //sub $t1,$t4,$t2 //求出记录的绝对偏移,栈是倒长的
    objMips_Regs("sub", "$t1", "$t4", "$t1", "#求出记录的绝对偏移,栈是倒长的");
    /*该变量所在AR的sp地址存在t1中，将t1地址里的数据取出来*/
    //lw $t0,0($t1); //取出记录里的内容
    objMips_RM("lw", "$t0", "$t1", 0, "#取出记录里的内容");
}

//这个函数里t0装要访问数组元素的下标，t1装数组下界
/* 功能：计算变量、数组元素或记录域的绝对地址，结果放入$t0。 */
void findAbsAddr(Treenode* t) {
    int off;
    int varLevel;
    fieldChain* field = nullptr;
    /*可能是普通变量，域变量和数组变量*/
    if (t) {
        /*得到该变量在符号表中的地址，即变量相对于自己AR基址的偏移*/
        off = t->table[0]->attrIR->More.VarAttr.off;
        varLevel = t->table[0]->attrIR->More.VarAttr.level;
        /*此时要取数组第一个元素的地址*/
        /*如果是普通变量*/
        if (t->child[0] == nullptr) {

            //addi $t0,$zero,off //t0里存相对于自己AR基址的偏移
            objMips_RegIm("addi", "$t0", "$zero", to_string(off), "#t0里存相对于自己AR基址的偏移");
        }
        else if (t->attr.ExpAttr.varkind == Varkind::ArrayMembV) {
            /*将数组下标值送入t0中*/
            cGen(t->child[0].get());
            /*将数组下界送入t1*/
            int low = t->table[0]->attrIR->idType->More.ArrayAttr.low;

            //addi $t1,$zero,low      //t1为数组下界
            objMips_RegIm("addi", "$t1", "$zero", to_string(low), "#t1为数组下界");

            //sub $t0,$t0,$t1        //下标减去下界
            objMips_Regs("sub", "$t0", "$t0", "$t1", "#下标减去下界");

            //addi $t0,$t0,off       //求出数组变量的偏移，即数组里的这个元素相对于基址的偏移
            objMips_Regs("li", T1, to_string(4), "地址偏移乘4");
            objMips_Regs("mult", T0, T1, "乘4的偏移");
            objMips_Regs("mflo", T0, "将低32为载入T0");
            objMips_RegIm("addi", "$t0", "$t0", to_string(off), "#求出数组变量的偏移，即数组里的这个元素相对于基址的偏移");
        }
        else if (t->attr.ExpAttr.varkind == Varkind::FieldMembV) {
            field = t->table[0]->attrIR->idType->More.body;
            while (field) {
                int result = t->child[0]->name[0].compare(field->idName);
                if (result == 0) break;
                else field = field->next;
            }
            if (t->child[0]->child[0] == nullptr) {
                //addi $t0,$zero,off //这里是域基址相对于自己AR的偏移
                objMips_RegIm("addi", "$t0", "$zero", to_string(off), "#这里是域基址相对于自己AR的偏移");
                int foff = field->off;
                //addi $t0,$t0,foff  //域基址相对于自己AR的偏移和自己相对于域基址的偏移
                objMips_RegIm("addi", "$t0", "$t0", to_string(foff), "#域基址相对于自己AR的偏移和自己相对于域基址的偏移");
            }
            else {
                //数组元素下标放入t0
                genExp(t->child[0]->child[0].get());
                //将数组下标送入t1吧
                int low = t->table[0]->attrIR->idType->More.ArrayAttr.low;
                //addi $t1,$zero,low   //t1为数组下界
                objMips_RegIm("addi", "$t1", "$zero", to_string(low), "#t1为数组下界");
                //sub $t0,$t0,$t1      //下标减去下界
                objMips_Regs("sub", "$t0", "$t0", "$t1", "#下标减去下界");
                objMips_Regs("li", T1, to_string(4), "地址偏移乘4");
                objMips_Regs("mult", T0, T1, "乘4的偏移");
                int foff = field->off;
                //addi $t0,$t0,foff  //数组元素相对于域基址的相对地址
                objMips_RegIm("addi", "$t0", "$t0", to_string(foff), "#数组元素相对于域基址的相对地址");
                //addi $t0,$t0,off   //数组元素相对于AR基址的相对地址（偏移）
                objMips_RegIm("addi", "$t0", "$t0", to_string(off), "#数组元素相对于AR基址的相对地址（偏移）");
            }
        }
    }
    //add $t6,$t0,$zero //先把t0的值传到t6里，防止后面冲突
    objMips_Regs("add", "$t6", "$t0", "$zero", "#先把t0的值传到t6里，防止后面冲突");
    getARBase(varLevel); 	//基址存在t0里
    //sub $t0,$t0,$t6  	//计算绝对偏移，t0里存的是绝对地址
    objMips_Regs("sub", "$t0", "$t0", "$t6", "#计算绝对偏移，t0里存的是绝对地址");
}

int labelId=0;
/* 功能：提前构造即将生成的标签名，不写入目标代码，供跳转指令引用。 */
string prepare_label(string kind = "", int beforeLabel = 0) {
    string ret = "label_";
    ret += to_string(labelId + beforeLabel);
    ret += "_" + kind;
    return ret;
}

/* 功能：生成条件跳转指令，如 beq/blt rs, rt, label。 */
int objMips_Conj(string op, string rs, string rt, string tag, string comment) {
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string mips = op + " " + rs + ", " + rt + ", " + tag + " #" + comment;
    mipsCode[curLine] = mips;
    curLine++;
    curAddr += 4;
    highLine = max(curLine, highLine);
    highAddr = max(curAddr, highAddr);
    return curLine - 1;//返回生成的代码行
}

/* 功能：生成带全局递增编号的唯一标签，并返回标签名。 */
string emitLabel(string kind = "") {
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    string ret = "label_";
    ret += to_string(labelId++);
    ret += "_" + kind;
    mipsCode[curLine] = ret + ":";
    curLine++;
    highLine = max(curLine, highLine);
    return ret;
}

/* 功能：为表达式节点生成MIPS代码，表达式计算结果统一放入$t0。 */
void genExp(Treenode* t)
{
    /* 语法树节点各个子节点 */
    Treenode* p1, * p2;
    /* 对语法树节点的表达式类型细分处理 */
    switch (t->kind.exp)
    {
        /* 语法树节点tree为ConstK表达式类型 */
    case ConstK:
        objMips_Regs("li", T0, to_string(t->attr.ExpAttr.val), "载入常量到T0");
        /* 如果代码生成追踪标志TraceCode为TRUE,写入注释,常数部分结束 */
        break;
        /* 语法树节点tree为IdK表达式类型 */
    case IdEK:
        /* 如果代码生成追踪标志TraceCode为TRUE,写入注释,标注标识符开始 */
        findAbsAddr(t);
        /*其中ac返回的是基本类型变量、域变量或下标变量的绝对偏移*/

        if (t->table[0]->attrIR->More.VarAttr.access == indir)
        {
            /*地址*/
            /*取值，作为地址*/
            objMips_RM("lw", T1, T0, 0, "indir load id value");
            /*按地址取单元内容*/
            objMips_RM("lw", T0, T1, 0, "");
        }
        else
        {
            /*值*/
            /* 写入数值载入指令,载入变量标识符的值*/
            objMips_RM("lw", T0, T0, 0, "load id value");
        }
        break;
        /* 语法树节点tree为OpK表达式类型 */
    case OpK:
        /* 语法树节点tree第一子节点为左操作数,赋给p1 */
        p1 = t->child[0].get();
        /* 语法树节点tree第二子节点为右操作数,赋给p2 */
        p2 = t->child[1].get();
        /* 对第一子节点递归调用函数cGen(),为左操作数生成目标代码 */
        cGen(p1);
        /* 生成单元设置指令,在临时数据存储区中压入左操作数 */
        objMips_RM("sw", T0, SP, 0, "存临时数据");
        objMips_Regs("li", T7, "4", "");
        objMips_Regs("sub", SP, SP, T7, "sp栈顶向下-4");
        /* 对第二子节点递归调用函数cGen(),为右操作数生成目标代码 */
        cGen(p2);
        objMips_RM("lw", T1, SP, 4, "载入数据");
        objMips_Regs("li", T7, "4", "");
        objMips_Regs("add", SP, SP, T7, "sp栈顶向上+4恢复");
        /* 生成数值载入指令,从临时数据存储区中载入左操作数 */
        /*交换一下*/
        objMips_Regs("move", T7, T1, "把t1放到t7里");
        objMips_Regs("move", T1, T0, "把t0放到t1里");
        objMips_Regs("move", T0, T7, "把t7放到t0里");
        /* 对语法树节点t的成员运算符attr.op分类处理 */
        switch (t->attr.ExpAttr.op)
        {
            /* 语法树节点成员运算符为PLUS,生成加法指令 */
        case PLUS:
            objMips_Regs("add", T0, T0, T1, "op +");
            break;
            /* 语法树节点成员运算符为MINUS,生成减法指令 */
        case MINUS:
            objMips_Regs("sub", T0, T0, T1, "op -");
            break;
            /* 语法树节点成员操作符为TIMES,写入乘法指令 */
        case TIMES:
            objMips_Regs("mult", T0, T1, "op *");
            objMips_Regs("mflo", T0, "将低32为载入T0");
            break;
            /* 语法树节点成员操作符为OVER,写入除法指令 */
        case OVER:
            objMips_Regs("div", T0, T1, "op /");
            objMips_Regs("mflo", T0, "将商为载入T0");
            break;
            /* 语法树节点成员操作符为LT,写入相应的指令序列 */
            /* 如果为真，结果为1；否则结果为0 */
        case LT: {
            /* 写入减指令,将(左-右)操作数相减,结果送t0 */
            objMips_Regs("sub", T0, T0, T1, "op <");
            /* 写入判断跳转指令,如果累加器ac的值小于0,	*
             * 则代码指令指示器跳过两条指令				*/
            string true_label = prepare_label("true");
            string false_label = prepare_label("false", 1);
            objMips_Conj("blt", T0, ZERO, true_label, "br if true");
            /* 写入载入常量指令,将累加器ac赋值为0 */
            objMips_Regs("li", T0, "0", "false case");
            /* 写入数值载入指令,代码指令指示器pc跳过下一条指令 */
            objMips_UnConj("j", false_label, "必定跳过下一条指令");
            emitLabel("true");
            /* 写入载入常量指令,将t0=1 */
            objMips_Regs("li", T0, "1", "true case");
            emitLabel("false");
            break;
            /* 语法树节点成员操作符为EQ,写入相应的指令序列 */
            /* 如果为真，结果为1；否则结果为0 */
        }
        case EQ:
        {
            /* 写入减法指令,将左,右操作数相减,结果送t0 */
            objMips_Regs("sub", T0, T1, T0, "op ==");
            /* 写入判断跳转指令,如果t0等于0,	*
             * 跳过两条指令			*/
            string true_label = prepare_label("true");
            string false_label = prepare_label("false", 1);
            objMips_Conj("beq", T0, ZERO, true_label, "br if true");
            /* 写入载入常量指令,将t0赋值为0 */
            objMips_Regs("li", T0, "0", "false case");
            /* 跳过一条指令 */
            objMips_UnConj("j", false_label, "必定跳过下一条指令");
            /* 写入载入常量指令,将t0赋值为1 */
            emitLabel("true");
            objMips_Regs("li", T0, "1", "true case");
            emitLabel("false");
            break;
        }
        /* 其他未知运算符,写入注释,标注未知运算符信息 */
        default:
            emitComment("BUG: Unknown operator");
            break;
        }
        break;
    default:
        break;
    }
}

/* 功能：代码生成分发入口，根据语法树节点类型调用语句或表达式生成函数。 */
void cGen(Treenode* tree) {
    if (tree != NULL)
    {
        /* 对语法树节点类型成员nodekind分类处理 */
        switch (tree->nodekind)
        {
            /* 对语句类型语法树节点调用代码生成函数,生成目标代码 */
        case StmtK:
            genStmt(tree);
            break;

            /* 对表达式类型语法树节点调用代码生成函数,生成目标代码 */
        case ExpK:
            genExp(tree);
            break;

        default:
            break;
        }
        /* 对语法树节点的兄弟节点递归调用函数cGen(),生成目标代码 */
        cGen(tree->sibling.get());
    }
}

/* 功能：生成MIPS系统调用代码，支持读整数、写整数、写字符串和退出等操作。 */
void objMips_SysCall(int choice, string reg) {
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    mipsCode[curLine] = "li $v0," + to_string(choice);
    curLine++;
    curAddr += 4;
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    switch (choice) {
    case 5:
        break;
    case 4:mipsCode[curLine] = "la $a0," + reg;
        curLine++;
        curAddr += 4;
        break;
    case 1:/*打印一个整数*/
        mipsCode[curLine] = "move $a0," + reg;
        curLine++;
        curAddr += 4;
        break;
    case 10:
        break;
    }
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
    mipsCode[curLine] = "syscall";
    curLine++;
    curAddr += 4;
    if (curLine >= mipsCode.size()) {
        mipsCode.resize(mipsCode.size() + 2000, "");
    }
}

/* 功能：为语句节点生成MIPS代码，处理if、while、赋值、读写、调用和返回语句。 */
void genStmt(Treenode* t) {
    /*用于控制转移display表的各项sp值*/

    /*用于存储语法树的各个节点*/
    Treenode* p0 = NULL;
    Treenode* p1 = NULL;
    Treenode* p2 = NULL;
    int FormParam;

    /*用于记录跳转回填时的地址*/
    int  savedLoc1, savedLoc2;
    int savedAddr1, savedAddr2;

    /*指向实参的指针*/
    ParamTable* curParam = NULL;

    switch (t->kind.stmt)
    {
        /*处理if语句*/
    case IfK: {
        p0 = t->child[0].get(); /*条件表达式部分*/
        p1 = t->child[1].get(); /*then语句序列部分*/
        p2 = t->child[2].get(); /*else语句序列部分*/
        emitComment("->if");
        cGen(p0);         /*产生测试表达式的代码*/

        /* savedLoc1赋值为当前指令的tm地址号(emitLoc),emitLoc加1,       *
        /* 为if条件表达式为假时，产生跳转地址回填所留一代码空位         */
        savedAddr1 = curAddr;
        savedLoc1 = emitSkip(1);

        /* 指令回填地址savedLoc1记录当前生成代码写入地址,				*
         * 再将当前生成代码写入地址emitLoc加1							*
         * 为条件表达式为假的跳转指令留一条指令写入空位,用于代码回填	*
         * 将注释写入目标代码文件code,此处为向else跳转指令位置			*/
         /*产生then语句部分的代码*/
        cGen(p1);

        /* 指令回填地址savedLoc2记录当前生成代码写入地址,				*
         * 再将当前生成代码写入地址emitLoc加1							*
         * 为跳转到判断语句结束位置的指令留一条代码空位,用于代码回填	*
         * 将注释写入目标代码文件code,此处为向end跳转指令位置	        */
        savedAddr2 = curAddr;
        savedLoc2 = emitSkip(1);

        /* 生成else标签，作为条件为假时的跳转目标 */
        string label_else = emitLabel("Else");
        /*退回到指令回填地址savedLoc1,此处已经预留了一个指令空位*/
        emitBackup(savedLoc1, savedAddr1);

        /* 写入跳转到else的指令 */
        objMips_Conj("beq", T0, ZERO, label_else, "跳转到else分支");
        /* 恢复到else标签后的代码生成位置 */
        emitRestore();

        /* 处理else语句部分的代码 */
        cGen(p2);

        /* 生成if结束标签，作为then分支结束后的跳转目标 */
        string label_endif = emitLabel("EndIf");
        /* 回退到指令回填地址savedLoc2，此处已经预留了一个指令空间 */
        emitBackup(savedLoc2, savedAddr2);

        /* 写入跳转到end的指令 */

        objMips_UnConj("j", label_endif, "jmp to end");
        /* 恢复到EndIf标签后的代码生成位置 */
        emitRestore();

        emitComment("<- if");            /*if语句结束*/
        break;
    }
            /*处理while语句*/
    case WhileK: {
        emitComment("->while");
        string while_label = emitLabel("While");
        p0 = t->child[0].get();  /*p0为while语句的条件表达式部分*/
        p1 = t->child[1].get();  /*p1为while语句的语句序列部分*/
        cGen(p0);          /*生成条件表达式部分代码*/

        /* 如果条件表达式为假，则跳转至while语句结束   *
        /* 此处为地址回填预留一个指令空间              */
        savedAddr1 = curAddr;
        savedLoc1 = emitSkip(1);

        emitComment(" while : jump out while ");

        cGen(p1);     /*生成语句序列部分代码*/

        /*跳到条件表达式处，进入下一次循环*/
        objMips_UnConj("j", while_label, "jump to start");
        emitComment(" return to condition exp");
        string endwhile_label = emitLabel("EndWhile");
        /*条件为假时,跳出while循环*/
        emitBackup(savedLoc1, savedAddr1);

        objMips_Conj("beq", T0, ZERO, endwhile_label, " jump out while ");
        emitRestore();

        emitComment("<-while");

        break;
    }
               /*处理赋值语句*/
    case AssignK:
    {
        emitComment("->assign");

        p0 = t->child[0].get();  /*赋值号左侧的部分*/

        p1 = t->child[1].get();  /*赋值号右侧的部分*/

        findAbsAddr(p0);

        objMips_Regs("move", T2, T0, "save ac");
        cGen(p1);          /*处理赋值号右侧的表达式部分*/
        /*结果存入ac中*/

        if (p0->table[0]->attrIR->More.VarAttr.access == dir)
            /*赋值*/
            objMips_RM("sw", T0, T2, 0, "var assign : store value");
        else
        {

            objMips_RM("lw", T2, T2, 0, "indir var assign");
            objMips_RM("sw", T0, T2, 0, "store value");

        }
        emitComment("<- assign");
        break;
    }

    /*处理输入语句*/
    case ReadK:

        emitComment("-> read");
        objMips_SysCall(5);
        findAbsAddr(t);
        //记得往T0存变量的绝对地址!!!!!!!!!!!!!!!!!!
        objMips_Regs("li", T1, "4", "");
        objMips_RM("sw", V0, SP, 0, "保存读入的值在栈中");
        objMips_Regs("sub", SP, SP, T1, "read语句开辟栈存储读入的值");
        if (t->table[0]->attrIR->More.VarAttr.access == dir)
        {
            /*直接存*/
            /*最后生成存储指令*/
            objMips_RM("lw", T1, SP, 4, " 栈中取read的数");

            objMips_RM("sw", T1, T0, 0, " var read : store value");
        }
        else
        {
            objMips_RM("lw", T1, T0, 0, "");
            objMips_RM("lw", T0, SP, 4, " 栈中取read的数");
            objMips_RM("sw", T0, T1, 0, " var read : store value");
        }
        objMips_Regs("li", T1, "4", "");
        objMips_Regs("add", SP, SP, T1, "read语句开辟栈存储读入的值");
        emitComment("<- read");

        break;

        /* 处理write语句类型 */
    case WriteK:
        emitComment("-> write");

        p0 = t->child[0].get();
        cGen(p0);             /*处理write语句部分的表达式部分*/

        objMips_SysCall(1, T0);
        /*输出后都打印换行符*/
        mipsCode[curLine++] = "li $v0, 4       # 设置系统调用编号为4（打印字符串）";
        mipsCode[curLine++] = "la $a0, newline # 加载换行符的地址到$a0寄存器";
        mipsCode[curLine++] = "syscall         # 调用系统调用";
        curAddr += 12;
        emitComment("<- write");
        break;

        /* 处理过程调用语句 */

    case ReturnK:

        emitComment("-> return");
        objMips_RM("lw", T2, T4, -4, "");
        objMips_UnConj("jr", T2, " return address");
        emitComment("<- return");
        break;
    case CallK: {
        emitComment("-------------------> Call Start");
        p0 = t->child[0].get();    /*过程名*/
        p1 = t->child[1].get();    /*过程的实参*/

        curParam = t->table[0]->attrIR->More.ProcAttr.param;
        SymbolsTable* procSymbol = t->table[0];


        while ((curParam != NULL) && (p1 != NULL)) {
            FormParam = curParam->entry->attrIR->More.VarAttr.off;//取当前形参在被调用过程活动记录中的偏移量

            /*形参是indir*/
            if (curParam->entry->attrIR->More.VarAttr.access == indir) {
                SymbolsTable* symbol = p1->table[0];
                
                /*放弃了搞不了*/
                /*如果形参是var 数组,并且传入参数为array，应该把数组首地址传入*/
                /*if (symbol->attrIR->idType->typeKind == ArrayK) {
                    findAbsAddr(p1, 1);
                    objMips_RM("sw", T0, SP, -FormParam, "如果形参是var数组,并且传入参数为array，应该把数组首地址传入");
                }*/
                
                /*实参是indir,把地址传入形参*/
                if (symbol->attrIR->More.VarAttr.access == indir) {
                   
                        findAbsAddr(p1);
                        objMips_RM("lw", T0, T0, 0, "变参取值,取出来的是地址");
                        objMips_RM("sw", T0, SP, -FormParam, "变参，把地址传入为下一层存传入的参数");
                    
                }
                /*实参是dir，直接把地址传入，不用二次取址*/
                else
                {
                    findAbsAddr(p1);
                    objMips_RM("sw", T0, SP, -FormParam, "为下一层存传入的参数");
                }

            }
            /*如果形参是dir*/
            else
            {   
                /*如果传入的参数是标识符*/
                if (p1->kind.exp == IdEK) {
                    SymbolsTable* symbol = p1->table[0];
                    /*这个里面如果是dir,返回的是值，如果是indir也是返回的值*/
                    genExp(p1);
                    
                    if (symbol->attrIR->More.VarAttr.access == indir) {
                        objMips_Regs("move", T2, T0, "调用参数是间接变量");
                        //objMips_RM("lw", T2, T0, 0, "是间接变量");
                        //objMips_RM("lw", T2, T2, 0, "");
                        objMips_RM("sw", T2, SP, -FormParam, "为下一层存传入的参数");
                    }
                    /*如果是直接变量，把值传进去*/
                    else {
                        //objMips_RM("lw", T2, T0, 0, "是直接变量");
                        objMips_RM("sw", T0, SP, -FormParam, "为下一层存传入的参数");
                    }
                }
                /*如果传入的参数是常数*/
                else {
                    genExp(p1);
                    objMips_RM("sw", T0, SP, -FormParam, "为下一层存传入的参数");
                }
            }

            curParam = curParam->next;
            p1 = p1->sibling.get();
        }
        objMips_RM("sw", T4, SP, 0, "存储动态链条地址");
        objMips_Regs("li", T5, to_string(procSymbol->attrIR->More.ProcAttr.level), "T5此时存层数");
        objMips_RM("sw", T5, T4, -8, "T5存入活动记录");
        objMips_RM("sw", T0, SP, -12, "存T0数值到栈");
        objMips_RM("sw", T1, SP, -16, "存T1数值到栈");
        objMips_RM("sw", T2, SP, -20, "存T2数值到栈");
        objMips_Regs("li", T5, to_string(procSymbol->attrIR->More.ProcAttr.off), "T5此时displayoff表偏移");
        objMips_RM("sw", T5, SP, -24, "存displayOff到栈");
        objMips_Regs("sub", T5, SP, T5, "T5存display表绝对地址");
        int ACSIZE = procSymbol->attrIR->More.ProcAttr.moff;
        objMips_Regs("move", T4, SP, "将新生成记录的栈底存储T4");
        //接下来的栈底统一用T4！！！！
        objMips_RegIm("addi", SP, SP, to_string(-ACSIZE), "申请ACSIZE大小的活动记录空间,SP指向栈顶");
        int displayFill = procSymbol->attrIR->More.ProcAttr.off;
        for (int i = 0; i < procSymbol->attrIR->More.ProcAttr.level; i++) {
            objMips_RM("lw", T5, T4, 0, "动态链取上层记录初始地址");
            objMips_RM("lw", T6, T5, -24, "取上层记录display表偏移地址");
            objMips_Regs("sub", T5, T5, T6, "T5存算出的上层display表绝对地址,T5=T5-T6");

            objMips_RM("lw", T6, T5, i * (-4), "T6此时上层display表中第i层内容");
            objMips_RM("sw", T6, T4, -displayFill, "这里T4是本层起始地址，从这计算的偏移,本层对应i层存入对应上层display表第i个数据");
            displayFill += 4;
        }
        objMips_RM("sw", T4, T4, -displayFill, "还有本层起始地址也要填入display表");

        objMips_UnConj("jal", "Proc_" + p0->name[0], "转向子程序");

        objMips_RM("lw", T0, T4, -12, "恢复T0");
        objMips_RM("lw", T1, T4, -16, "恢复T1");
        objMips_RM("lw", T2, T4, -20, "恢复T2");
        objMips_RegIm("addi", SP, T4, "0", "恢复SP");
        objMips_RM("lw", T4, T4, 0, "恢复T4");
        emitComment("<------------------- Call End");
    }
              break;
    default:
        break;
    }
}

/* 功能：为过程声明生成MIPS代码，包括过程入口、嵌套过程、过程体和返回逻辑。 */
void genProc(Treenode* t) {
    int savedLoc1;         /*处理过程入口时所需的代码地址*/
    int savedAddr1;
    emitComment("->procedure");
    /*生成标签*/
    /*修改处*/
    genLabel("Proc_" + t->name[0]);
    /*先填写返回地址*/
    objMips_RM("sw", RA, T4, -4, "填写返回地址");

    TreenodePtr& tp1 = t->child[1];//声明部分
    Treenode* p1=tp1.get();
    TreenodePtr& tp2 = t->child[2];//过程体
    Treenode* p2=tp2.get();

    /*存一条指令，跳转到begin处*/
    savedAddr1 = curAddr;
    savedLoc1 = emitSkip(1);

    while (p1 != NULL)
    {
        /*处理过程声明部分*/
        if (p1->nodekind == ProcDecK)
        {
            genProc(p1);
        }
        p1 = p1->sibling.get();
    }
    /*在此处生成begin标签*/
    genLabel(t->name[0] + "_begin");
    emitBackup(savedLoc1, savedAddr1); //回填跳转指令 
    /*填写保留的跳转标签*/
    objMips_UnConj("j", t->name[0] + "_begin", "过程入口");
    emitRestore();
    if (p2 != NULL)
        p2 = p2->child[0].get();
    while (p2 != NULL)
    {
        genStmt(p2);
        p2 = p2->sibling.get();
    }
    /*那return用不用无所谓了*/
    objMips_RM("lw", T2, T4, -4, " fetch return address");
    objMips_UnConj("jr", T2, "回到上一级过程");
    emitComment("<-procedure");
}

/* 功能：目标代码生成总入口，生成数据段、过程代码、主程序代码并写入目标文件。 */
void codeGen(TreenodePtr t) {
    int savedloc;//目标代码第一条地址
    int savedAddr;
    TreenodePtr& MainProc = t->child[0];//Phead
    TreenodePtr& tmp = t->child[1];
    Treenode* DecNode=tmp.get();
    TreenodePtr& BodyNode = t->child[2];
    mipsCode.resize(2000, "");

     /*数据区换行符*/
     //写数据段和代码段
    mipsCode[curLine++] = DATASEG;
    highLine = max(curLine, highLine);
    mipsCode[curLine++] = "newline: .asciiz \"\\n\"";
    highLine = max(curLine, highLine);

    //写代码段
    mipsCode[curLine] = CODESEG;
    curLine++;
    highLine = max(curLine, highLine);

    
    savedAddr = curAddr;
    savedloc = emitSkip(1);//为主程序跳转预留空间，预留1条指令的空间用于回填

    //处理主程序声明部分
    while(DecNode != nullptr) {
        if (DecNode->nodekind == ProcDecK) {
            genProc(DecNode);
        }
        DecNode = DecNode->sibling.get();
    }

    string mainlabel = emitLabel("main");
    objMips_RegIm("addi", T4, SP, "0", "主过程活动记录生成！！！");
    SymbolsTable* proSymbol = MainProc->table[0];
    int ACSIZE = proSymbol->attrIR->More.ProcAttr.moff;
    int off = proSymbol->attrIR->More.ProcAttr.off;
    objMips_Regs("li", T1, to_string(off), "Main程序display偏移");
    //此时SP=T4，还没有作为栈顶
    objMips_RM("sw", T1, SP, -24, "Main程序存displayoff偏移");
    objMips_Regs("sub", T1, SP, T1, "Main程序display表绝对偏移地址");
    objMips_RM("sw", SP, T1, 0, "存储Main程序display表第一个元素，即他自己");
    objMips_RegIm("addi", SP, SP, to_string(-ACSIZE), "申请Main程序栈空间");

    //处理主程序体
    emitBackup(savedloc, savedAddr);
    objMips_UnConj("j", mainlabel, "main entry");
    emitRestore();
    Treenode* stmtNode = BodyNode->child[0].get();
    if (stmtNode != nullptr) {
         cGen(stmtNode);
     }

    //结束部分
    objMips_Regs("li", V0, to_string(10), "退出程序");
    mipsCode[curLine] = "syscall";
    ofstream outObj;
    outObj.open(objAddr);
    for (int i = 0; i <= curLine; i++) {
        outObj << mipsCode[i] << endl;
    }
    outObj.close();
}
