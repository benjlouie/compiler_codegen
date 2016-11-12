/**
 * authors: Forest, Benji
 */
#include "vTable.h"

//debug
#include <stdio.h>

vTable::vTable() {}

/**
 * authors: Forest, Benji
 */
void dfs_buildVTable(vector<string> curEntry, string clsName) {
	if (globalTypeList.count(clsName) == 0)
		return; //not in a class anymore
	static int entry_count = 1;
	vector<string> entry;
	entry.push_back("string" + entry_count++);
	entry.push_back(clsName + "..new");
	if (clsName != "Object") {
		for (int i = 2; i < curEntry.size(); i++) {
			entry.push_back(curEntry[i]);
		}
	} else {
		entry.push_back("Object.abort");
		entry.push_back("Object.copy");
		entry.push_back("Object.type_name");
	}
	printf("class: %s\n", clsName.c_str());
	for (string s : curEntry) {
		printf("\t%s\n", s.c_str());
	}
	for (auto chld : globalSymTable->getChildrenNames()) {
		dfs_buildVTable(entry, chld);
	}
}
/**
 * authors: Forest, Benji
 */
void buildVTable() {
	globalVTable = new vTable;

	vector<string> entry;
	globalSymTable->cur = globalSymTable->symRoot; //reset the current scope
	dfs_buildVTable(entry, "Object");

}
