#include "linearIR.h"

// IMPORTANT REGISTER INFO
// volatile registers: (destroyed on function call)
// rax, rcx, rdx, r8, r9, r10, r11
// nonvolatile registers: 
// rbx, rbp, rdi, rsi, rsp, r12, r13, r14, r15


InstructionList &makeMethodIR(Node *feat);
void makeNew(InstructionList &methodLinear, string valType);
void makeExprIR_recursive(InstructionList &methodLinear, Node *expression);
void methodInit(InstructionList &methodLinear);
void methodExit(InstructionList &methodLinear);
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

/*
 * Authors: Matt, Robert, and Ben
 */
unordered_map<string,InstructionList &> *makeLinear() 
{

	unordered_map<string, InstructionList &> *retMap = new unordered_map<string,InstructionList &>;

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
				globalSymTable->enterScope(methName);
				//(*retMap)[tmp] = makeMethodIR(feature);
				retMap->emplace(tmp, makeMethodIR(feature));
				globalSymTable->leaveScope();
			}
		}
		//Go through each method

	}

	return retMap;
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
	methodInit(*methodLinear);
	
	//go through method expressions
	makeExprIR_recursive(*methodLinear, expression);

	//method exit
	methodExit(*methodLinear);

	return *methodLinear;
}

/*
* Author: Matt, Robert, Ben
*/
void methodInit(InstructionList &methodLinear)
{
	//do method initialization
}

/*
* Author: Matt, Robert, Ben
*/
void methodExit(InstructionList &methodLinear)
{
	//do method exit instructions
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

		break;
	default:
		break;
	}
}


void makeNew(InstructionList &methodLinear, string valType)
{
	//Do object construction
	methodLinear.addInstrToTail("push", "rbp");

	methodLinear.addInstrToTail("call", valType + "..new"); //type constructor

	methodLinear.addInstrToTail("pop", "rbp");
}

void doIdentifier(InstructionList &methodLinear, Node *expression)
{

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



