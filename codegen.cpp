#include "codegen.h"

void code()
{
	//generate offsets for symbolTable variables
	globalSymTable->generateOffsets();
	//get number of locals at each level scope level
	globalSymTable->countLocals();

    //Forest's stuff
    buildVTable();

    //make stuff for builtins
       
    //Make the Linear IR
    unordered_map<std::string, InstructionList &> *methods = makeLinear();

	for (auto it = methods->begin(); it != methods->end(); it++) {
		cout << it->first << '\n';
		it->second.printIR();
	}
       
    //highLevelInstrSelection();

    //printAssembly();
}
