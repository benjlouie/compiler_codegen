/**
 * authors: Forest, Benji
 */
#include "vTable.h"
#include <string>
#include <vector>


vTable::vTable() {}

/**
 * authors: Forest, Benji
 */
int inVtable(string method, vector<string> inher) {
	string meth;
	for (unsigned int i = 2; i < inher.size(); i++) {
		meth = inher[i].substr(inher[i].find(".") + 1);
		if (meth == method) //from .onwards
			return i;
	}
	return -1;
}

/**
 * authors: Forest, Benji
 */
void dfs_buildVTable(vector<string> curEntry, string clsName) {
	if (globalTypeList.count(clsName) == 0)
		return; //not in a class anymore
	static int entry_count = 1;
	vector<string> entry;
	entry.push_back("className" + to_string(entry_count++));
	entry.push_back(clsName + "..new");
	if (clsName != "Object") {
		for (size_t i = 2; i < curEntry.size(); i++) {
			entry.push_back(curEntry[i]);
		}
		vector<string> methods = globalSymTable->getMethodNames();
		for (auto method : methods) {
			int index = inVtable(method, curEntry);
			if (index == -1) {
				entry.push_back(clsName + "." + method);
			} else {
				entry[index] = clsName + "." + method;
			}			
		}
	} else {
		entry.push_back("Object.abort");
		entry.push_back("Object.copy");
		entry.push_back("Object.type_name");
	}
	for (auto chld : globalSymTable->getChildrenNames()) {
		globalSymTable->enterScope(chld);
		dfs_buildVTable(entry, chld);
		globalSymTable->leaveScope();
	}
	globalVTable->vtable[clsName] = entry;
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

//Author: Forest
//Gives the offset, in number of entries, a method is from the base of the classes vtable entry
int vTable::getOffset(string cls, string method_name)
{
	vector<string> methods = vtable[cls];
	string method;
	for (unsigned int i = 2; i < methods.size(); i++) {//start at 2 to ignore name and ..new
		method = methods[i].substr(methods[i].find(".") + 1);
		if (method_name == method) //from .onwards
			return i;
	}
	cerr << "Unable to find method in vtable!!" << endl;
	exit(1);
}
