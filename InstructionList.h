#pragma once
#include<vector>
#include<string>

class InstructionList
{
public:
	InstructionList();
	~InstructionList();

	enum ExtraInstrData
	{
		INSTR_OK
	};

	struct Instruction
	{
		std::string instruction;
		std::string src;
		std::string dest;
		ExtraInstrData extraData;
	};

	void addInstrToTail(Instruction instr);
	
	void addNewNode();




private:
	//Add a thing for a label for the assembly.
	struct InstructionNode
	{
		std::vector<Instruction> instructions;
		std::string prelabel;
		std::string postlabel;
		InstructionNode *next;
		InstructionNode *prev;

		InstructionNode() {
			next = nullptr;
			prev = nullptr;
		}

	} *head, *tail;

	size_t size;

};

InstructionList::InstructionList()
{
}

InstructionList::~InstructionList()
{
}
