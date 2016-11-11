#include "linearIR.h"

InstructionList &makeMethodIR(Node *feat);

/*
 * Authors: Matt, Robert, and Ben
 */
unordered_map<string,InstructionList &> *makeLinear() {

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

		for (Tree *tFeature : features->getChildren()) {
			Node *feature = (Node *)tFeature;
			
			//Only care about methods
			if (feature->type == AST_FEATURE_METHOD) {
				std::string methName = ((Node *)feature->getChildren()[0])->value;
				std::string tmp = className + "." + methName;
				(*retMap)[tmp] = makeMethodIR(feature);
			}
		}
		//Go through each method

	}

	return retMap;
}

InstructionList &makeMethodIR(Node *feat) {

}
