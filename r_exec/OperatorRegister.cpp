#include "OperatorRegister.h"
#include "opcodes.h"
#include "Operators.h"

namespace r_exec {

OperatorRegister::OperatorRegister()
{
	for (int i = 0; i < 256; ++i)
		operators[i] = 0;
	operators[OPCODE_ADD] = operator_add;
	operators[OPCODE_EQU] = operator_equ;
	operators[OPCODE_NEQ] = operator_neq;
	operators[OPCODE_GTE] = operator_gte;
	operators[OPCODE_GTR] = operator_gtr;
	operators[OPCODE_LSE] = operator_lse;
	operators[OPCODE_LSR] = operator_lsr;
	operators[OPCODE_NOW] = operator_now;
	operators[OPCODE_SUB] = operator_sub;
	operators[OPCODE_RED] = operator_red;
	operators[OPCODE_NOTRED] = operator_notred;
}

OperatorRegister& theOperatorRegister()
{
	static OperatorRegister theRegister;
	return theRegister;
}

}
