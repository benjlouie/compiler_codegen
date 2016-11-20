#include "../compiler_codegen/linearIR.h"

// IMPORTANT REGISTER INFO
// volatile registers: (destroyed on function call)
// rax, rcx, rdx, r8, r9, r10, r11
// nonvolatile registers: 
// rbx, rbp, rdi, rsi, rsp, r12, r13, r14, r15

// IMPORTANT CALL INFO
// caller responsibility
//// push rbp
//// push formals (left to right)
//// push self
//// call function
//// pop (self)
//// pop (formals)
//// pop rbp
//// mov rbp rsp
// callee responsibility
//// return value in r15


class StackVariables {
public:
	StackVariables() {
		formalOffset = 8;
		varOffset = -8;
	}
	~StackVariables() {

	}

	void addFormal(std::string formalName) {
		vars[formalName].push(formalOffset);
		formalOffset += 8;
	}
	void addVar(std::string varName) {
		vars[varName].push(varOffset);
		varOffset -= 8;
	}
	void removeVar(std::string varName) {
		vars[varName].pop();
		if (vars[varName].size() == 0) {
			vars.erase(varName);
		}
	}

	bool checkVar(std::string name) {
		return vars.count(name) > 0;
	}
	int getOffset(std::string name) {
		return vars[name].top();
	}
	void addCaseVars(vector<string> ids) {
		for (string id : ids) {
			vars[id].push(varOffset);
		}
		varOffset -= 8;
	}
	void removeCaseVars(vector<string> ids) {
		for (string id : ids) {
			vars[id].pop();
			if (vars[id].size() == 0)
				vars.erase(id);
		}
		varOffset += 8;
	}
private:
	std::unordered_map<std::string, std::stack<int>> vars;
	int formalOffset;
	int varOffset;
};

StackVariables *vars;

InstructionList &makeMethodIR(Node *feat);
InstructionList &makeClassIR(Node *cls, unordered_map<string, vector<Node *>> *attributes);
InstructionList &makeEntryPointIR();
InstructionList &makeIntIR();
InstructionList &makeIOIR();
InstructionList &makeBoolIR();
InstructionList &makeStringIR();
InstructionList &makeObjectIR();
InstructionList &makeVTableIR();
InstructionList &makeStringsIR();
InstructionList &makeHeaderIR();

/*built in functions*/
InstructionList &makeAbortIR();
InstructionList &makeTypeNameIR();
InstructionList &makeCopyIR();
InstructionList &makeOutStringIR();
InstructionList &makeInStringIR();
InstructionList &makeOutIntIR();
InstructionList &makeInIntIR();
InstructionList &makeLengthIR();
InstructionList &makeConcatIR();
InstructionList &makeSubstrIR();
InstructionList &makeLThandler();
InstructionList &makeLTEhandler();
InstructionList &makeCaseErrorIR();

/*Some helper functions*/
void atCalleeExit(InstructionList &methodLinear);
void atCalleeEntry(InstructionList &methodLinear);
void getMethodParamIntoRegister(InstructionList &methodLinear, int numParam, string placeToPut);
void callCalloc(InstructionList &methodLinear, string paramHoldNumElements, string paramHoldSizeOfEachElement);
void errorHandlerDoExit(InstructionList &methodLinear, string label, string error);

void makeNew(InstructionList &methodLinear, string valType);
void makeExprIR_recursive(InstructionList &methodLinear, Node *expression);
void methodInit(InstructionList &methodLinear, Node *feature);
void methodExit(InstructionList &methodLinear, Node *feature);
void doIntLiteral(InstructionList &methodLinear, Node *expression);
void doPlus(InstructionList &methodLinear, Node *expression);
void doMinus(InstructionList &methodLinear, Node *expression);
void doMultiply(InstructionList &methodLinear, Node *expression);
void doDivide(InstructionList &methodLinear, Node *expression);
void doBool(InstructionList &methodLinear, Node *expression, bool val);
void doString(InstructionList &methodLinear, Node *expression);
void doIdentifier(InstructionList &methodLinear, Node *expression);
void doLessThan(InstructionList &methodLinear, Node *expression);
void doLessThanEqual(InstructionList &methodLinear, Node *expression);
void doEqual(InstructionList &methodLinear, Node *expression);
void doTilde(InstructionList &methodLinear, Node *expression);
void doNot(InstructionList &methodLinear, Node *expression);
void doExprSemiList(InstructionList &methodLinear, Node *expression);
void doNew(InstructionList &methodLinear, Node *expression);
void doIsVoid(InstructionList &methodLinear, Node *expression);
void doWhile(InstructionList &methodLinear, Node *expression);
void doIf(InstructionList &methodLinear, Node *expression);
void doAssign(InstructionList &methodLinear, Node *expression);
void doLet(InstructionList &methodLinear, Node *expression);
void doDispatch(InstructionList &methodLinear, Node *expression);
void doCaseStatement(InstructionList &methodLinear, Node *expression);

void objectInit(InstructionList &classLinear, string name, int tag, size_t size);
void setupMethodCall(InstructionList &methodLinear, string methodName, vector<string> formals);

/*
 * Authors: Matt, Robert, and Ben
 */
unordered_map<string,InstructionList &> *makeLinear() 
{


	unordered_map<string, InstructionList &> *retMap = new unordered_map<string,InstructionList &>;

    retMap->emplace("#header", makeHeaderIR());
	retMap->emplace("#global_vtable", makeVTableIR());

	//a mapping from classes to their features
	unordered_map<string, vector<Node *>> *attributes = new unordered_map<string, vector<Node *>>;

	globalSymTable->goToRoot();
	
	//Go through each class
	auto classNodes = root->getChildren();
	for (Tree *tchild : classNodes) {
		Node *child = (Node *)tchild;

		auto classDescNodes = child->getChildren();
		string className = ((Node *)classDescNodes[0])->value;
		string inherits = ((Node *)classDescNodes[1])->value;

		Node* features = ((Node *)classDescNodes[2]);

		//enter proper scope for class and go to correct scope
		globalSymTable->goToRoot();

		string curClass = className;
		vector<string> inheritanceList;
		while (curClass != "Object") {
			inheritanceList.push_back(curClass);
			curClass = globalTypeList[curClass];
		}
		//traverse to class scope
		for (auto it = inheritanceList.rbegin(); it < inheritanceList.rend(); it++) {
			globalSymTable->enterScope(*it);
		}


		for (Tree *tFeature : features->getChildren()) {
			Node *feature = (Node *)tFeature;
			
			//Only care about methods
			if (feature->type == AST_FEATURE_METHOD) {
				std::string methName = ((Node *)feature->getChildren()[0])->value;
				std::string tmp = className + "." + methName;
				//main recursive call
				retMap->emplace(tmp, makeMethodIR(feature));
			}
			else if (feature->type == AST_FEATURE_ATTRIBUTE) {
				(*attributes)[className].push_back(feature);
			}
		}
	}

	//Need to traverse each class again, the first time we had to gather
	//each attribute now we can make initializers for each class
	classNodes = root->getChildren();
	for (Tree *tchild : classNodes) {
		Node *child = (Node *)tchild;

		auto classDescNodes = child->getChildren();
		string className = ((Node *)classDescNodes[0])->value;

		Node* features = ((Node *)classDescNodes[2]);

		//enter proper scope for class and go to correct scope
		globalSymTable->goToRoot();

		string curClass = className;
		vector<string> inheritanceList;
		while (curClass != "Object") {
			inheritanceList.push_back(curClass);
			curClass = globalTypeList[curClass];
		}
		//traverse to class scope
		for (auto it = inheritanceList.rbegin(); it < inheritanceList.rend(); it++) {
			globalSymTable->enterScope(*it);
		}

		retMap->emplace(className + "..new", makeClassIR(child, attributes));
	}

	retMap->emplace("main", makeEntryPointIR());

	//Default classes
	retMap->emplace("Int..new", makeIntIR());
	/*todo*/
	retMap->emplace("IO..new", makeIOIR());
	retMap->emplace("Object..new", makeObjectIR());
	retMap->emplace("String..new", makeStringIR());
	retMap->emplace("Bool..new", makeBoolIR());

	//builtins
	retMap->emplace("Object.abort", makeAbortIR());
	retMap->emplace("Object.type_name", makeTypeNameIR());
	retMap->emplace("Object.copy", makeCopyIR());
	retMap->emplace("IO.out_string", makeOutStringIR());
	retMap->emplace("IO.in_string", makeInStringIR());
	retMap->emplace("IO.out_int", makeOutIntIR());
	retMap->emplace("IO.in_int", makeInIntIR());
	retMap->emplace("String.length", makeLengthIR());
	retMap->emplace("String.concat", makeConcatIR());
	retMap->emplace("String.substr", makeSubstrIR());
	retMap->emplace("LT..Handler", makeLThandler());
	retMap->emplace("LTE..Handler", makeLTEhandler());

	retMap->emplace(".data", makeStringsIR());
	retMap->emplace("#case error handling", makeCaseErrorIR());

	return retMap;
}

