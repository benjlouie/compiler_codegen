#pragma once
#include<unordered_map>
#include<vector>
#include<string>
#include <stack>
#include <algorithm>
#include "../src/ast.h"
#include "../compiler_semantics/symbolTable.h"
#include "InstructionList.h"
#include "vTable.h"

#define DEFAULT_VAR_OFFSET 24

extern int numErrors;
extern Node *root;
extern SymbolTable *globalSymTable;
extern unordered_map<string, string> globalTypeList;
extern unordered_map<size_t, string> globalStringTable;
extern int whileLabelCount;
extern int ifLabelCount;
extern int caseLabelCount;
extern vTable *globalVTable;


unordered_map<string,InstructionList &> *makeLinear();
