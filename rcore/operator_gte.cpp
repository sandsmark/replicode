#include "ExecutionContext.h"

using namespace r_code;

void operator_gte(ExecutionContext& context)
{
	RExpression lhs = context.evaluateOperand(1);
	RExpression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(r_atom::Boolean(lhs.head().asFloat() >= rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == r_atom::TIMESTAMP && rhs.head().getDescriptor() == r_atom::TIMESTAMP) {
		context.setResult(r_atom::Boolean(
			lhs.decodeTimestamp() >= rhs.decodeTimestamp()));
	}
}