/*
* Keyboard: Forest
* Others:
* 
*/
InstructionList &makeHeaderIR() 
{
    InstructionList *header = new InstructionList;
    header->addNewNode();
    header->addComment("Required for assembly");
    header->addInstrToTail(".globl", "main", "", InstructionList::INSTR_DIRECTIVE);
    header->addInstrToTail(".intel_syntax", "noprefix", "", InstructionList::INSTR_DIRECTIVE);
    return *header;
}

/*
* Author: Forest
* Adds the entry point for the assembly
*/
InstructionList &makeEntryPointIR() 
{
	InstructionList *entry = new InstructionList;

	entry->addNewNode();
	entry->addComment("Start point of the program");
	makeNew(*entry, "Main");

	//call Main.main()
	setupMethodCall(*entry, "Main.main", { "r15" });
	
	//return 0 for successful execution
	entry->addInstrToTail("mov", "0", "rdi");
	entry->addInstrToTail("call", "exit");

	return *entry;
}

/*
* Author: Forest
*/
InstructionList &makeClassIR(Node *cls, unordered_map<string, vector<Node *>> *attributes)
{
	InstructionList *classLinear = new InstructionList;
	string className = ((Node *)cls->getChildren()[0])->value;
	//handle return address
	classLinear->addNewNode();
	classLinear->addComment("Class " + className + " Initialization");
	//classLinear->addInstrToTail("push", "rbp");
	classLinear->addInstrToTail("mov", "rsp", "rbp");

	//TODO make space for locals

	int tag = globalSymTable->getClassTag(className);

	//get the size from the already calculated offsets
	int max = -8; //if no variables are found, this will produce correct size
	string varName, name = className;
	SymTableVariable *var;
	while (name != "Object") {
		if ((*attributes)[name].size() != 0) {
			for (auto attr : (*attributes)[name]) { //search through attributes for offsets
				varName = ((Node *)attr->getChildren()[0])->value;
				var = globalSymTable->getVariable(varName);
				if ((int) var->offset > max) //set max to be the largest offset
					max = (int)var->offset;
			}
			break; //max is now correct
		}
		else { //need to check parent
			name = globalTypeList[name];
		}
	}
	int size = (max / 8) + 4; //3 for tag, size and vtable pointer 1 for offsets starting at 0

	objectInit(*classLinear, className, tag, size);

	//initialize variables
	name = className;
	Node *expr;
	while (name != "Object") {
		for (Node *attr : (*attributes)[name]) {
			varName = ((Node *)attr->getChildren()[0])->value;
			expr = (Node *)attr->getChildren()[2];
			var = globalSymTable->getVariable(varName);

			classLinear->addNewNode();
			classLinear->addComment("Initializing variable: " + varName);

			if (expr->type == AST_NULL) {
				classLinear->addInstrToTail("mov", "0", "rax");
			}
			else {
				makeExprIR_recursive(*classLinear, expr);
				classLinear->addInstrToTail("pop", "rax");
			}

			classLinear->addInstrToTail("mov", "[rbp + 8]", "rbx"); //move self object to rbx
			classLinear->addInstrToTail("mov", "rax", "[rbx+" + std::to_string(DEFAULT_VAR_OFFSET + var->offset) +  "]");
		}
		name = globalTypeList[name];
	}

	//place return value in r15
	classLinear->addInstrToTail("mov", "[rbp + 8]", "r15");

	//function call return
	classLinear->addInstrToTail("mov", "rbp", "rsp");
	//classLinear->addInstrToTail("pop", "rbp");
	classLinear->addInstrToTail("ret");

	return *classLinear;
}

/*
* Author: Forest
* Handles allocating memory for the object, setting class tag
* and assigning the pointer to the vtable
* size should be the number of entries NOT THE NUMBER OF BYTES
*/
void objectInit(InstructionList &classLinear, string name, int tag, size_t size)
{
	//allocate memory
	classLinear.addNewNode();
	classLinear.addComment("Allocate memory, and place into rbp + 8");
	classLinear.addInstrToTail("mov", std::to_string(size), "rdi");
	classLinear.addInstrToTail("mov", "8", "rsi");
	classLinear.addInstrToTail("call", "calloc");
	classLinear.addInstrToTail("mov", "rax", "[rbp + 8]");

	//move pointer to our object to r12
	classLinear.addInstrToTail("mov", "rax", "r12");
	classLinear.addNewNode();
	classLinear.addComment("store class tag, object size and vtable pointer");

	//store tag in self[0]
	classLinear.addInstrToTail("mov", std::to_string(tag), "rax");
	classLinear.addInstrToTail("mov", "rax", "[r12]");

	//store size in self[1]
	classLinear.addInstrToTail("mov", std::to_string(size), "rax");
	classLinear.addInstrToTail("mov", "rax", "[r12 + 8]");

	//store vtable pointer in self[2]
	classLinear.addInstrToTail("lea", name + "..vtable", "rax"); //want pointer, not value at the pointer
	classLinear.addInstrToTail("mov", "rax", "[r12 + 16]");

}

/*
* Author: Forest
*/
InstructionList &makeIntIR() 
{
	InstructionList *intLinear = new InstructionList;

	string className = "Int";
	//handle return address
	intLinear->addNewNode();
	intLinear->addComment("Class " + className + " Initialization");
	//intLinear->addInstrToTail("push", "rbp");
	intLinear->addInstrToTail("mov", "rsp", "rbp");


	int tag = globalSymTable->getClassTag(className);
	int size = 4; //3 object 1 data

	objectInit(*intLinear, className, tag, size);

	//not necassary to mov 0 into self[3] because calloc 0 initializes

	//place return value in r15
	intLinear->addInstrToTail("mov", "[rbp + 8]", "r15");

	//function call return
	intLinear->addInstrToTail("mov", "rbp", "rsp");
	//intLinear->addInstrToTail("pop", "rbp");
	intLinear->addInstrToTail("ret");

	return *intLinear;
}

/*
* author: Benji
*/
InstructionList &makeIOIR()
{
	InstructionList *ioLinear = new InstructionList;
	string className = "IO";
	ioLinear->addNewNode();
	//comment added
	ioLinear->addComment("Class " + className + " Initialization");
	ioLinear->addInstrToTail("mov", "rsp", "rbp");
	int tag = globalSymTable->getClassTag(className);
	int size = 3;
	objectInit(*ioLinear, className, tag, size);
	ioLinear->addInstrToTail("ret");
	return *ioLinear;
}

/*
* author: Benji
*/
InstructionList &makeObjectIR()
{
	InstructionList *objLinear = new InstructionList;
	string className = "Object";
	objLinear->addNewNode();
	//comment added
	objLinear->addComment("Class " + className + " Initialization");
	int tag = globalSymTable->getClassTag(className);
	int size = 3;
	objLinear->addInstrToTail("mov", "rsp", "rbp");
	objectInit(*objLinear, className, tag, size);
	objLinear->addInstrToTail("ret");
	return *objLinear;
}

/*
* author: Benji
* others: everyone
*/
InstructionList &makeStringIR()
{
	InstructionList *strLinear = new InstructionList;
	string className = "String";
	strLinear->addNewNode();
	//comment added
	strLinear->addComment("Class " + className + " Initialization");
	strLinear->addInstrToTail("mov", "rsp", "rbp");
	int tag = globalSymTable->getClassTag(className);
	int size = 4;

	objectInit(*strLinear, className, tag, size);
	//make empty string (this method should only be called once
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = "";
	//load effective address of string into self[3]
	strLinear->addInstrToTail("lea", ".string" + to_string(stringNum), "rax");
	strLinear->addInstrToTail("mov", "rax", "[r12+" + to_string(DEFAULT_VAR_OFFSET) + "]");
	strLinear->addInstrToTail("mov", "r12", "r15");

	strLinear->addInstrToTail("mov", "rbp", "rsp");
	strLinear->addInstrToTail("ret");
	return *strLinear;
}

/*
* author: Benji
*/
InstructionList &makeBoolIR()
{
	InstructionList *booLinear = new InstructionList;
	string className = "Bool";
	booLinear->addNewNode();
	//comment added
	booLinear->addComment("Class " + className + " Initialization");
	int tag = globalSymTable->getClassTag(className);
	int size = 4;
	booLinear->addInstrToTail("mov", "rsp", "rbp");
	booLinear->addInstrToTail("ret");
	return *booLinear;
}


/*
* Keyboard: Forest
* Others: everyone
* 
*/
InstructionList &makeVTableIR() 
{
	InstructionList *vtableIR = new InstructionList;
	vtableIR->addNewNode();
	vtableIR->addComment("Vtable");
	string name;
	for (auto vtable : globalVTable->vtable) {
		vtableIR->addInstrToTail(vtable.first + "..vtable:", "", "", InstructionList::INSTR_LABEL);
		for (string meth : vtable.second) {
			vtableIR->addInstrToTail(".quad", meth);
		}
	}

	return *vtableIR;
}


