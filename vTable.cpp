#include "ast.h"
#include "symTable.h"

void vTable::buildVTable() {
	vTable newVTable = new vTable;

	globalVTable = newVTable;
}
