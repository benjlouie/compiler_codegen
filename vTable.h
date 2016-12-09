/**
 * authors: Forest, Benj
 */
#pragma once
#include <unordered_map>
#include <vector>
#include "../compiler_semantics/symbolTable.h"
#include "../src/ast.h"

using namespace std;

extern SymbolTable *globalSymTable;
extern unordered_map<string, string> globalTypeList;


void buildVTable(void);

class vTable {
public:
	unordered_map<string, vector<string>> vtable;

	vTable();
	int getOffset(string cls, string method_name);
};

extern vTable *globalVTable;
