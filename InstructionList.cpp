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
void InstructionList::printIR(fstream &outfile) {
	InstructionNode * cur = head;

	while (cur) {
		outfile << cur->comment;
		if (cur->prelabel != "") {
			outfile << cur->prelabel << '\n';
		}

		for (auto instr : cur->instructions) {
			if (instr.extraData == INSTR_OK)
				outfile << "\t";
			outfile << instr.instruction;
			if (instr.dest != "") {
				outfile << "\t" << instr.dest;
				if (instr.src != "") {
					outfile << ", " << instr.src;
				}
			}
			else if (instr.src != "") {
				outfile << "\t" << instr.src;
			}
			outfile << '\n';
		}
		if (cur->postlabel != "") {
			outfile << cur->postlabel << '\n';
		}
		outfile << "#============================================" << '\n';

		cur = cur->next;
	}


}