/*
* Keyboard: Forest
* Others:
* 
*/
InstructionList &makeStringsIR()
{
	InstructionList *stringIR = new InstructionList;
	stringIR->addNewNode();
	stringIR->addComment("string constants");
	//stringIR->addInstrToTail(".data", "", "", InstructionList::INSTR_LABEL);

	string name;
	for (auto vtableEntry : globalVTable->vtable)
	{
		name = vtableEntry.second[0];
		stringIR->addInstrToTail(name + ":", "", "", InstructionList::INSTR_LABEL);
		stringIR->addInstrToTail(".string", "\"" + vtableEntry.first + "\"");
	}

	for (size_t i = 0; i < globalStringTable.size(); i++)
	{
		name = globalStringTable[i];
		stringIR->addInstrToTail(".string" + std::to_string(i) + ":", "", "", InstructionList::INSTR_LABEL);
		stringIR->addInstrToTail(".string", "\"" + name + "\"");
	}


	return *stringIR;
}

/*Built in function definitions start*/
/* Robert */
InstructionList &makeAbortIR()
{
	InstructionList *methodLinear = new InstructionList;


	methodLinear->addNewNode();
	methodLinear->addComment("ABORT FUNCTION");
	//add string to data table
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = "Abort was Called";				//******************************
	string stringName = ".string" + std::to_string(stringNum);		//		   INFO PAGE		   *
																	//******************************
	//load the string into rdi										//	--PREPARE FOR PUTS--	   *
	methodLinear->addInstrToTail("lea", stringName, "rdi");			//	move abort string into rdi *
																	//							   *
	//call puts														//							   *
	methodLinear->addInstrToTail("call", "puts");					//	call puts				   *
																	//							   *
	//move 1 into rdi for return value								//	--PREPARE FOR EXIT--	   *
	methodLinear->addInstrToTail("mov", "1", "rdi");				//	move status 0x1 into rdi   *
																	//							   *
	//call exit with code 0x1										//							   *
	methodLinear->addInstrToTail("call", "exit");					//	call exit				   *
																	//******************************
	return *methodLinear;
}

InstructionList &makeTypeNameIR()
{
	InstructionList *methodLinear = new InstructionList;

	methodLinear->addNewNode();
	methodLinear->addComment("Function needs to be implemented");
	methodLinear->addInstrToTail("ret");

	return *methodLinear;
}

/* Robert */
InstructionList &makeLThandler() {

	InstructionList *methodLinear = new InstructionList;
	methodLinear->addNewNode();
	methodLinear->addComment("LT Handler: Checking if two values are LT ");									//***********************************************
																											//*					INFO PAGE					*
	//entrance stuff																						//***********************************************
	methodLinear->addInstrToTail("mov", "rsp", "rbp");														//		boiler plate entry stuff				*
																											//												*
	//make a new bool																						//												*
	makeNew(*methodLinear, "Bool");																			//		make new boolean object					*
																											//												*
	//mov values to compare into rax and rbx																//												*
	methodLinear->addInstrToTail("mov", "[rbp+8]", "rax");													//		move first int pointer into rax			*
	methodLinear->addInstrToTail("mov", "[rax+24]", "rax");													//		move second int value into rax			*
	methodLinear->addInstrToTail("mov", "[rbp+16]", "rbx");													//		move second int pointer into rbx		*
	methodLinear->addInstrToTail("mov", "[rbx+24]", "rbx");													//		move second int value into rbx			*
																											//												*
	//compare the values																					//												*
	methodLinear->addInstrToTail("cmp", "rbx", "rax");														//		comapre rbx and rax						*
	methodLinear->addInstrToTail("jl", "LT.HANDLER.TRUE");													//		if false jump to LT.HANDLER.FALSE		*
																											//												*
	//if false move 0 into bool																				//												*
	methodLinear->addInstrToTail("mov", "0", "DWORD PTR[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");	//		move 1 into bool value					*
	methodLinear->addInstrToTail("jmp", "LT.HANDLER.END");													//		jump to LT.HANDLER.END					*
	//if true move 1 into bool																				//												*
	methodLinear->addInstrToTail("LT.HANDLER.TRUE:", "", "", InstructionList::INSTR_LABEL);					//LT.HANDLER.FALSE								*
	methodLinear->addInstrToTail("mov", "1", "DWORD PTR[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");	//		move 0 into bool value					*
																											//												*
	//return the bool and return from function																//												*
	methodLinear->addInstrToTail("LT.HANDLER.END:","","",InstructionList::INSTR_LABEL);						//LT.HANDLER.END								*
	methodLinear->addInstrToTail("mov", "rbp", "rsp");														//		boiler plate end stuff					*
	methodLinear->addInstrToTail("ret");																	//		return @ r15							*
																											//***********************************************

	return *methodLinear;
}

/* Robert */
InstructionList &makeLTEhandler() {

	InstructionList *methodLinear = new InstructionList;
	methodLinear->addNewNode();
	methodLinear->addComment("LT Handler: Checking if two values are LTE");									//************************************************
																											//*					INFO PAGE                    *
	//entrance stuff																						//************************************************
	methodLinear->addInstrToTail("mov", "rsp", "rbp");														//		boiler plate entry stuff				 *
																											//												 *
	//	make a new bool																						//												 *
	makeNew(*methodLinear, "Bool");																			//		make new boolean object					 *
																											//												 *
	//	mov values to compare into rax and rbx																//												 *
	methodLinear->addInstrToTail("mov", "[rbp+8]", "rax");													//		move first int pointer into rax			 *
	methodLinear->addInstrToTail("mov", "[rax+24]", "rax");													//		move second int value into rax			 *
	methodLinear->addInstrToTail("mov", "[rbp+16]", "rbx");													//		move second int pointer into rbx		 *
	methodLinear->addInstrToTail("mov", "[rbx+24]", "rbx");													//		move second int value into rbx			 *
																											//												 *
	//	compare the values																					//												 *
	methodLinear->addInstrToTail("cmp", "rbx" , "rax");														//		comapre rbx and rax						 *
	methodLinear->addInstrToTail("jg", "LTE.HANDLER.FALSE");												//		if false jump to LT.HANDLER.FALSE		 *
																											//												 *
	//if true move 1 into bool																				//												 *
	methodLinear->addInstrToTail("mov", "1", "DWORD PTR[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");	//		move 1 into bool value					 *
	methodLinear->addInstrToTail("jmp", "LTE.HANDLER.END");													//		jump to LT.HANDLER.END					 *
	//if false move 0 into bool																				//												 *
	methodLinear->addInstrToTail("LTE.HANDLER.FALSE:", "", "", InstructionList::INSTR_LABEL);				//LT.HANDLER.FALSE								 *
	methodLinear->addInstrToTail("mov", "0", "DWORD PTR[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");	//		move 0 into bool value					 *
																											//												 *
	//return the bool and return from function																//												 *
	methodLinear->addInstrToTail("LTE.HANDLER.END:", "", "", InstructionList::INSTR_LABEL);					//LT.HANDLER.END								 *
	methodLinear->addInstrToTail("mov", "rbp", "rsp");														//		boiler plate end stuff					 *
	methodLinear->addInstrToTail("ret");																	//		return @ r15							 *
																											//************************************************

	return *methodLinear;
}

/* Robert */
InstructionList &makeCopyIR()
{
	//register order
	//RDI, RSI, RDX, RCX
	//memcpy(void *dest, const void *src, size_t n);


	InstructionList *methodLinear = new InstructionList;
																					
	methodLinear->addNewNode();														//*************************************************************
	methodLinear->addComment("Copy ");												//							INFO PAGE						  *
	//boiler plate entry stuff														//*************************************************************
	methodLinear->addInstrToTail("mov", "rsp", "rbp");								//	boiler plate entry stuff								  *
																					//															  *
	//lookup size of the object being copied and move size into RCX					//	*This will be calling memcpy and calloc					  *
	//during the call to callc rdx would be destroyed so I use r13 to hold for now	//															  *
	methodLinear->addInstrToTail("mov", "[rbp+8]", "r13");							//	move int object from stack into rax						  *
	methodLinear->addInstrToTail("mov", "[r13+8]", "r13");							//	move int value into r13									  *
	methodLinear->addInstrToTail("imul", "8", "r13");								//	multiply r13 by 8 to get size in # of bytes				  *
																					//															  *
	//malloc setup for malloc call of size RCX										//	--PREPARE TO CALL CALLOC--								  *
	methodLinear->addInstrToTail("mov", "1", "rdi");								//	move 1 into rdi to have 1 element						  *
	methodLinear->addInstrToTail("mov", "r13", "rsi");								//	move r13*8 to have element size 						  *
	methodLinear->addInstrToTail("call", "calloc");									//	call calloc with 1 element of size r13*8 				  *
																					//	 --PREPARE FOR MEMCPY--									  *
	methodLinear->addInstrToTail("mov", "rax", "rdi");								//	move pointer returned by calloc into rdi				  *
																					//															  *
	//move r13 from holding into rdx for function call								//															  *
	methodLinear->addInstrToTail("mov", "r13", "rdx");								//	move r13 into rdx. This was done to save r13 during call  *
																					//															  *
	//move source into rsi															//															  *
	methodLinear->addInstrToTail("mov", "[rbp+8]", "rsi");							//	move source address into rsi						      *
																					//															  *
	//call memcpy																	//															  *
	methodLinear->addInstrToTail("call", "memcpy");								//	call memcpy with args rdi,rsi,rdx						  *
																					//															  *
	//return pointer to the copied object											//															  *
	methodLinear->addInstrToTail("mov", "rax", "r15");								//	move the return of memcpy into r15						  *
																					//															  *
	//boiler plate exit stuff														//															  *
	methodLinear->addInstrToTail("mov", "rbp", "rsp");								//	boiler plate exit stuff									  *
	methodLinear->addInstrToTail("ret");											//	return @ r15											  *
																					//*************************************************************
	return *methodLinear;
}


