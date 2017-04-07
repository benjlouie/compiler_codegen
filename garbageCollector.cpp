/**
*	This file contains all the necassary functions to create the garbage collector.
*	The functions here are based on the garbage.c file in the gc_design folder
*	Because this file is basically making our compiler print out assembly we converted from c
*	the author of the assembly is credited under assembly in function headers. Author refers to
*	the person who wrote the actual code in the function
*/

#include "garbageCollector.h"

void initGC(InstructionList &gcIR);
void addRef(InstructionList &gcIR);
void collectAndResize(InstructionList &gcIR);
void clearAll(InstructionList &gcIR);
void transfer(InstructionList &gcIR);
void insert(InstructionList &gcIR);
void get(InstructionList &gcIR);
void setGreys(InstructionList &gcIR);
void removeNonWhite(InstructionList &gcIR);
void freeSent(InstructionList &gcIR);
void makeNode(InstructionList &gcIR);
void pop(InstructionList &gcIR);
void push(InstructionList &gcIR);
void getGreySet(InstructionList &gcIR);
void hashFun(InstructionList &gcIR);
void makeQNode(InstructionList &gcIR);

InstructionList & makeGarbageCollectorIR()
{
	InstructionList *gcIR = new InstructionList;

	gcIR->addNewNode();
	gcIR->addComment("Initializes the Garbage collector");
	initGC(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Adds a reference for the garbage collector to keep track of");
	addRef(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Collects unreachable memory and resizes the garbage collectors table");
	collectAndResize(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Frees all memory that the garbage collector is keeping track of, and all memory allocated by the GC");
	clearAll(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Transfers black set items from one table to another");
	transfer(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Inserts a node into a table");
	insert(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("gets a node from a table");
	get(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Finds all addresses we have a reference to and sets them to grey in the table");
	setGreys(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Removes a non white node from a sentinel");
	removeNonWhite(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("frees a sentinel and all data associated with it");
	freeSent(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Creates a new Node");
	makeNode(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Pops from a queue");
	pop(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Push a node into the queue");
	push(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Finds all grey nodes in the table, and puts them into a queue");
	getGreySet(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Simple hash function");
	hashFun(*gcIR);

	gcIR->addNewNode();
	gcIR->addComment("Makes a node for use in Queues");
	makeQNode(*gcIR);

	return *gcIR;
}

/**
*  author: Forest
*  assembly: Robert
*  This function intializes an empty garbage collector.
*/
void initGC(InstructionList & gcIR)
{
	gcIR.addInstrToTail("initGC:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/* call calloc(24, 1) */
	gcIR.addInstrToTail("mov", "1", "rdi");
	gcIR.addInstrToTail("mov", MAP_SIZE, "rsi");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("call", "calloc");
	gcIR.addInstrToTail("pop", "rbp");

	/* move calloc'd memory into m->table_size */
	gcIR.addInstrToTail("mov", PARAM_1, "rsi");
	gcIR.addInstrToTail("mov", "rsi", "[rax]");
	
	/*Save m */
	gcIR.addInstrToTail("push", "rax");

	/* call calloc(16, table_size) note: rsi holds table_size */
	gcIR.addInstrToTail("mov", SENT_SIZE, "rdi");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("call", "calloc");
	gcIR.addInstrToTail("pop", "rbp");

	/* move calloc'd memory to m->*/
	gcIR.addInstrToTail("pop", "r8");
	gcIR.addInstrToTail("mov", "rax", "[r8+16]");
	gcIR.addInstrToTail("mov", "r8", "rax");

	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  assembly: Robert
*  This function is stuffed inside our helper function for calloc. Every time calloc is called a reference is added 
*  to the garbage collector. This is how we keep track of all our calls to calloc.
*/
void addRef(InstructionList & gcIR)
{
	gcIR.addInstrToTail("addRef:", "", "", InstructionList::INSTR_LABEL);


	atCalleeEntry(gcIR);

	/*call makeNode(PARAM_2, PARAM_3) */
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", PARAM_3);
	gcIR.addInstrToTail("push", PARAM_2);
	gcIR.addInstrToTail("call", "makeNode");
	gcIR.addInstrToTail("add", "16", "rsp"); //stack grows down (changed from Robert's code)
	gcIR.addInstrToTail("pop", "rbp"); 

	/*call insert(PARAM_1, rax) */
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "insert");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/* multiple m->num_elems by 100 */
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax+8]", "rax"); //move m->num_elems into rax
	gcIR.addInstrToTail("xor", "r8", "r8");
	gcIR.addInstrToTail("mov", "100", "r8");
	gcIR.addInstrToTail("mul", "r8"); //rax *= r8

	/*rax /= m->table_size */
	gcIR.addInstrToTail("mov", PARAM_1, "rcx");
	gcIR.addInstrToTail("mov", "[rcx]", "rcx");
	gcIR.addInstrToTail("xor", "rdx", "rdx");
	gcIR.addInstrToTail("div", "rcx");

	/*compare rax to 75, jump less than*/
	gcIR.addInstrToTail("cmp", "75", "rax");
	gcIR.addInstrToTail("jle", "LFngtLF_MAX");

	/* call collectAndResize(PARAM_1)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", PARAM_1);
	//gcIR.addInstrToTail("call", "collectAndResize");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	//gcIR.addInstrToTail("mov", "rax", "[rbp+8]");

	/*exit function*/
	gcIR.addInstrToTail("LFngtLF_MAX:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "[rbp+8]", "rax");
	atCalleeExit(gcIR);
}

/**
* Author: Forest
* This function is not used because we were never able debug is properly. This function would allow the garbage collector to be called during runtime.
* This function will search through the list of calloc references and remove any node which is unused in the rest of the program. In theory this would 
* allow us to do garbage collection during runtime safely but we were never able to get this to work properly.
*/
void collectAndResize(InstructionList & gcIR)
{
	gcIR.addInstrToTail("collectAndResize:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*add space for storing gs at rbp-8*/
	gcIR.addInstrToTail("sub", "8", "rsp");

	/*call setGreys(param_1)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "setGreys");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*call getGreySet(param_1)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "getGreySet");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*move return value into r8*/
	gcIR.addInstrToTail("mov", "rax", "r8");

	/*start of while loop*/
	gcIR.addInstrToTail("collectAndResize_WHILE:", "", "", InstructionList::INSTR_LABEL);

	/*check if gs->num_elems > 0*/
	gcIR.addInstrToTail("mov", "[r8+16]", "rax");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "collectAndResize_WHILE_END");

	/*store gs*/
	gcIR.addInstrToTail("mov", "r8", "[rbp-8]");

	/*call ref = pop(gs)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "r8");
	gcIR.addInstrToTail("call", "pop");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*set ref->color = BLACK*/
	gcIR.addInstrToTail("mov", "QWORD PTR " BLACK, "[rax+24]");

	/*if ref->type == PRIMITIVE_TYPE*/
	gcIR.addInstrToTail("mov", "[rax+16]", "r8");
	gcIR.addInstrToTail("cmp", PRIMITIVE_TYPE, "r8");
	gcIR.addInstrToTail("je", "collectAndResize_NOT_PRIM");
	gcIR.addInstrToTail("mov", "[rbp-8]", "r8");
	gcIR.addInstrToTail("jmp", "collectAndResize_WHILE");

	gcIR.addInstrToTail("collectAndResize_NOT_PRIM:", "", "", InstructionList::INSTR_LABEL);
	/*interpret ref->data as an object, check the tag*/
	gcIR.addInstrToTail("mov", "[rax]", "rax");
	gcIR.addInstrToTail("mov", "[rax]", "r8");
	gcIR.addInstrToTail("cmp", to_string(globalSymTable->getClassTag("Int")), "r8");
	gcIR.addInstrToTail("jne", "collectAndResize_NOT_INT");
	gcIR.addInstrToTail("mov", "[rbp-8]", "r8");
	gcIR.addInstrToTail("jmp", "collectAndResize_WHILE");
	gcIR.addInstrToTail("collectAndResize_NOT_INT:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", to_string(globalSymTable->getClassTag("Bool")), "r8");
	gcIR.addInstrToTail("jne", "collectAndResize_NOT_BOOL");
	gcIR.addInstrToTail("mov", "[rbp-8]", "r8");
	gcIR.addInstrToTail("jmp", "collectAndResize_WHILE");
	gcIR.addInstrToTail("collectAndResize_NOT_BOOL:", "", "", InstructionList::INSTR_LABEL);

	/*for loop init*/
	gcIR.addInstrToTail("xor", "rcx", "rcx");
	gcIR.addInstrToTail("mov", "3", "rcx");
	gcIR.addInstrToTail("mov", "rax", "rdx");//need rax later
	gcIR.addInstrToTail("mov", "[rax+8]", "rax");

	/*for check condition rcx should be i and rax should be object.size*/
	gcIR.addInstrToTail("collectAndResize_FOR_START:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", "rax", "rcx");
	gcIR.addInstrToTail("jge", "collectAndResize_FOR_END");
	/*save rax and rcx */
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", "rcx");
	gcIR.addInstrToTail("push", "rdx");

	/*body of for loop*/
	/*call get(m, object[i]*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "[rdx+rcx*8]");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*if nested_ref == NULL continue*/
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "collectAndResize_FOR_UPDATE");

	/*if nested_ref->color != WHITE continue*/
	gcIR.addInstrToTail("mov", "[rax+24]", "r8");
	gcIR.addInstrToTail("cmp", WHITE, "r8");
	gcIR.addInstrToTail("jne", "collectAndResize_FOR_UPDATE");

	/*if nested_ref->type != OBJECT_TYPE jmp else*/
	gcIR.addInstrToTail("mov", "[rax+16]", "r8");
	gcIR.addInstrToTail("cmp", OBJECT_TYPE, "r8");
	gcIR.addInstrToTail("jne", "collectAndResize_ELSE");

	/*set color to grey*/
	gcIR.addInstrToTail("mov", GREY, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");

	/*call push(gs, nested_ref)*/
	gcIR.addInstrToTail("mov", "[rbp-8]", "r8");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", "r8");
	gcIR.addInstrToTail("call", "push");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rsp");

	gcIR.addInstrToTail("jmp", "collectAndResize_END_IF");
	/*else*/
	gcIR.addInstrToTail("collectAndResize_ELSE:", "", "", InstructionList::INSTR_LABEL);
	/*set color to black*/
	gcIR.addInstrToTail("mov", BLACK, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");

	/*end if-else statement*/
	gcIR.addInstrToTail("collectAndResize_END_IF:", "", "", InstructionList::INSTR_LABEL);

	/*update for loop*/
	gcIR.addInstrToTail("collectAndResize_FOR_UPDATE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("pop", "rdx");
	gcIR.addInstrToTail("pop", "rcx");
	gcIR.addInstrToTail("pop", "rax");
	gcIR.addInstrToTail("inc", "rcx");
	gcIR.addInstrToTail("jmp", "collectAndResize_FOR_START");
	gcIR.addInstrToTail("collectAndResize_FOR_END:", "", "", InstructionList::INSTR_LABEL);


	/*end of while loop*/
	gcIR.addInstrToTail("mov", "[rbp-8]", "r8");
	gcIR.addInstrToTail("jmp", "collectAndResize_WHILE");
	gcIR.addInstrToTail("collectAndResize_WHILE_END:", "", "", InstructionList::INSTR_LABEL);

	/*call free on gs*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", "r8", "rdi");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rbp");

	/*table_size *= 2*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax]", "rax");
	gcIR.addInstrToTail("sal", "1", "rax"); //rax *= 2
	
	/*call initGC(rax)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("call", "initGC");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*save rax for return value*/
	gcIR.addInstrToTail("push", "rax");

	/*call transfer(m, initGC())*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "transfer");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*call clear all on m*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "clearAll");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	gcIR.addInstrToTail("pop", "rax");

	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function simply frees all the references in the garbage collector along with all overhead before finally freeing the garbage collector itself.
*/
void clearAll(InstructionList & gcIR)
{
	gcIR.addInstrToTail("clearAll:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*for init*/
	gcIR.addInstrToTail("xor", "rcx", "rcx");

	/*for condition*/
	gcIR.addInstrToTail("clearAll_LOOP:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax]", "rax");
	gcIR.addInstrToTail("cmp", "rax", "rcx");
	gcIR.addInstrToTail("jge", "clearAll_LOOP_END");

	/*check if m->table[i] is NULL*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax+16]", "rax");
	gcIR.addInstrToTail("mov", "[rax+rcx*8]", "rax");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "clearAll_ENDIF");
	
	/*save rbp, rcx, call freeSent*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rcx");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("call", "freeSent");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rcx");
	gcIR.addInstrToTail("pop", "rbp");


	gcIR.addInstrToTail("clearAll_ENDIF:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("inc", "rcx");
	gcIR.addInstrToTail("jmp", "clearAll_LOOP");
	/*end of loop*/
	gcIR.addInstrToTail("clearAll_LOOP_END:", "", "", InstructionList::INSTR_LABEL);
	
	/*free table*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax+16]", "rdi");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rbp");

	/*free m*/
	gcIR.addInstrToTail("mov", PARAM_1, "rdi");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rbp");

	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  assembly: Robert
*  This function is used along side collectAndResize. This function allows us to move nodes from the "old" garbage collector to the "new" one. This is
*  how we would be safely freeing only the unused nodes when calling the garbage collector at run time.
*/
void transfer(InstructionList & gcIR)
{
	gcIR.addInstrToTail("transfer:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*rax = 0, r8 = PARAM_1->table_size*/
	gcIR.addInstrToTail("xor", "rax", "rax");
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8]", "r8");

	/*start of for loop */
	gcIR.addInstrToTail("transfer_CMP:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", "r8", "rax");
	gcIR.addInstrToTail("jge", "transfer_END_FUNCT");

	/*move src->table[rax] into rcx */
	gcIR.addInstrToTail("mov", PARAM_1, "rcx");
	gcIR.addInstrToTail("mov", "[rcx+16]", "rcx");
	gcIR.addInstrToTail("mov", "[rcx+rax*8]", "rcx");
	
	/*check if null*/
	gcIR.addInstrToTail("cmp", "0", "rcx");
	gcIR.addInstrToTail("jne", "transfer_NOTNULL");
	gcIR.addInstrToTail("inc", "rax");
	gcIR.addInstrToTail("jmp", "transfer_CMP");

	/*not null*/
	gcIR.addInstrToTail("transfer_NOTNULL:", "", "", InstructionList::INSTR_LABEL);

	/*save rbp, rax, r8*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", "r8");
	gcIR.addInstrToTail("push", "rcx");

	/*call removeNonWhite(rcx), with rcx = src->table[rax]*/
	gcIR.addInstrToTail("push", "rcx");
	gcIR.addInstrToTail("call", "removeNonWhite");
	gcIR.addInstrToTail("mov", "rax", "rdx"); //put return val in rdx
	gcIR.addInstrToTail("add", "8", "rsp");

	/*restore r8, rax, rbp */
	gcIR.addInstrToTail("pop", "rcx");
	gcIR.addInstrToTail("pop", "r8");
	gcIR.addInstrToTail("pop", "rax");
	gcIR.addInstrToTail("pop", "rbp");

	/*check if return val is 0 */
	gcIR.addInstrToTail("cmp", "0", "rdx");
	gcIR.addInstrToTail("je", "transfer_END_WHILE");

	/*call insert(PARAM_2, rdx)*/
	/*store rbp, rax, r8*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", "r8");
	gcIR.addInstrToTail("push", "rcx");

	gcIR.addInstrToTail("push", "rdx");
	gcIR.addInstrToTail("push", PARAM_2);
	gcIR.addInstrToTail("call", "insert");
	gcIR.addInstrToTail("add", "16", "rsp");
	
	/*restore rbp, rax, r8 */
	gcIR.addInstrToTail("pop", "rcx");
	gcIR.addInstrToTail("pop", "r8");
	gcIR.addInstrToTail("pop", "rax");
	gcIR.addInstrToTail("pop", "rbp");

	/*jump back to start of loop*/
	gcIR.addInstrToTail("jmp", "transfer_NOTNULL");

	/*end of while loop*/
	gcIR.addInstrToTail("transfer_END_WHILE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("inc", "rax");
	gcIR.addInstrToTail("jmp", "transfer_CMP");

	/*end of function*/
	gcIR.addInstrToTail("transfer_END_FUNCT:", "", "", InstructionList::INSTR_LABEL);
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  assembly: Robert
*  This function adds a new reference to the garbage collector. This is what calculates the hash for where to store the reference and handles chaining.
*/
void insert(InstructionList & gcIR)
{
	gcIR.addInstrToTail("insert:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*call hash(param_2->data, param_1->table_size*/
	gcIR.addInstrToTail("mov", PARAM_2, "rax");
	gcIR.addInstrToTail("mov", "[rax]", "rax");
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8]", "r8");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "r8");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("call", "hash");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*save return value for later*/
	gcIR.addInstrToTail("push", "rax");

	/*move m->table[hash] into r8*/
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8+16]", "r8");
	gcIR.addInstrToTail("mov", "[r8+rax*8]", "r8");

	/*check for null */
	gcIR.addInstrToTail("cmp", "0", "r8");
	gcIR.addInstrToTail("jne", "insert_NOTNULL");

	/*calloc, put the mem into m->table[i]*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", SENT_SIZE, "rdi");
	gcIR.addInstrToTail("mov", "1", "rsi");
	gcIR.addInstrToTail("call", "calloc");
	gcIR.addInstrToTail("pop", "rbp");

	/*move alloc'd memory into m->table[index]*/
	gcIR.addInstrToTail("mov", PARAM_1, "rcx");
	gcIR.addInstrToTail("mov", "[rcx+16]", "rcx");
	gcIR.addInstrToTail("pop", "r8");
	gcIR.addInstrToTail("mov", "rax", "[rcx+r8*8]");
	gcIR.addInstrToTail("mov", "rax", "r8");

	/*r8 holds m->table[i], move r8->head into param_2->next*/
	gcIR.addInstrToTail("insert_NOTNULL:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", PARAM_2, "rax");
	gcIR.addInstrToTail("mov", "[r8]", "rdx");
	gcIR.addInstrToTail("mov", "rdx", "[rax+8]");

	/*move param_2 into r8->head*/
	gcIR.addInstrToTail("mov", "rax", "[r8]");

	/*increment m->table[i]->num_elems*/
	gcIR.addInstrToTail("mov", "[r8+8]", "rcx");
	gcIR.addInstrToTail("inc", "rcx");
	gcIR.addInstrToTail("mov", "rcx",  "[r8+8]");

	/*increment m->num_elems*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax+8]", "r8");
	gcIR.addInstrToTail("inc", "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+8]");

	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function gets a given node from the map. If there is no node with that memory address it returns NULL.
*/
void get(InstructionList & gcIR)
{
	gcIR.addInstrToTail("get:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*Get call hash*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("push", "[rax]");
	gcIR.addInstrToTail("push", PARAM_2);
	gcIR.addInstrToTail("call", "hash");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*mov m->table[hash] rax*/
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8+16]", "r8");
	gcIR.addInstrToTail("mov", "[r8+rax*8]", "rax");

	/*if rax == 0 return*/
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("jne", "get_NOT_NULL");
	gcIR.addInstrToTail("jmp", "get_WHILE_END");

	gcIR.addInstrToTail("get_NOT_NULL:", "", "", InstructionList::INSTR_LABEL);

	gcIR.addInstrToTail("mov", "[rax]", "rax");

	/*while loop start*/
	gcIR.addInstrToTail("get_WHILE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "get_WHILE_END");

	/*if tmp->data = ref return tmp*/
	gcIR.addInstrToTail("mov", "[rax]", "r8");
	gcIR.addInstrToTail("mov", PARAM_2, "rcx");
	gcIR.addInstrToTail("cmp", "rcx", "r8");
	gcIR.addInstrToTail("je", "get_WHILE_END");

	/*rax = rax->next*/
	gcIR.addInstrToTail("mov", "[rax+8]", "rax");
	gcIR.addInstrToTail("jmp", "get_WHILE");

	gcIR.addInstrToTail("get_WHILE_END:", "", "", InstructionList::INSTR_LABEL);
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function finds all immediately reachable memory addresses and sets their color to gray.
*/
void setGreys(InstructionList & gcIR)
{
	gcIR.addInstrToTail("setGreys:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);
	/*go through registers rbx, r12, r13, r14, r15, check if they have valid references*/
	/*call get(PARAM_1, rbx)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rbx");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "setGreys_IF1");
	gcIR.addInstrToTail("mov", GREY, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");
	gcIR.addInstrToTail("setGreys_IF1:", "", "", InstructionList::INSTR_LABEL);

	/*call get(PARAM_1, r12)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "r12");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "setGreys_IF2");
	gcIR.addInstrToTail("mov", GREY, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");
	gcIR.addInstrToTail("setGreys_IF2:", "", "", InstructionList::INSTR_LABEL);

	/*call get(PARAM_1, r12)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "r13");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "setGreys_IF3");
	gcIR.addInstrToTail("mov", GREY, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");
	gcIR.addInstrToTail("setGreys_IF3:", "", "", InstructionList::INSTR_LABEL);

	/*call get(PARAM_1, r12)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "r14");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "setGreys_IF4");
	gcIR.addInstrToTail("mov", GREY, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");
	gcIR.addInstrToTail("setGreys_IF4:", "", "", InstructionList::INSTR_LABEL);

	/*call get(PARAM_1, r12)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "r15");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "setGreys_IF5");
	gcIR.addInstrToTail("mov", GREY, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");
	gcIR.addInstrToTail("setGreys_IF5:", "", "", InstructionList::INSTR_LABEL);


	/*go through the whole stack for valid references*/
	gcIR.addInstrToTail("mov", "tos[rip]", "rcx");
	gcIR.addInstrToTail("mov", "rsp", "r8");

	/*while loop start*/
	gcIR.addInstrToTail("setGreys_WHILE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", "rcx", "r8");
	gcIR.addInstrToTail("jge", "setGreys_WHILE_END");

	/*save rcx, r8*/
	gcIR.addInstrToTail("push", "rcx");
	gcIR.addInstrToTail("push", "r8");

	/*call get(param_1, *r8) */
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "[r8]");
	gcIR.addInstrToTail("push", PARAM_1);
	gcIR.addInstrToTail("call", "get");
	gcIR.addInstrToTail("add", "16", "rsp");
	gcIR.addInstrToTail("pop", "rbp");

	/*compare rax, 0 assign color to grey */
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "setGreys_WHILE_UPDATE");
	gcIR.addInstrToTail("mov", "QWORD PTR " GREY, "[rax+24]");

	/*add 8 to r8 to continue while loop*/
	gcIR.addInstrToTail("setGreys_WHILE_UPDATE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("pop", "r8");
	gcIR.addInstrToTail("add", "8", "r8");
	gcIR.addInstrToTail("pop", "rcx");
	gcIR.addInstrToTail("jmp", "setGreys_WHILE");

	gcIR.addInstrToTail("setGreys_WHILE_END:", "", "", InstructionList::INSTR_LABEL);
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function removes a single non white node from a given sentinel node.
*/
void removeNonWhite(InstructionList & gcIR)
{
	gcIR.addInstrToTail("removeNonWhite:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*check if param_1->num_elems is 0 if so exit */
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8+8]", "rax");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "removeNonWhite_EXIT");

	/*check if s->head->color is not white*/
	gcIR.addInstrToTail("mov", "[r8]", "rax");
	gcIR.addInstrToTail("mov", "[rax+24]", "rax");
	gcIR.addInstrToTail("cmp", WHITE, "rax");
	gcIR.addInstrToTail("je", "removeNonWhite_ELSE");

	/*remove and return head r8 = s [r8] = s->head*/
	gcIR.addInstrToTail("mov", "[r8]", "rax");
	gcIR.addInstrToTail("mov", "[rax+8]", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8]");

	/*deccrement num_elems */
	gcIR.addInstrToTail("mov", "[r8+8]", "rcx");
	gcIR.addInstrToTail("dec", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+8]");

	/*set rv->next = 0 */
	gcIR.addInstrToTail("xor", "rcx", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+8]");
	
	/*return*/
	gcIR.addInstrToTail("jmp", "removeNonWhite_EXIT");

	/*else*/
	gcIR.addInstrToTail("removeNonWhite_ELSE:", "", "", InstructionList::INSTR_LABEL);
	
	/*store s->head in rax*/
	gcIR.addInstrToTail("mov", "[r8]", "rax");

	/*while loop start*/
	gcIR.addInstrToTail("removeNonWhite_WHILE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "[rax+8]", "r8");
	gcIR.addInstrToTail("cmp", "0", "r8");
	gcIR.addInstrToTail("je", "removeNonWhite_WHILE_END");

	/*while body*/
	gcIR.addInstrToTail("mov", "[r8+24]", "r8");
	gcIR.addInstrToTail("cmp", WHITE, "r8");
	gcIR.addInstrToTail("je", "removeNonWhite_ENDIF");

	/*store tmp->next*/
	gcIR.addInstrToTail("mov", "rax", "r8");
	gcIR.addInstrToTail("mov", "[rax+8]", "rax");
	
	/*remove tmp->next*/
	gcIR.addInstrToTail("mov", "[rax+8]", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+8]");

	/*decrement num_elems*/
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8+8]", "rcx");
	gcIR.addInstrToTail("dec", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+8]");

	/*set rv->next = 0 */
	gcIR.addInstrToTail("xor", "rcx", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+8]");
	gcIR.addInstrToTail("jmp", "removeNonWhite_EXIT");

	gcIR.addInstrToTail("removeNonWhite_ENDIF:", "", "", InstructionList::INSTR_LABEL);
	/*update and continue*/
	gcIR.addInstrToTail("mov", "[rax+8]", "rax");
	gcIR.addInstrToTail("jmp", "removeNonWhite_WHILE");


	gcIR.addInstrToTail("removeNonWhite_WHILE_END:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "0", "rax");
	gcIR.addInstrToTail("removeNonWhite_EXIT:", "", "", InstructionList::INSTR_LABEL);
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function frees the sentinel node and the memory it is keeping track of.
*/
void freeSent(InstructionList & gcIR)
{
	gcIR.addInstrToTail("freeSent:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);
	/*make space for saving n's location*/
	gcIR.addInstrToTail("sub", "8", "rsp");

	/*move s->head into rax*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax]", "rax");

	/*start while*/
	gcIR.addInstrToTail("freeSent_WHILE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "freeSent_WHILE_END");

	/*save tmp*/
	gcIR.addInstrToTail("mov", "rax", "r8");
	gcIR.addInstrToTail("push", "r8");

	/*store n = n->next*/
	gcIR.addInstrToTail("mov", "[rax+8]", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[rbp-8]");

	/*call free r8->data*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", "[r8]", "rdi");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rbp");

	/*restore tmp and free it*/
	gcIR.addInstrToTail("pop", "rdi");
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rbp");

	/*move tmp into rax and jump back to the top of loop*/
	gcIR.addInstrToTail("mov", "[rbp-8]", "rax");
	gcIR.addInstrToTail("jmp", "freeSent_WHILE");

	/*free param_1*/
	gcIR.addInstrToTail("freeSent_WHILE_END:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", PARAM_1, "rdi");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rbp");
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function makes a new node setting tis default color to white.
*/
void makeNode(InstructionList & gcIR)
{
	gcIR.addInstrToTail("makeNode:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*call malloc(32)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", NODE_SIZE, "rdi");
	gcIR.addInstrToTail("call", "malloc");
	gcIR.addInstrToTail("pop", "rbp");

	/*n->data = ref*/
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax]");

	/*n->type = type*/
	gcIR.addInstrToTail("mov", PARAM_2, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+16]");

	/*n->color = white*/
	gcIR.addInstrToTail("mov", WHITE, "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+24]");
	
	/*n->next = NULL*/
	gcIR.addInstrToTail("mov", "0", "r8");
	gcIR.addInstrToTail("mov", "r8", "[rax+8]");

	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function pops a node from the queue.
*/
void pop(InstructionList & gcIR)
{
	gcIR.addInstrToTail("pop:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*save q->head */
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax]", "rax");

	/*compare head and tail*/
	gcIR.addInstrToTail("mov", PARAM_1, "rcx");
	gcIR.addInstrToTail("mov", "[rcx+8]", "rcx");
	gcIR.addInstrToTail("cmp", "rax", "rcx");
	gcIR.addInstrToTail("jne", "pop_ELSE");

	/*save head, then set head and tail to 0*/
	gcIR.addInstrToTail("xor", "r8", "r8");
	gcIR.addInstrToTail("mov", PARAM_1, "rcx");
	gcIR.addInstrToTail("mov", "r8", "[rcx]");
	gcIR.addInstrToTail("mov", "r8", "[rcx+8]");
	gcIR.addInstrToTail("jmp", "pop_ENDIF");

	/*else*/
	gcIR.addInstrToTail("pop_ELSE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "[rax+8]", "r8");
	gcIR.addInstrToTail("mov", PARAM_1, "rcx");
	gcIR.addInstrToTail("mov", "r8", "[rcx]");

	gcIR.addInstrToTail("pop_ENDIF:", "", "", InstructionList::INSTR_LABEL);
	/*subtract 1 from num_elems*/
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8+16]", "rcx");
	gcIR.addInstrToTail("dec", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+16]");

	/*save [rax], call free(rax) */
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "[rax]");
	gcIR.addInstrToTail("mov", "rax", "rdi");
	gcIR.addInstrToTail("call", "free");
	gcIR.addInstrToTail("pop", "rax");
	gcIR.addInstrToTail("pop", "rbp");

	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function pushes a node to the queue.
*/
void push(InstructionList & gcIR)
{
	gcIR.addInstrToTail("push:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*call makeQNode(PARAM_2) move result into rcx*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", PARAM_2);
	gcIR.addInstrToTail("call", "makeQNode");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("mov", "rax", "rcx");

	/*compare q->tail to NULL*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax+8]", "r8");
	gcIR.addInstrToTail("cmp", "0", "r8");
	gcIR.addInstrToTail("jne", "push_ELSE");

	/*move n to q->head */
	gcIR.addInstrToTail("mov", "rcx", "[rax]");
	gcIR.addInstrToTail("jmp", "push_ENDIF");

	gcIR.addInstrToTail("push_ELSE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "rcx", "[r8+8]");

	gcIR.addInstrToTail("push_ENDIF:", "", "", InstructionList::INSTR_LABEL);
	
	/*q->tail = n*/
	gcIR.addInstrToTail("mov", "rcx", "[rax+8]");


	/*add 1 from num_elems*/
	gcIR.addInstrToTail("mov", PARAM_1, "r8");
	gcIR.addInstrToTail("mov", "[r8+16]", "rcx");
	gcIR.addInstrToTail("inc", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[r8+16]");
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  This function creates a queue of every gray set in the garbage collector.
*/
void getGreySet(InstructionList & gcIR)
{
	gcIR.addInstrToTail("getGreySet:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);
	/*make space for locals, -8 = gs, -16 = i, -24 = table_size*/
	gcIR.addInstrToTail("sub", "24", "rsp");
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax]", "rax");
	gcIR.addInstrToTail("mov", "rax", "[rbp-24]");

	/*call calloc(24)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", Q_SIZE, "rdi");
	gcIR.addInstrToTail("mov", "1", "rsi");
	gcIR.addInstrToTail("call", "calloc");
	gcIR.addInstrToTail("pop", "rbp");
	gcIR.addInstrToTail("mov", "rax", "[rbp-8]");

	gcIR.addInstrToTail("xor", "rcx", "rcx");

	/*for loop start*/
	gcIR.addInstrToTail("getGreySet_FOR:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "rcx", "[rbp-16]");
	gcIR.addInstrToTail("mov", "[rbp-24]", "rax");
	gcIR.addInstrToTail("cmp", "rax", "rcx");
	gcIR.addInstrToTail("jge", "getGreySet_FOR_END");

	/*for loop body*/
	/*get m->table[i]*/
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("mov", "[rax+16]", "rax");
	gcIR.addInstrToTail("mov", "[rax+rcx*8]", "rax");
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "getGreySet_FOR_UPDATE");

	/*move head into rax*/
	gcIR.addInstrToTail("mov", "[rax]", "rax");

	/*while loop start*/
	gcIR.addInstrToTail("getGreySet_WHILE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("cmp", "0", "rax");
	gcIR.addInstrToTail("je", "getGreySet_WHILE_END");

	/*while body*/
	/*move tmp->color to r8*/
	gcIR.addInstrToTail("mov", "[rax+24]", "r8");
	gcIR.addInstrToTail("cmp", GREY, "r8");
	gcIR.addInstrToTail("jne", "getGreySet_ENDIF");

	/*call push(gs, tmp)*/
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("push", "rax");
	gcIR.addInstrToTail("push", "[rbp-8]");
	gcIR.addInstrToTail("call", "push");
	gcIR.addInstrToTail("add", "8", "rsp");
	gcIR.addInstrToTail("pop", "rax");
	gcIR.addInstrToTail("pop", "rbp");

	gcIR.addInstrToTail("getGreySet_ENDIF:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "[rax+8]", "rax");
	gcIR.addInstrToTail("jmp", "getGreySet_WHILE");

	/*end while loop*/
	gcIR.addInstrToTail("getGreySet_WHILE_END:", "", "", InstructionList::INSTR_LABEL);

	/*for loop update*/
	gcIR.addInstrToTail("getGreySet_FOR_UPDATE:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "[rbp-16]", "rcx");
	gcIR.addInstrToTail("inc", "rcx");
	gcIR.addInstrToTail("jmp", "getGreySet_FOR");

	/*return gs*/
	gcIR.addInstrToTail("getGreySet_FOR_END:", "", "", InstructionList::INSTR_LABEL);
	gcIR.addInstrToTail("mov", "[rbp-8]", "rax");
	atCalleeExit(gcIR);
}

/**
*  author: Forest
*  assembly: Robert
*  This is the function that computes the simple hash used for our garbage collector map.
*/
void hashFun(InstructionList & gcIR)
{
	gcIR.addInstrToTail("hash:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/* (param 1 >> 5) % param 2*/
	gcIR.addInstrToTail("xor", "rax", "rax");
	gcIR.addInstrToTail("mov", PARAM_1, "rax");
	gcIR.addInstrToTail("shr", "5", "rax");
	gcIR.addInstrToTail("xor", "rdx", "rdx");
	gcIR.addInstrToTail("mov", PARAM_2, "rcx");
	gcIR.addInstrToTail("div", "rcx");
	gcIR.addInstrToTail("mov", "rdx", "rax"); //div stores remainder in rdx (changed from Robert's code)

	atCalleeExit(gcIR);
}

/**
 * author: Forest
 * This function makes a new node that is used with the queue.
 */
void makeQNode(InstructionList &gcIR)
{
	gcIR.addInstrToTail("makeQNode:", "", "", InstructionList::INSTR_LABEL);
	atCalleeEntry(gcIR);

	/*malloc 16 bytes for node */
	gcIR.addInstrToTail("push", "rbp");
	gcIR.addInstrToTail("mov", "16", "rdi");
	gcIR.addInstrToTail("pop", "rbp");

	/*set data to param 1, next to 0 */
	gcIR.addInstrToTail("mov", "[rbp+8]", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[rax]");
	gcIR.addInstrToTail("xor", "rcx", "rcx");
	gcIR.addInstrToTail("mov", "rcx", "[rax+8]");

	atCalleeExit(gcIR);
}
