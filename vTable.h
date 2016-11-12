#pragma once
#include <unordered_map>
#include <vector>
using namespace std;

extern vTable *globalVTable;

class vTable {
public:
	unordered_map<string, vector<strings>> vtable;

	buildVTable(void);
}
