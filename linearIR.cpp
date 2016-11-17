#include "linearIR.h"

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
InstructionList &makeVTableIR();
InstructionList &makeStringsIR();

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

void objectInit(InstructionList &classLinear, string name, int tag, size_t size);

/*
 * Authors: Matt, Robert, and Ben
 */
unordered_map<string,InstructionList &> *makeLinear() 
{


	unordered_map<string, InstructionList &> *retMap = new unordered_map<string,InstructionList &>;

	retMap->emplace("global..vtable", makeVTableIR());

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

	retMap->emplace("_start", makeEntryPointIR());

	//Default classes
	retMap->emplace("Int..new", makeIntIR());
	/*todo*/
	//retMap->emplace("IO..new", makeIOIR());
	//retMap->emplace("Object..new", makeObjectIR());
	//retMap->emplace("String..new", makeObjectIR());
	//retMap->emplace("Bool..new", makeBoolIR());


	retMap->emplace(";data section", makeStringsIR());

	return retMap;
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
	entry->addInstrToTail("push", "rbp");
	entry->addInstrToTail("push", "r15");
	entry->addInstrToTail("call", "Main.main");
	
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

	int tag = 0; //TODO get an actual tag

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
					max = var->offset;
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
	classLinear.addInstrToTail("mov", name + "..vtable", "rax");
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


	int tag = 0; //TODO get an actual tag
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
* author: everyone
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


InstructionList &makeStringsIR()
{
	InstructionList *stringIR = new InstructionList;
	stringIR->addNewNode();
	stringIR->addComment("string constants");
	stringIR->addInstrToTail(".data", "", "", InstructionList::INSTR_LABEL);

	string name;
	for (auto vtableEntry : globalVTable->vtable)
	{
		name = vtableEntry.second[0];
		stringIR->addInstrToTail(name, "", "", InstructionList::INSTR_LABEL);
		stringIR->addInstrToTail(".ascii", "\"" + vtableEntry.first + "\"");
	}

	for (int i = 0; i < globalStringTable.size(); i++)
	{
		name = globalStringTable[i];
		stringIR->addInstrToTail(".string" + std::to_string(i) + ":", "", "", InstructionList::INSTR_LABEL);
		stringIR->addInstrToTail(".ascii", "\"" + name + "\"");
	}


	return *stringIR;
}

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
	methodLinear.addInstrToTail("mov", std::to_string(space), "r12");
	methodLinear.addInstrToTail("sub", "rsp", "r12");
	methodLinear.addInstrToTail("mov", "r12", "rsp");

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
	globalSymTable->enterScope(methodName);
	int space = globalSymTable->cur->numLocals * 8;
	globalSymTable->leaveScope();
	methodLinear.addInstrToTail("mov", std::to_string(space), "r12");
	methodLinear.addInstrToTail("add", "rsp", "r12");
	methodLinear.addInstrToTail("mov", "r12", "rsp");

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
		break;
	case AST_CASE:
		break;
	default:
		break;
	}
}


void makeNew(InstructionList &methodLinear, string valType)
{
	//Do object construction
	methodLinear.addInstrToTail("push", "rbp");

	//Add space at rbp+8 for self object
	methodLinear.addInstrToTail("add", "8", "rsp");

	methodLinear.addInstrToTail("call", valType + "..new"); //type constructor

	methodLinear.addInstrToTail("pop", "rax");

	methodLinear.addInstrToTail("pop", "rbp");
}

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
		int selfMemOffset = DEFAULT_VAR_OFFSET + globalSymTable->getVariable(varName)->offset;
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
	methodLinear.addInstrToTail("mov", expression->value, "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");
	
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

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//make new Int for result (in r15)
	makeNew(methodLinear, expression->valType);

	//temporaries for add instruction
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10");

	//TODO: check the 'PTR' part
	methodLinear.addInstrToTail("add", "DWORD PTR [r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "DWORD r10");

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

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//make new Int for result (in rax)
	makeNew(methodLinear, expression->valType);

	//temporaries for add instruction
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10");

	//TODO: check the 'PTR' part
	methodLinear.addInstrToTail("sub", "DWORD PTR [r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "DWORD r10");

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

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("pop", "r12");

	//make new Int for result (in rax)
	makeNew(methodLinear, expression->valType);

	//temporaries for add instruction
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "r10");

	//TODO: check the 'PTR' part
	methodLinear.addInstrToTail("imul", "DWORD PTR [r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "DWORD r10");

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

	//get the two values
	methodLinear.addInstrToTail("pop", "r13");
	methodLinear.addInstrToTail("mov", "[r13+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rbx");
	
	methodLinear.addInstrToTail("pop", "r12");
	methodLinear.addInstrToTail("mov", "[r12+" + std::to_string(DEFAULT_VAR_OFFSET) + "]", "rax");

	//result in rax
	methodLinear.addInstrToTail("idiv", "ebx");
	methodLinear.addInstrToTail("mov","rax","r14");

	//make new object (Int)
	makeNew(methodLinear, expression->valType);
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
	methodLinear.addInstrToTail("mov", std::to_string(val), "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

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
	methodLinear.addInstrToTail("push", "rbp");
	methodLinear.addInstrToTail("call", "LT..Handler");
	methodLinear.addInstrToTail("pop", "rbp");

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
	methodLinear.addInstrToTail("push", "rbp");
	methodLinear.addInstrToTail("call", "LTE..Handler");
	methodLinear.addInstrToTail("pop", "rbp");

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

	//TODO: write lessthan handler
	methodLinear.addInstrToTail("push", "rbp");
	methodLinear.addInstrToTail("call", "Equal..Handler");
	methodLinear.addInstrToTail("pop", "rbp");

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
	makeNew(methodLinear, expression->valType);

	//add string to data table
	size_t stringNum = globalStringTable.size();
	globalStringTable[stringNum] = expression->value;
	string stringName = ".string" + std::to_string(stringNum);

	//mov the string ptr into new object
	methodLinear.addInstrToTail("mov", stringName, "[r15+" + std::to_string(DEFAULT_VAR_OFFSET) + "]");

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
	methodLinear.addInstrToTail("cmp", "rax", std::to_string(false));
	
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
		int selfMemOffset = DEFAULT_VAR_OFFSET + globalSymTable->getVariable(varName)->offset;
		int selfVarOffset = vars->getOffset("self");

		//get self ptr
		methodLinear.addInstrToTail("mov", "[rbp+" + to_string(selfVarOffset) + "]", "r12");
		//move result into self at var offset
		methodLinear.addInstrToTail("mov", "rax", "[r12+" + std::to_string(selfMemOffset) + "]");
	}
}

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