#include "InstructionList.h"

/* Simply adds a new instruction onto the end of the vector
 * in tail.
 *
 * Authors:Ben, Matt, Robert
 */
void InstructionList::addInstrToTail(Instruction instr) {
	tail->instructions.push_back(instr);
}

void InstructionList::addInstrToTail(std::string instr, std::string src, std::string dest, ExtraInstrData extraData) {
	Instruction inst;
	inst.set(instr, src, dest, extraData);
	this->addInstrToTail(inst);
}

/* Simply adds a new node onto the tail of the list.
 * Makes a new linked list if one doesn't exist.
 *
 * Authors:Ben, Matt, Robert
 */
void InstructionList::addNewNode() {
	InstructionNode *newNode = new InstructionNode();
	size++; //adding a node either way
	if (head == nullptr) {
		head = newNode;
		tail = newNode;
		return;
	}

	newNode->prev = tail;
	tail->next = newNode;
	tail = newNode;	
}

/*
* Author: Matt, Robert, Ben
*/
void InstructionList::addComment(std::string comment) {
	tail->comment += "#" + comment + "\n";
}

void InstructionList::addPreLabel(std::string preLabel) {
	tail->prelabel = preLabel;
}

void InstructionList::addPostLabel(std::string postLabel) {
	tail->postlabel = postLabel;
}

InstructionList::InstructionList() {
	head = nullptr;
	tail = nullptr;
	size = 0;
}

InstructionList::~InstructionList() {

}

/* 
 * Author: Ben Louie
 */
void InstructionList::printIR() {
	InstructionNode * cur = head;

	while (cur) {
		std::cout << cur->comment;
		if (cur->prelabel != "") {
			std::cout << cur->prelabel << '\n';
		}

		for (auto instr : cur->instructions) {
			if (instr.extraData == INSTR_OK)
				std::cout << "\t";
			std::cout << instr.instruction;
			if (instr.dest != "") {
				std::cout << "\t" << instr.dest;
				if (instr.src != "") {
					std::cout << ", " << instr.src;
				}
			}
			else if (instr.src != "") {
				std::cout << "\t" << instr.src;
			}
			std::cout << '\n';
		}
		if (cur->postlabel != "") {
			std::cout << cur->postlabel << '\n';
		}
		std::cout << "#============================================" << '\n';

		cur = cur->next;
	}


}