/*Originally written by: Forrest
 *Rewritten by: Robert
 * Out_String was written by forrest initially so we could do testing.
 * It was rewritten by robert later
*/
InstructionList &makeOutStringIR()
{
	InstructionList *methodLinear = new InstructionList;

	//add string to data table
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = "%s";
	string stringName = ".string" + std::to_string(stringNum);


	methodLinear->addNewNode();									//*******************************************
	methodLinear->addComment("OUT STRING");						//			     INFO PAGE					*
	//boiler plate entry stuff									//*******************************************
	methodLinear->addInstrToTail("mov", "rsp", "rbp");			//	boiler plate entry						*
																//											*
	//push the base pointer										//											*
	methodLinear->addInstrToTail("push", "rbp");				//	push the base pointer					*
																//											*
	//push string												//	--PREPARE FOR PRINTF--					*
	methodLinear->addInstrToTail("mov", "[rbp+16]", "rax");		//	move pointer to string object into rax	*
	methodLinear->addInstrToTail("mov", "[rax+24]", "rsi");		//	push pointer to string on to stack		*
																//											*
	//push format												//											*
	methodLinear->addInstrToTail("lea", stringName,"rdi");		//	push the format string on to stack		*
																//											*
	//call printf												//											*
	methodLinear->addInstrToTail("call", "printf");				//	call printf								*
																//											*
																//											*
	//restore rbp												//											*
	methodLinear->addInstrToTail("pop", "rbp");					//	pop into rbp to restore base pointer	*
																//											*
	//boiler plate exit stuff									//											*
	methodLinear->addInstrToTail("mov", "rbp", "rsp");			//	boiler plate exit						*
	methodLinear->addInstrToTail("ret");						//	return @ no return						*
																//*******************************************
	return *methodLinear;
}

InstructionList &makeInStringIR()
{
	InstructionList *methodLinear = new InstructionList;

	methodLinear->addNewNode();
	methodLinear->addComment("Function needs to be implemented");
	methodLinear->addInstrToTail("ret");

	return *methodLinear;
}

InstructionList &makeOutIntIR()
{
	InstructionList *methodLinear = new InstructionList;

	methodLinear->addNewNode();
	methodLinear->addComment("Function needs to be implemented");
	methodLinear->addInstrToTail("ret");

	return *methodLinear;
}

/* Robert */
InstructionList &makeInIntIR()
{
	//register order
	//RDI, RSI, RDX, RCX
	//calloc(size_t nitems, size_t size)
	//fgets(char *str, int n, FILE *stream)
	//sscanf(const char *str, const char *format, ...)

	InstructionList *methodLinear = new InstructionList;

	//add string to data table
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = "%ld";
	string stringName = ".string" + std::to_string(stringNum);
																				//*******************************************************
	methodLinear->addNewNode();													//*                     INFO PAGE                       *
	//boiler plate entry stuff													//*******************************************************
	methodLinear->addInstrToTail("mov", "rsp", "rbp");							//	boiler plate entry stuff							*
																				//														*
	//make new int																//														*
	makeNew(*methodLinear, "Int");												//	make new integer									*
																				//														*
	//calloc 16 bytes of memory for fgets										//	--PREPARE TO CALL CALLOC--							*
	methodLinear->addInstrToTail("mov", "1", "rdi");							//	move 1 into rdi so we have 1 element				*
	methodLinear->addInstrToTail("mov", "16", "rsi");							//	move 16 into rsi so we have 16 elements				*
	methodLinear->addInstrToTail("call", "calloc");								//	call calloc											*
																				//														*
	//save memory pointer from calloc for later									//														*
	methodLinear->addInstrToTail("push", "rax");								//	save pointer to callod'c memory for later			*
																				//														*
	//call fgets with stdin														//	--PREPARE TO CALL FGETS--							*
	methodLinear->addInstrToTail("mov", "rax", "rdi");							//	move pointer to calloc'd memory into rdi			*
	methodLinear->addInstrToTail("mov", "16", "rsi");							//	move 16 into rsi to read 16 characters				*
	methodLinear->addInstrToTail("mov", "stdin[rip]", "rdx");					//	move value for stdin into rdx						*
	methodLinear->addInstrToTail("call", "fgets");								//	call fgets											*
																				//														*
	//call sscanf on return of fgets											//	--PREPARE TO CALL SSCANF--							*
	methodLinear->addInstrToTail("pop", "rdi");									//	pop our saved pointer to calloc'd memory into rdi	*
	methodLinear->addInstrToTail("mov", "0", "rax");							//	move 0 into rax										*
	methodLinear->addInstrToTail("push", "rax");								//	push rax											*
	methodLinear->addInstrToTail("mov", "rsp", "rdx");							//	move rsp into rdx this makes a temp value for sscanf*
	methodLinear->addInstrToTail("mov", stringName, "rsi");						//	move our format string into rsi						*
	methodLinear->addInstrToTail("call", "sscanf");								//	call sscanf											*
																				//														*
	//check to make sure the return of sscanf is between INT_MAX and INT_MIN	//														*
	methodLinear->addInstrToTail("pop", "rax");									//	pop our temp value into rax							*
	methodLinear->addInstrToTail("xor", "rsi","rsi");							//	clear out register rsi 
	methodLinear->addInstrToTail("cmp", "2147483647", "rax");					//	check to see if value > INT_MAX						*
	methodLinear->addInstrToTail("cmovg", "rsi", "rax");						//	if greater than set to 0							*
	methodLinear->addInstrToTail("cmp", "-2147483648", "rax");					//	check to see if value < INT_MIN						*
	methodLinear->addInstrToTail("cmovl", "rsi", "rax");						//	if less than set to 0								*
																				//														*
	//move value into the int we made in the beginning							//														*
	methodLinear->addInstrToTail("mov", "rax", "[r15+24]");						//	move the final value into the int we created		*
																				//														*
	//boiler plate exit stuff													//														*
	methodLinear->addInstrToTail("mov", "rbp", "rsp");							// boiler plate exit stuff								*
	methodLinear->addInstrToTail("ret");										//	return @ r15										*								
																				//*******************************************************
	return *methodLinear;
}

/*
 * @author: Matt
 */ 
InstructionList &makeLengthIR()
{
	/*Idea gotten from http://www.int80h.org/strlen/ */
	InstructionList *methodLinear = new InstructionList;

	methodLinear->addNewNode();
	methodLinear->addComment("Get length of string");
	atCalleeEntry(*methodLinear);
	
	//Make ECX == max unsigned int == -1
	methodLinear->addInstrToTail("xor","rcx", "rcx");
	methodLinear->addInstrToTail("not","rcx");

	//make AL == 0
	methodLinear->addInstrToTail("xor","al","al");

	//put string pointer into rdi
	getMethodParamIntoRegister(*methodLinear, 0, "r10");
	/*CHECK ME TO MAKE SURE I'M GETTING STRING RIGHT.*/
	methodLinear->addInstrToTail("mov", "[r10+24]", "rdi");

	//clear flag
	methodLinear->addInstrToTail("cld");
	
	//Search for the first occurence of a byte == al, which is 0
	//Decreases ECX every time it scans a byte
	//Goes until it finds == al OR ECX == 0
	methodLinear->addInstrToTail("repne", "scasb");

	//String length is now in ecx - kinda. ecx == -strlen - 2
	//SO not ecx and sub 1.
	methodLinear->addInstrToTail("not","ecx");
	methodLinear->addInstrToTail("dec", "ecx");

	//Put value into int object to return
	//Store value on stack
	methodLinear->addInstrToTail("push", "rcx");

	//Make new integer
	setupMethodCall(*methodLinear, "Int..new", { "rax" });

	//Pop value into register
	methodLinear->addInstrToTail("pop", "r14");

	//Put into place in new Int object, and leave the object in r15 to return.
	methodLinear->addInstrToTail("mov", "r14", "[r15+" + to_string(DEFAULT_VAR_OFFSET) + "]");

	atCalleeExit(*methodLinear);

	return *methodLinear;
}

