#include "linearIR.h"

InstructionList &makeMethodIR(Node *feat);
void makeExprIR_recursive(InstructionList &methodLinear, Node *expression);
void methodInit(InstructionList &methodLinear);
void methodExit(InstructionList &methodLinear);
void doIntLiteralCode(InstructionList &methodLinear, Node *expression);
void doPlus(InstructionList &methodLinear, Node *expression);
void doMinus(InstructionList &methodLinear, Node *expression);
void doMultiply(InstructionList &methodLinear, Node *expression);
void doDivide(InstructionList &methodLinear, Node *expression);

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
		//
		break;
	case AST_INTEGERLITERAL:
		doIntLiteralCode(methodLinear, expression);
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

	default:
		break;
	}
}

/*
* Author: Matt, Robert, Ben
*/
void doIntLiteralCode(InstructionList &methodLinear, Node *expression)
{
	methodLinear.addNewNode();
	InstructionList::Instruction instr;
	methodLinear.addComment("Make new integer with value " + expression->value);
	//TODO SWITCH TO INTEGER CONSTRUCTOR
	//How much space we want
	instr.set("mov", "24", "rdi");
	methodLinear.addInstrToTail(instr);

	//call Malloc to get space
	instr.set("call", "malloc");
	methodLinear.addInstrToTail(instr);

	//mov the correct value into rax's allocated space
	instr.set("mov", expression->value, "[rax]");
	methodLinear.addInstrToTail(instr);
	
	//Move the ref to the new space onto the stack
	instr.set("push", "rax");
	methodLinear.addInstrToTail(instr);
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
	InstructionList::Instruction instr;
	methodLinear.addComment("Add the two integers");

	//get the two values
	instr.set("pop", "r9");
	methodLinear.addInstrToTail(instr);

	instr.set("pop", "r8");
	methodLinear.addInstrToTail(instr);

	instr.set("add", "DWORD PTR [r9+16]", "DWORD PTR [r8+16]");
	methodLinear.addInstrToTail(instr);

	instr.set("push", "r8");
	methodLinear.addInstrToTail(instr);
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
	InstructionList::Instruction instr;
	methodLinear.addComment("Add the two integers");

	//get the two values
	instr.set("pop", "r9");
	methodLinear.addInstrToTail(instr);

	instr.set("pop", "r8");
	methodLinear.addInstrToTail(instr);


	instr.set("sub", "DWORD PTR [r9+16]", "DWORD PTR [r8+16]");
	methodLinear.addInstrToTail(instr);

	instr.set("push", "r8");
	methodLinear.addInstrToTail(instr);
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
	InstructionList::Instruction instr;
	methodLinear.addComment("Add the two integers");

	//get the two values
	instr.set("pop", "r9");
	methodLinear.addInstrToTail(instr);

	instr.set("pop", "r8");
	methodLinear.addInstrToTail(instr);

	instr.set("imul", "DWORD PTR [r9+16]", "DWORD PTR [r8+16]");
	methodLinear.addInstrToTail(instr);

	instr.set("push", "r8");
	methodLinear.addInstrToTail(instr);
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
	InstructionList::Instruction instr;
	methodLinear.addComment("Add the two integers");

	//get the two values
	instr.set("pop", "r9");
	methodLinear.addInstrToTail(instr);
	
	instr.set("mov", "[r9+16]", "rbx");
	methodLinear.addInstrToTail(instr);

	instr.set("pop", "r8");
	methodLinear.addInstrToTail(instr);

	instr.set("mov", "[r8+16]", "rax");
	methodLinear.addInstrToTail(instr);

	instr.set("idiv", "ebx");
	methodLinear.addInstrToTail(instr);

	instr.set("mov","rax","[r8+16]");
	methodLinear.addInstrToTail(instr);

	instr.set("push", "r8");
	methodLinear.addInstrToTail(instr);
}