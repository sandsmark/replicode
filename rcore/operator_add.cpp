#include "ExecutionContext.h"

using namespace r_code;

void operator_add(ExecutionContext& context)
{
	RExpression lhs(context.evaluateOperand(1));
	RExpression rhs(context.evaluateOperand(2));
	if (lhs.head().isFloat()) {
		if (rhs.head().isFloat()) {
			context.setResult(r_atom::Float( rhs.head().asFloat() + lhs.head().asFloat()));
		} else if (rhs.head().getDescriptor() == r_atom::TIMESTAMP) {
			context.setResultTimestamp( int(lhs.head().asFloat() * 1e6) + rhs.decodeTimestamp() );
		}
	} else if (lhs.head().getDescriptor() == r_atom::TIMESTAMP) {
		if (rhs.head().isFloat()) {
			context.setResultTimestamp( lhs.decodeTimestamp() + int(rhs.head().asFloat() * 1e6));
		}
	}
}