/*
 * @author: Matt 
 */
InstructionList &makeConcatIR()
{
	InstructionList *methodLinear = new InstructionList;

	methodLinear->addNewNode();
	methodLinear->addComment("Concatenate two strings into 1");
	atCalleeEntry(*methodLinear);

	//c calling conventions
	//RDI RSI RDX RCX
	//RETURN IN RAX

	/*get length of final string*/
	//Get self
	getMethodParamIntoRegister(*methodLinear, 0, "r10");
	//Call strlen on it
	setupMethodCall(*methodLinear, "String.length", { "r10" });
	//get the length of the string onto the stack
	methodLinear->addInstrToTail("mov", "[r15+" + to_string(DEFAULT_VAR_OFFSET) + "]", "r14");
	methodLinear->addInstrToTail("push", "r14");

	//Same as above for self, except now it's the first formal param
	getMethodParamIntoRegister(*methodLinear, 1, "r11");
	setupMethodCall(*methodLinear, "String.length", { "r11" });
	methodLinear->addInstrToTail("mov", "[r15+" + to_string(DEFAULT_VAR_OFFSET) + "]", "r14");

	methodLinear->addInstrToTail("pop", "r10");
	methodLinear->addInstrToTail("add", "r14", "r10");


	/*make space for final string*/
	callCalloc(*methodLinear, "r10", "1");

	/*copy in self*/
	methodLinear->addInstrToTail("push", "rax");
	methodLinear->addInstrToTail("mov", "rax", "rdi");
	getMethodParamIntoRegister(*methodLinear, 0, "r10");
	methodLinear->addInstrToTail("mov", "[r10+" + to_string(DEFAULT_VAR_OFFSET)+"]", "rsi");

	methodLinear->addInstrToTail("call", "strcpy");

	/*concatenate the second one*/
	//Pop the created mem, and then push to save it again.
	methodLinear->addInstrToTail("pop", "rdi #I swear we want these two functions. Trust me.");
	methodLinear->addInstrToTail("push", "rdi");
	getMethodParamIntoRegister(*methodLinear, 1, "r11");
	methodLinear->addInstrToTail("mov", "[r11 + 24]", "rsi");

	methodLinear->addInstrToTail("call", "strcat");

	/*put the new string into a string object to return*/
	//Make a new string to put this into.
	setupMethodCall(*methodLinear, "String..new", { "rax" });

	//put new string into return object
	methodLinear->addInstrToTail("pop", "r10");
	methodLinear->addInstrToTail("mov", "r10", "[r15+" + to_string(DEFAULT_VAR_OFFSET) + "]");	

	//return that new object
	atCalleeExit(*methodLinear);
	
	return *methodLinear;
}

InstructionList &makeSubstrIR()
{
	string startGreaterThanEndLabel = "SUBSTR.HANDLER.SGTE";
	string endGreaterThanStringEndLabel = "SUBSTR.HANDLER.EGTLENSTR";
	InstructionList *methodLinear = new InstructionList;

	methodLinear->addNewNode();
	methodLinear->addComment("Returns part of a string that is a certain length.");

	atCalleeEntry(*methodLinear);
	/*Check if param2 < param1 is negative. If it is, error and exit*/ 
	getMethodParamIntoRegister(*methodLinear, 1, "r10");
	methodLinear->addInstrToTail("mov", "[r10+" + to_string(DEFAULT_VAR_OFFSET) + "]", "r12");
	getMethodParamIntoRegister(*methodLinear, 2, "r11");
	methodLinear->addInstrToTail("mov", "[r11+" + to_string(DEFAULT_VAR_OFFSET) + "]", "r13");
	methodLinear->addInstrToTail("cmp", "r12", "r13");
	methodLinear->addInstrToTail("jg", startGreaterThanEndLabel);

	/*get length of self*/
	getMethodParamIntoRegister(*methodLinear, 0, "r14");
	//Save param1 and then param2, since destroyed on function call
	methodLinear->addInstrToTail("push", "r12");
	methodLinear->addInstrToTail("push", "r13");
	//Get length
	setupMethodCall(*methodLinear, "String.length", { "r14" });
	methodLinear->addInstrToTail("mov", "[r15+" + to_string(DEFAULT_VAR_OFFSET) + "]", "r15");

	/*Check if end > len(self). If it is, error and exit*/
	//compare param2 to length
	methodLinear->addInstrToTail("pop", "r8");
	methodLinear->addInstrToTail("cmp", "r8", "r15");
	methodLinear->addInstrToTail("jg", endGreaterThanStringEndLabel);

	//CHECK IF NEED TO SAVE LENGTH OF ORIGINAL STRING - I DON'T THINK SO, BUT IF you do uncomment the next line.'
	//methodLinear->addInstrToTail("push", "r15");

	/*Get address of string and add param1's int value*/
	getMethodParamIntoRegister(*methodLinear, 0, "r14");
	methodLinear->addInstrToTail("mov", "[r14+" + to_string(DEFAULT_VAR_OFFSET) + "]", "r14");
	methodLinear->addInstrToTail("add", "r8", "r15");

	/*make space the size of param2 - param1*/
	methodLinear->addInstrToTail("pop", "r9");
	methodLinear->addInstrToTail("sub", "r8", "r9");
	methodLinear->addInstrToTail("push", "r9");
	callCalloc(*methodLinear, "r9", "1");

	/*memcpy size of param2 - param1 into new space*/
	methodLinear->addInstrToTail("mov", "rax", "rdi");
	methodLinear->addInstrToTail("pop", "rdx");
	methodLinear->addInstrToTail("mov", "r15", "rsi");
	methodLinear->addInstrToTail("call", "memcpy");

	/*Make new string and put in created space*/
	methodLinear->addInstrToTail("push", "rax");
	setupMethodCall(*methodLinear, "String..new", { "rax" });
	methodLinear->addInstrToTail("pop", "r10");
	methodLinear->addInstrToTail("mov", "r10", "[r15+"+ to_string(DEFAULT_VAR_OFFSET) + "]");

	/*Return above newly created string*/
	atCalleeExit(*methodLinear);

	/*end < start error handler*/
	errorHandlerDoExit(*methodLinear, startGreaterThanEndLabel,"Substring end was less than start.");

	/*end > len(self) handler*/
	errorHandlerDoExit(*methodLinear, endGreaterThanStringEndLabel,"End value was past the end of the string.");


	return *methodLinear;
}

/*Built in function definitions end*/

/*Helper functions start */

/*
 * @author: Matt 
 */
void atCalleeEntry(InstructionList &methodLinear)
{
	methodLinear.addInstrToTail("mov", "rsp", "rbp");
}

/*
 * @author: Matt 
 */
void atCalleeExit(InstructionList &methodLinear)
{
	methodLinear.addInstrToTail("mov", "rbp", "rsp");
	methodLinear.addInstrToTail("ret");
}

/*
 * @author: Matt 
 */
void getMethodParamIntoRegister(InstructionList &methodLinear, int numParam, string placeToPut)
{
	int numOffRBP = 8 + 8*numParam;
	methodLinear.addInstrToTail("mov", "[rbp + " + to_string(numOffRBP) + "]", placeToPut);
}

/*
 * @author: Matt 
 */
void callCalloc(InstructionList &methodLinear, string paramHoldNumElements, string paramHoldSizeOfEachElement)
{
	methodLinear.addInstrToTail("mov", paramHoldNumElements, "rdi");
	methodLinear.addInstrToTail("mov", paramHoldSizeOfEachElement, "rsi");

	methodLinear.addInstrToTail("call", "calloc");
}

/*
 * @author: Mostly Robert, with a touch of Matt 
 */
void errorHandlerDoExit(InstructionList &methodLinear, string label, string error)
{
	//add label
	methodLinear.addInstrToTail(label+":", "","", InstructionList::INSTR_LABEL);

	//add string to data table
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = "ERROR: " + error;
	string stringName = ".string" + to_string(stringNum);

	//load string into rdi
	methodLinear.addInstrToTail("lea", stringName, "rdi");

	//call puts
	methodLinear.addInstrToTail("call", "puts");

	//move 1 into rdi for return val
	methodLinear.addInstrToTail("mov", "1", "rdi");

	//call exit with error code 1
	methodLinear.addInstrToTail("call", "exit");
}

/*Helper functions end */

