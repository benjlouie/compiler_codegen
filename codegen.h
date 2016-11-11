#include <iostream>
#include <unordered_map>
#include "ast.h"
#include "symbolTable.h"

extern int numErrors;
extern Node *root;
extern SymbolTable *globalSymTable;
extern unordered_map<string,string> globalTypeList;

int code();
