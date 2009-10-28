#include "OperatorRegister.h"
#include "opcodes.h"

extern void operator_add(ExecutionContext& context);
extern void operator_equ(ExecutionContext& context);
extern void operator_neq(ExecutionContext& context);
extern void operator_gte(ExecutionContext& context);
extern void operator_gtr(ExecutionContext& context);
extern void operator_lse(ExecutionContext& context);
extern void operator_lsr(ExecutionContext& context);
extern void operator_now(ExecutionContext& context);
extern void operator_sub(ExecutionContext& context);
extern void operator_red(ExecutionContext& context);
extern void operator_notred(ExecutionContext& context);

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