/*
* Author: Matt, Robert, Ben
*/
InstructionList &makeMethodIR(Node *feat) 
{
	InstructionList *methodLinear = new InstructionList;

	//get expressions comprising method
	auto featureData = feat->getChildren();
	Node *formals = (Node *)featureData[1];
	Node *expression = (Node *)featureData[3];

	//method initialization
	methodInit(*methodLinear, feat);

	//go through method expressions
	makeExprIR_recursive(*methodLinear, expression);

	//method exit
	methodExit(*methodLinear, feat);

	return *methodLinear;
}

/*
* Author: Matt, Robert, Ben
*/
void methodInit(InstructionList &methodLinear, Node *feature)
{
	//do method initialization
	methodLinear.addNewNode();
	methodLinear.addComment("start method");
	methodLinear.addInstrToTail("mov", "rsp", "rbp");

	//TODO: Improve local variables so we don't have to allocate space for all of them at the same time
	//set up space for local vars
	std::string methodName = ((Node *)feature->getChildren()[0])->value;
	globalSymTable->enterScope(methodName);
	int space = globalSymTable->cur->numLocals * 8;
	globalSymTable->leaveScope();
	methodLinear.addInstrToTail("sub", std::to_string(space), "rsp");

	vars = new StackVariables;
	Node * formals = (Node *)feature->getChildren()[1];

	vars->addFormal("self");

	// add formals
	auto children = formals->getChildren();
	for (auto it = children.rbegin(); it != children.rend(); it++) {
		Node *formal = (Node *)*it;
		Node *formalID = (Node *)formal->getChildren()[0];
		vars->addFormal(formalID->value);
	}
}

/*
* Author: Matt, Robert, Ben
*/
void methodExit(InstructionList &methodLinear, Node *feature)
{
	//do method exit instructions
	methodLinear.addNewNode();
	methodLinear.addComment("End of method");

	//put result of method into return register
	methodLinear.addInstrToTail("pop", "r15");

	//remove space for local vars
	std::string methodName = ((Node *)feature->getChildren()[0])->value;
	methodLinear.addInstrToTail("mov", "rbp", "rsp");

	//call return
	methodLinear.addInstrToTail("ret");
}

/*
* Author: Matt, Robert, Ben
*/
void makeExprIR_recursive(InstructionList &methodLinear, Node *expression)
{
	switch (expression->type)
	{
	case AST_IDENTIFIER:
		doIdentifier(methodLinear, expression);
		break;
	case AST_INTEGERLITERAL:
		doIntLiteral(methodLinear, expression);
		break;
	case AST_PLUS:
		doPlus(methodLinear, expression);
		break;
	case AST_MINUS:
		doMinus(methodLinear, expression);
		break;
	case AST_TIMES:
		doMultiply(methodLinear, expression);
		break;
	case AST_DIVIDE:
		doDivide(methodLinear, expression);
		break;
	case AST_TRUE:
		doBool(methodLinear, expression, true);
		break;
	case AST_FALSE:
		doBool(methodLinear, expression, false);
		break;
	case AST_LT:
		doLessThan(methodLinear, expression);
		break;
	case AST_LE:
		doLessThanEqual(methodLinear, expression);
		break;
	case AST_EQUALS:
		doEqual(methodLinear, expression);
		break;
	case AST_STRING:
		doString(methodLinear, expression);
		break;
	case AST_TILDE:
		doTilde(methodLinear, expression);
		break;
	case AST_NOT:
		doNot(methodLinear, expression);
		break;
	case AST_EXPRSEMILIST:
		doExprSemiList(methodLinear, expression);
		break;
	case AST_NEW:
		doNew(methodLinear, expression);
		break;
	case AST_ISVOID:
		doIsVoid(methodLinear, expression);
		break;
	case AST_LARROW:
		//assignment
		doAssign(methodLinear, expression);
		break;
	case AST_WHILE:
		doWhile(methodLinear, expression);
		break;
	case AST_IF:
		doIf(methodLinear, expression);
		break;
	case AST_LET:
		doLet(methodLinear, expression);
		break;
	case AST_CASESTATEMENT:
		doCaseStatement(methodLinear, expression);
		break;
	case AST_CASE:
		break;
	case AST_DISPATCH:
		doDispatch(methodLinear, expression);
		break;
	default:
		break;
	}
}


/*
* Keyboard: Ben
* Others: everyone
* Was changed significantly (aka obsolete now)
*/
inline void makeNew(InstructionList &methodLinear, string valType)
{
	setupMethodCall(methodLinear, valType + "..new", { "rax" });
}

/*
* Keyboard: Ben
* Others: Everyone
* 
*/
void doIdentifier(InstructionList &methodLinear, Node *expression)
{
	std::string varName = expression->value;

	methodLinear.addNewNode();
	methodLinear.addComment("get variable " + varName);

	//push var onto stack
	if (vars->checkVar(varName)) {
		// local (on stack)
		int stackOffset = vars->getOffset(varName);

		//push var onto stack
		if (stackOffset < 0) {
			methodLinear.addInstrToTail("push", "[rbp-" + std::to_string(-stackOffset) + "]");
		}
		else {
			methodLinear.addInstrToTail("push", "[rbp+" + std::to_string(stackOffset) + "]");
		}

	}
	else {
		// in self
		int selfMemOffset = DEFAULT_VAR_OFFSET + (int)globalSymTable->getVariable(varName)->offset;
		int selfVarOffset = vars->getOffset("self");

		//push pointer in self onto stack
		methodLinear.addInstrToTail("mov", "[rbp+" + to_string(selfVarOffset) + "]", "r12");
		methodLinear.addInstrToTail("push", "[r12+" + to_string(selfMemOffset) + "]");
	}
}

/*
* Author: Matt, Robert, Ben
*/
void doIntLiteral(InstructionList &methodLinear, Node *expression)
{
	methodLinear.addNewNode();
	methodLinear.addComment("Make new integer with value " + expression->value);
	
	//Do integer construction
	makeNew(methodLinear, expression->valType);

	//mov the correct value into rax's allocated space
	methodLinear.addInstrToTail("mov", expression->value, "DWORD PTR [r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");
	
	//Move the ref to the new space onto the stack
	methodLinear.addInstrToTail("push", "r15");
}

/*
* Author: Matt, Robert, Ben
*/
void doPlus(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop, add, push
	methodLinear.addNewNode();
	methodLinear.addComment("Add the two integers");

	//make new Int for result (in r15)
	makeNew(methodLinear, expression->valType);

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//temporaries for add instruction
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10");

	//TODO: check the 'PTR' part
	methodLinear.addInstrToTail("add", "DWORD PTR [r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10D");

	//move result into new object
	methodLinear.addInstrToTail("mov", "r10", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "r15");
}

/*
* Author: Matt, Robert, Ben
*/
void doMinus(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop,subtract, push
	methodLinear.addNewNode();
	methodLinear.addComment("Subtract the two integers");

	//make new Int for result (in rax)
	makeNew(methodLinear, expression->valType);

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//temporaries for add instruction
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10");

	//TODO: check the 'PTR' part
	methodLinear.addInstrToTail("sub","DWORD PTR [r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10D");

	//move result into new object
	methodLinear.addInstrToTail("mov", "r10", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "r15");
}

/*
* Author: Matt, Robert, Ben
*/
void doMultiply(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop, multiply, push
	methodLinear.addNewNode();
	methodLinear.addComment("Multiply the two integers");

	//make new Int for result (in rax)
	makeNew(methodLinear, expression->valType);

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//temporaries for add instruction
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10");

	//TODO: check the 'PTR' part
	methodLinear.addInstrToTail("imul", "DWORD PTR [r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10D");

	//move result into new object
	methodLinear.addInstrToTail("mov", "r10", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "r15");
	
}

