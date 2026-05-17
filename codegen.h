#ifndef COMPILER_CODEGEN_H
#define COMPILER_CODEGEN_H

#include "semantic.h"

#include <string>

int objMips_UnConj(string op, string target, string comment);
int objMips_RM(string op, string rs, string rt, int immediate, string comment);
int objMips_Conj(string op, string rs, string rt, string tag, string comment);
int objMips_RegIm(string op, string rd, string rs, string immediate, string comment);
int objMips_Regs(string op, string rd, string rs, string comment);
int objMips_Regs(string op, string rd, string rs, string rt, string comment);
int objMips_Regs(string op, string rd, string comment);
void objMips_SysCall(int choice, string reg = "");
void getARBase(int varLevel);
void findAbsAddr(Treenode* t);
void genStmt(Treenode* t);
void genExp(Treenode* t);
void cGen(Treenode* t);
void codeGen(TreenodePtr t);
int emitSkip(int num);
void emitBackup(int loc, int addr);
void emitRestore();
void emitComment(string c);
void genProc(Treenode* t);

#endif /* COMPILER_CODEGEN_H */
