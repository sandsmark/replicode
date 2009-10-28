#ifndef __OPERATOR_REGISTER_H
#define __OPERATOR_REGISTER_H
#include "r_code.h"
#include "ExecutionContext.h"
typedef void (*Operator)(ExecutionContext&);
class OperatorRegister {
public:
	OperatorRegister();
	Operator& operator [](uint16 r_opcode) { return operators[r_opcode]; }
private:
	Operator operators[256];
};

OperatorRegister& theOperatorRegister();
	
#endif
