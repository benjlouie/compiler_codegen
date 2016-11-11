#include<unordered_map>
#include<vector>
#include<string>
#include "../src/ast.h"
#include "../compiler_semantics/symbolTable.h"
#include "InstructionList.h"

extern int numErrors;
extern Node *root;
extern SymbolTable *globalSymTable;
extern unordered_map<string, string> globalTypeList;

unordered_map<string,InstructionList &> *makeLinear();
