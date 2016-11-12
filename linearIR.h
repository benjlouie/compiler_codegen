#pragma once
#include<unordered_map>
#include<vector>
#include<string>
#include "../src/ast.h"
#include "../compiler_semantics/symbolTable.h"
#include "InstructionList.h"

#define DEFAULT_VAR_OFFSET 24

extern int numErrors;
extern Node *root;
extern SymbolTable *globalSymTable;
extern unordered_map<string, string> globalTypeList;
extern unordered_map<size_t, string> globalStringTable;


unordered_map<string,InstructionList &> *makeLinear();
