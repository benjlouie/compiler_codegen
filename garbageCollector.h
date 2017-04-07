#pragma once
#include "InstructionList.h"
#include "linearIR.h"

#define NODE_SIZE "32"
#define MAP_SIZE "24"
#define SENT_SIZE "16"
#define Q_SIZE "24"
#define PARAM_1 "[rbp+8]"
#define PARAM_2 "[rbp+16]"
#define PARAM_3 "[rbp+24]"
#define WHITE "0"
#define GREY "1"
#define BLACK "2"

InstructionList &makeGarbageCollectorIR();
