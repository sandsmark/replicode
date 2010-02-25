#ifndef __OPERATORS_H
#define __OPERATORS_H

namespace r_exec {

bool operator == (const Expression& lhs, const Expression& rhs);
void operator_add(ExecutionContext& context);
void operator_equ(ExecutionContext& context);
void operator_neq(ExecutionContext& context);
void operator_gte(ExecutionContext& context);
void operator_gtr(ExecutionContext& context);
void operator_lse(ExecutionContext& context);
void operator_lsr(ExecutionContext& context);
void operator_now(ExecutionContext& context);
void operator_sub(ExecutionContext& context);
void operator_red(ExecutionContext& context);
void operator_notred(ExecutionContext& context);
void operator_ins(ExecutionContext& context);

}

#endif
