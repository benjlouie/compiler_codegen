#include "codegen.h"

void code()
{
    //Forest's stuff
    //buildVTable();

    //make stuff for builtins
       
    //Make the Linear IR
    unordered_map<std::string, InstructionList &> *methods = makeLinear();

	(*methods).at("Main.main").printIR();

	for (auto method : (*methods)) {
		cout << method.first << '\n';
		method.second.printIR();
	}
       
    //highLevelInstrSelection();

    //printAssembly();
}
