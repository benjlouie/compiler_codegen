#include "codegen.h"

void code()
{
	//generate offsets for symbolTable variables
	globalSymTable->generateOffsets();
	//get number of locals at each level scope level
	globalSymTable->countLocals();
	globalSymTable->generateTags();

    //Forest's stuff
    buildVTable();

    //make stuff for builtins
       
    //Make the Linear IR
    unordered_map<std::string, InstructionList &> *sections = makeLinear();

	//header then data then .text then vtable
	sections->at("#header").printIR();
	cout << ".data\n";
	sections->at(".data").printIR();
	cout << ".text\n";
	sections->at("#global_vtable").printIR();
	for (auto it = sections->begin(); it != sections->end(); it++) {
		if (it->first == ".data" || it->first[0] == '#') {
			continue;
		}
		cout << it->first << ":\n";
		it->second.printIR();
	}
       
    //highLevelInstrSelection();

    //printAssembly();
}
