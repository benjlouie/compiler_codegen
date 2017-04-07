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
#define OBJECT_TYPE "0"
#define PRIMITIVE_TYPE "1"

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

/*Some helper functions*/
void atCalleeExit(InstructionList &methodLinear);
void atCalleeEntry(InstructionList &methodLinear);
void getMethodParamIntoRegister(InstructionList &methodLinear, int numParam, string placeToPut, int numFormals);
void callCalloc(InstructionList &methodLinear, string paramHoldNumElements, string paramHoldSizeOfEachElement, string type);
void errorHandlerDoExit(InstructionList &methodLinear, string label, string error);
