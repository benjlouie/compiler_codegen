#ifndef __CODEGEN_H_
#define __CODEGEN_H_

#include <iostream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "../src/ast.h"
#include "../compiler_semantics/symbolTable.h"
#include "linearIR.h"
#include "vTable.h"
#include "garbageCollector.h"

extern int numErrors;
extern int whileLabelCount;
extern int ifLabelCount;
extern Node *root;
extern SymbolTable *globalSymTable;
extern unordered_map<string,string> globalTypeList;
extern unordered_map<size_t, string> globalStringTable;


void code(fstream &outfile);

#endif // !__CODEGEN_H_
