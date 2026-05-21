#ifndef COMPILER_MIDCODE_H
#define COMPILER_MIDCODE_H

#include "semantic.h"

#include <string>
#include <vector>

struct MidCode {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;
};

std::vector<MidCode> buildMidCode(const Treenode* root);
void writeMidCode(const std::vector<MidCode>& codes, const std::string& outputPath = "midcode.txt");
void generateMidCode(const Treenode* root, const std::string& outputPath = "midcode.txt");

#endif /* COMPILER_MIDCODE_H */
