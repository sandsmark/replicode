#ifndef __OPERATOR_REGISTER_H
#define __OPERATOR_REGISTER_H
#include "ExecutionContext.h"

namespace r_exec {

typedef void (*Operator)(ExecutionContext&);
class OperatorRegister {
public:
	OperatorRegister();
	Operator& operator [](uint16 r_opcode) { return operators[r_opcode]; }
private:
	Operator operators[256];
};

OperatorRegister& theOperatorRegister();
	
}

#endif
