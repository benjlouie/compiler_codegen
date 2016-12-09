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
	if (strengthreduce) {
		int src_int = 0;
		float src_flt = 0;
		int twopow = 0;
		src_int = atoi(src.c_str());
		src_flt = (float)src_int;

		//if add
		if (instr == "add" && src == "1") {
			inst.set("inc", "", dest, extraData);
		}

		//if subtract
		else if (instr == "sub" && src == "1") {
			inst.set("dec", "", dest, extraData);
		}

		//if mult by 0
		else if (instr == "imul" && src == "0") {
			inst.set("xor", dest, dest, extraData);
		}

		//if mult
		else if (instr == "imul" && !(src_int & (src_int - 1))) {
			twopow = (int)log2(src_flt);
			inst.set("shl", std::to_string(twopow), dest, extraData);
		}

		//if divide
		else if (instr == "div" && src != "0" & !(src_int & (src_int - 1))) {
			twopow = (int)log2(src_flt);
			inst.set("shr", std::to_string(twopow), dest, extraData);
		}

		//if no reduction possible
		else {
			inst.set(instr, src, dest, extraData);
		}
	}
	else {
		inst.set(instr, src, dest, extraData);
	}
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