/*
* Author: Matt, Robert, Ben
*/
void doDivide(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop, divide, push
	methodLinear.addNewNode();
	methodLinear.addComment("Divide the two integers");

	//make new object (Int)
	makeNew(methodLinear, expression->valType);

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("mov", "[r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rbx");
	
	methodLinear.addInstrToTail("pop", "r12");
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rax");

	//clear edg
	methodLinear.addInstrToTail("xor", "rdx", "rdx");

	//result in rax
	methodLinear.addInstrToTail("idiv", "ebx");
	methodLinear.addInstrToTail("mov","rax","r14");
	//put div result in Int
	methodLinear.addInstrToTail("mov", "r14", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "r15");
	
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doBool(InstructionList &methodLinear, Node *expression, bool val)
{
	methodLinear.addNewNode();
	methodLinear.addComment("Make new bool with value: " + std::to_string(val));

	//Do Bool construction
	makeNew(methodLinear, expression->valType);

	//mov the correct value into rax's allocated space
	methodLinear.addInstrToTail("mov", std::to_string(val), "DWORD PTR[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	//Move the ref to the new space onto the stack
	methodLinear.addInstrToTail("push", "r15");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doLessThan(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop, LT, push
	methodLinear.addNewNode();
	methodLinear.addComment("Compare two integers LT");

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//TODO: write lessthan handler
	setupMethodCall(methodLinear, "LT..Handler", { "r13", "r12" });

	methodLinear.addInstrToTail("push", "r15");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doLessThanEqual(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop, LT, push
	methodLinear.addNewNode();
	methodLinear.addComment("Compare two integers LTE");

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//TODO: write lessthanEqual handler
	setupMethodCall(methodLinear, "LTE..Handler", { "r13", "r12" });

	methodLinear.addInstrToTail("push", "r15");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doEqual(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node *)children[0]);
	makeExprIR_recursive(methodLinear, (Node *)children[1]);

	//at the very end
	//pop, pop, LT, push
	methodLinear.addNewNode();
	methodLinear.addComment("Compare two objects Equal");

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//TODO: write EQ handler
	setupMethodCall(methodLinear, "EQ..Handler", { "r13", "r12" });

	methodLinear.addInstrToTail("push", "r15");
}

/*
 * Forest, Benji, Robert, Ben, Matt
 */
void doString(InstructionList &methodLinear, Node *expression)
{
	methodLinear.addNewNode();
	methodLinear.addComment("Make new String with value: " + expression->value);


	//Do String construction
	makeNew(methodLinear, "String");

	//add string to data table
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = expression->value;
	string stringName = ".string" + std::to_string(stringNum);

	//mov the string ptr into new object
	methodLinear.addInstrToTail("lea", stringName, "rax");
	methodLinear.addInstrToTail("mov", "rax", "[r15+" + to_string(DEFAULT_VAR_OFFSET) + "]");

	//Move the ref to the new obj onto the stack
	methodLinear.addInstrToTail("push", "r15");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doTilde(InstructionList &methodLinear, Node *expression)
{
	//Get whatever we're supposed to negate
	makeExprIR_recursive(methodLinear, (Node *)expression->getChildren()[0]);

	methodLinear.addNewNode();
	methodLinear.addComment("Negate an integer");
	
	//Put the reference in a register
	methodLinear.addInstrToTail("pop", "rbx");

	//put the value into rax
	methodLinear.addInstrToTail("mov", "[rbx+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rax");

	//not the value and add 1 (2's complement)
	methodLinear.addInstrToTail("not", "eax");

	methodLinear.addInstrToTail("inc", "eax");

	makeNew(methodLinear, expression->valType);

	//put back
	methodLinear.addInstrToTail("mov", "rax", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "rbx");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doNot(InstructionList &methodLinear, Node *expression)
{

	//Get whatever we're supposed to not
	makeExprIR_recursive(methodLinear, (Node *)expression->getChildren()[0]);

	methodLinear.addNewNode();
	methodLinear.addComment("Not-ing a bool with value" + expression->value);

	//Get the value into a regeister
	methodLinear.addInstrToTail("pop", "rbx");

	methodLinear.addInstrToTail("mov", "[rbx+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rax");

	//XOR the value with 1 - 0^1 = 1, 1^1 = 0
	methodLinear.addInstrToTail("xor", "1", "eax");

	makeNew(methodLinear, expression->valType);

	//Put it back
	methodLinear.addInstrToTail("mov", "rax", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "r15");

}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doExprSemiList(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	size_t size = children.size();
	methodLinear.addNewNode();
	methodLinear.addComment("Start of new block expressions.");

	for (size_t i = 0; i < size - 1; i++) {
		makeExprIR_recursive(methodLinear, (Node *)children[i]);
		methodLinear.addInstrToTail("pop", "rax");
	}

	makeExprIR_recursive(methodLinear, (Node *)children[size - 1]);


	methodLinear.addNewNode();
	methodLinear.addComment("End of block expressions.");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doNew(InstructionList &methodLinear, Node *expression)
{
	methodLinear.addNewNode();
	methodLinear.addComment("Making a new object of type " + expression->valType);
	
	makeNew(methodLinear, expression->valType);
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doIsVoid(InstructionList &methodLinear, Node *expression)
{
	//Get whatever we're supposed to not
	makeExprIR_recursive(methodLinear, (Node *)expression->getChildren()[0]);

	methodLinear.addNewNode();
	methodLinear.addComment("Check if pointer is void");
	
	//get pointer
	methodLinear.addInstrToTail("pop", "rbx");

	methodLinear.addInstrToTail("cmp", "0", "rbx");

	methodLinear.addInstrToTail("sete", "rax");

	makeNew(methodLinear, "Boolean");
	
	//Put it back
	methodLinear.addInstrToTail("mov", "rax", "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

	methodLinear.addInstrToTail("push", "r15");
}

/*
* Forest, Benji, Robert, Ben, Matt
*/
void doWhile(InstructionList &methodLinear, Node *expression) {
	int countSave = whileLabelCount;

	methodLinear.addNewNode();
	methodLinear.addComment("Start of while loop" + std::to_string(countSave));
	whileLabelCount++;
	//add label for jump to redo loop
	methodLinear.addInstrToTail("While_Condition" + std::to_string(countSave) + ":", "", "", InstructionList::INSTR_LABEL);
	
	//write code for condition
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear,(Node*)children[0]);
	methodLinear.addInstrToTail("pop", "rax");
	methodLinear.addInstrToTail("mov", "[rax+" + std::to_string(DEFAULT_VAR_OFFSET) + "]","rax");
	methodLinear.addInstrToTail("cmp", "rax",std::to_string(false));

	//add jump to end of while if condition fails
	methodLinear.addInstrToTail("je", "While_End" + std::to_string(countSave));

	//add code for expression
	makeExprIR_recursive(methodLinear, (Node*)children[1]);
	methodLinear.addInstrToTail("pop", "rax");

	//add jump to go back to redo condition and maybe loop
	methodLinear.addInstrToTail("jmp", "While_Condition" + std::to_string(countSave));

	//add label to jump to if condition fails and push 0 since while loops return void
	methodLinear.addInstrToTail("While_End" + std::to_string(countSave) + ":", "", "", InstructionList::INSTR_LABEL);
	methodLinear.addInstrToTail("push", "0");
}

/*
* Keyboard: Robert
* Others: everyone
* 
*/
void doIf(InstructionList &methodLinear, Node *expression) {
	int countSave = ifLabelCount;

	methodLinear.addNewNode();
	methodLinear.addComment("Start of if statement" + std::to_string(countSave));
	ifLabelCount++;

	//write code for conditional
	auto children = expression->getChildren();
	makeExprIR_recursive(methodLinear, (Node*)children[0]);
	methodLinear.addInstrToTail("pop", "rax");
	methodLinear.addInstrToTail("mov", "[rax+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rax");
	methodLinear.addInstrToTail("cmp", std::to_string(false), "rax");
	
	//write jump to go to else
	methodLinear.addInstrToTail("je", "If_Else" + std::to_string(countSave));

	//write code for then
	makeExprIR_recursive(methodLinear, (Node*)children[1]);

	//write jump to go to end
	methodLinear.addInstrToTail("jmp", "If_End" + std::to_string(countSave));

	//write label for else
	methodLinear.addInstrToTail("If_Else" + std::to_string(countSave) + ":", "", "", InstructionList::INSTR_LABEL);

	//write code for else
	makeExprIR_recursive(methodLinear, (Node*)children[2]);

	//write label for end
	methodLinear.addInstrToTail("If_End" + std::to_string(countSave) + ":", "", "", InstructionList::INSTR_LABEL);
}

/*
* Keyboard: Matt
* Others: everyone
* 
*/
void doAssign(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	Node *assignID = (Node *)children[0];
	Node *assignExpr = (Node *)children[1];
	std::string &varName = assignID->value;

	//run expression
	makeExprIR_recursive(methodLinear, assignExpr);

	methodLinear.addNewNode();
	methodLinear.addComment("Assign " + varName);
	//pop result into rax
	methodLinear.addInstrToTail("pop", "rax");
	//assign result to correct var
	if (vars->checkVar(varName)) {
		// local (on stack)
		int stackOffset = vars->getOffset(varName);

		//set result to correct var in stack
		if (stackOffset < 0) {
			methodLinear.addInstrToTail("mov", "rax", "[rbp-" + std::to_string(-stackOffset) + "]");
		}
		else {
			methodLinear.addInstrToTail("mov", "rax", "[rbp+" + std::to_string(stackOffset) + "]");
		}

	}
	else {
		// in self
		int selfMemOffset = DEFAULT_VAR_OFFSET + (int)globalSymTable->getVariable(varName)->offset;
		int selfVarOffset = vars->getOffset("self");

		//get self ptr
		methodLinear.addInstrToTail("mov", "[rbp+" + to_string(selfVarOffset) + "]", "r12");
		//move result into self at var offset
		methodLinear.addInstrToTail("mov", "rax", "[r12+" + std::to_string(selfMemOffset) + "]");
	}
	methodLinear.addInstrToTail("push", "rax");
}

/*
* Keyboard: Ben
* Others: everyone
* 
*/
void doLet(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();

	methodLinear.addNewNode();
	methodLinear.addComment("Start of let");

	Node *idTypeExpr = (Node *)children[0];

	//do idTypeExpr first
	auto typeExprKids = idTypeExpr->getChildren();
	string &varName = ((Node *)typeExprKids[0])->value;
	string &varType = ((Node *)typeExprKids[1])->value;
	Node *varExpr = (Node *)typeExprKids[2];

	//recurse into idTypeExpr expression
	makeExprIR_recursive(methodLinear, varExpr);
	vars->addVar(varName);
	if (varExpr->type != AST_NULL) {
		//assign to variable, already on the stack from recursive call
		//pop and move result into variable position
		methodLinear.addInstrToTail("pop", "r12");
		methodLinear.addInstrToTail("mov", "r12", "[rbp-" + std::to_string(-vars->getOffset(varName)) + "]");
	}
	else {
		//init value to void
		methodLinear.addInstrToTail("mov", "0", "[rbp-" + std::to_string(-vars->getOffset(varName)) + "]");
	}

	methodLinear.addNewNode();
	methodLinear.addComment("start of let in expr");

	//recurse to expr
	Node *letExpr = (Node *)children[1];
	makeExprIR_recursive(methodLinear, letExpr);

	//remove variable from locals
	methodLinear.addNewNode();
	methodLinear.addComment("end of let expr");

	vars->removeVar(varName);
}

/*
* Keyboard: Forest
* Others:
* 
*/
void doDispatch(InstructionList &methodLinear, Node *expression)
{
	auto children = expression->getChildren();
	Node *caller = (Node *)children[0];
	Node *stattype = (Node *)children[1];
	Node *method = (Node *)children[2];
	Node *exprs = (Node *)children[3];

	methodLinear.addNewNode();
	methodLinear.addComment("Start of call to method " + method->value);

	//evaluate parameters left to right (cool manual p27)
	//TODO: optimize for low number of inputs
	children = exprs->getChildren();
	int params = children.size();
	Node *chld;
	vector<string> paramlist;
	for (auto tchild : children) {
		chld = (Node *)tchild;
		makeExprIR_recursive(methodLinear, chld);
	}
	
	//save current stack offset, and give parameters as locations relative to that
	for (int i = params-1; i >= 0; i--) {
		paramlist.push_back("[rcx+" + to_string(i*8) + "]");
	}
	std::reverse(paramlist.begin(), paramlist.end());


	int vtableOffset;
	string type;
	//Caller will be evaluated AFTER parameters (cool manual p27)
	//put caller into r12
	if (caller->type != AST_NULL) { //must evaluate caller
		makeExprIR_recursive(methodLinear, caller);
		methodLinear.addInstrToTail("pop", "r12");
	}
	else { //caller is self
		methodLinear.addInstrToTail("mov", "[rbp+8]", "r12");
	}
	paramlist.push_back("r12");

	//mov function location into rax
	if (stattype->type == AST_NULL) { //dynamic dispatch
		if (caller->type == AST_NULL || caller->valType == "SELF_TYPE") { //get offset from current class
			vtableOffset = 8*globalVTable->getOffset(globalSymTable->cur->name, method->value);
		}
		else {//get offset from static type
			vtableOffset = 8*globalVTable->getOffset(caller->valType, method->value);
		}
		methodLinear.addInstrToTail("mov", "[r12+16]", "rbx");//16 holds vtable pointer
		methodLinear.addInstrToTail("mov", "[rbx+" + to_string(vtableOffset) + "]", "rax");
	}
	else { //static dispatch
		methodLinear.addInstrToTail("lea", stattype->valType + "..vtable", "rbx");
		vtableOffset = 8*globalVTable->getOffset(stattype->valType, method->value);
		methodLinear.addInstrToTail("mov", "[rbx+" + to_string(vtableOffset) + "]", "rax");
	}

	methodLinear.addInstrToTail("mov", "rsp", "rcx"); //need to store this
	setupMethodCall(methodLinear, "rax", paramlist);

	methodLinear.addInstrToTail("add", to_string(8*params), "rsp");
	methodLinear.addInstrToTail("push", "r15");

	methodLinear.addNewNode();
	methodLinear.addComment("End of function call to " + method->value);
}

/*
* Authors: Forest, Ben
*/
void doCaseStatement(InstructionList &methodLinear, Node *expression)
{
	int caseLabelSave = caseLabelCount++;
	methodLinear.addNewNode();
	methodLinear.addComment("start case" + to_string(caseLabelSave));

	auto children = expression->getChildren();
	Node *caseExpr = (Node *)children[0];
	Node *caseList = (Node *)children[1];
	makeExprIR_recursive(methodLinear, caseExpr);
	methodLinear.addInstrToTail("pop", "rax");
	methodLinear.addInstrToTail("mov", "[rax]", "rbx");

	//sort cases and grab ids
	vector<string> varNames;
	vector<Node *> cases;
	for (auto tchld : caseList->getChildren()) {
		Node *chld = (Node *)tchld;
		cases.push_back(chld);
		varNames.push_back(((Node *)chld->getChildren()[0])->value);
	}
	
	methodLinear.addInstrToTail("lea", "case" + to_string(caseLabelSave) + "_table", "r12");
	methodLinear.addInstrToTail("jmp", "[r12+rbx*8+0]");
	//case#_table (for jmp table)
	methodLinear.addInstrToTail("case" + to_string(caseLabelSave) + "_table:", "", "", InstructionList::INSTR_LABEL);

	auto  cmpr = [](Node *a, Node *b) -> bool {
		Node *atype = (Node *)a->getChildren()[1];
		int atag = globalSymTable->getClassTag(atype->value);
		Node *btype = (Node *)b->getChildren()[1];
		int btag = globalSymTable->getClassTag(btype->value);
		return btag < atag;
	};
	std::sort(cases.begin(), cases.end(), cmpr);

	auto caseType = [](Node *n) -> string {
		Node *ntype = (Node *)n->getChildren()[1];
		return ntype->value;
	};

	//setup jump table
	vector<string> jmpTable;
	vector<string> allTypes;
	for (auto type : globalTypeList) {
		allTypes.push_back(type.first);
	}
	auto jmpTableCmp = [](string a, string b) -> bool {
		string aClass = a.substr(a.find("_") + 1);
		string bClass = b.substr(b.find("_") + 1);
		return globalSymTable->getClassTag(aClass) < globalSymTable->getClassTag(bClass);
	};
	std::sort(allTypes.begin(), allTypes.end(), jmpTableCmp);

	for (auto type : allTypes) {
		string tag = "";
		for (Node *cs : cases) {
			string cType = caseType(cs);
			if (globalSymTable->isSubClass(type, cType)) {
				tag = "case" + to_string(caseLabelSave) + "_" + cType;
				break;
			}
		}
		if (tag == "") {
			tag = "case_error";
		}

		jmpTable.push_back(tag);
	}

	//put table directly into assembly
	for (string tag : jmpTable) {
		methodLinear.addInstrToTail(".quad", tag);
	}

	cerr << "";
	//each expression (with label)
	for (Node *cs : cases) {
		string caseId = ((Node *)cs->getChildren()[0])->value;
		auto tExpr = cs->getChildren()[2];
		Node *expr = (Node *)tExpr;
		methodLinear.addInstrToTail("case" + to_string(caseLabelSave) + "_" + caseType(cs) + ":", "", "", InstructionList::INSTR_LABEL);
		//add offset info
		vars->addVar(caseId);
		//put rax into case variable name offset on stack
		methodLinear.addInstrToTail("mov", "rax", "[rbp-" + to_string(-vars->getOffset(caseId)) + "]");

		makeExprIR_recursive(methodLinear, expr);
		//remove scoping
		vars->removeVar(caseId);
		methodLinear.addInstrToTail("jmp", "case" + to_string(caseLabelSave) + "_end");
	}

	//case#_end
	methodLinear.addNewNode();
	methodLinear.addComment("case" + to_string(caseLabelSave) + " END");
	methodLinear.addPreLabel("case" + to_string(caseLabelSave) + "_end:");
}

/*
* 
*/
InstructionList &makeCaseErrorIR() 
{
	InstructionList*caseErr = new InstructionList;
	//errorHandlerDoExit(*caseErr, "case_error", "Case without matching branch");
	return *caseErr;
}

/*
* Keyboard: Ben
* Others: Matt, Robert, Forest
* 
*/
//call with arguments rights to left ex: func(a, b, c) would be: setupMethodCall( , , <c, b, a>)
void setupMethodCall(InstructionList &methodLinear, string methodName, vector<string> formals)
{
	methodLinear.addInstrToTail("push", "rbp");

	for(string formal : formals)
	{
		methodLinear.addInstrToTail("push", formal);
	}

	methodLinear.addInstrToTail("call", methodName);
	methodLinear.addInstrToTail("add", to_string(8 * formals.size()), "rsp");
	methodLinear.addInstrToTail("pop", "rbp");
}
