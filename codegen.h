#include <iostream>
#include <unordered_map>
#include "../src/ast.h"
#include "../compiler_semantics/symbolTable.h"
#include "linearIR.h"

extern int numErrors;
extern Node *root;
extern SymbolTable *globalSymTable;
extern unordered_map<string,string> globalTypeList;

void code();
