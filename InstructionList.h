#pragma once
#include <iostream>
#include<vector>
#include<string>

class InstructionList
{
public:
	InstructionList();
	~InstructionList();

	enum ExtraInstrData
	{
		INSTR_OK,
		INSTR_LABEL
	};

	struct Instruction
	{
		std::string instruction;
		std::string src;
		std::string dest;
		ExtraInstrData extraData;

		/*
		 * Author: Matt, Robert, Ben
		 */
		void set(std::string instr, std::string src = "", std::string dest = "", ExtraInstrData extraData = INSTR_OK) {
			this->instruction = instr;
			this->src = src;
			this->dest = dest;
			this->extraData = extraData;
		}
	};

	void addInstrToTail(Instruction instr);

	void addInstrToTail(std::string instr, std::string src = "", std::string dest = "", ExtraInstrData extraData = INSTR_OK);
	
	void addNewNode();

	void addComment(std::string comment);

	void addPreLabel(std::string preLabel);

	void addPostLabel(std::string postLabel);

	void printIR();



private:
	//Add a thing for a label for the assembly.
	struct InstructionNode
	{
		std::vector<Instruction> instructions;
		std::string prelabel;
		std::string postlabel;
		InstructionNode *next;
		InstructionNode *prev;
		std::string comment;

		InstructionNode() {
			next = nullptr;
			prev = nullptr;
		}
	} *head, *tail;

	size_t size;

};

