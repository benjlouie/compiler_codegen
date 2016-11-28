#include "codegen.h"

/*
 * @Author: Everyone
 * Calls everything in the correct ways
 */
void code(fstream &outfile)
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
	sections->at("#header").printIR(outfile);
	outfile << ".data\n";
	sections->at(".data").printIR(outfile);
	outfile << ".text\n";
	sections->at("#global_vtable").printIR(outfile);
	for (auto it = sections->begin(); it != sections->end(); it++) {
		if (it->first == ".data" || it->first[0] == '#') {
			continue;
		}
		outfile << it->first << ":\n";
		it->second.printIR(outfile);
	}
    
	makeGarbageCollectorIR().printIR(outfile);
    //highLevelInstrSelection();

    //printAssembly();
}
