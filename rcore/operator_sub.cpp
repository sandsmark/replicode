#include "ExecutionContext.h"

using namespace r_code;

void operator_sub(ExecutionContext& context)
{
	RExpression lhs(context.evaluateOperand(1));
	RExpression rhs(context.evaluateOperand(2));

	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult( r_atom::Float( lhs.head().asFloat() - rhs.head().asFloat() ));
	} else if (lhs.head().getDescriptor() == r_atom::TIMESTAMP && rhs.head().getDescriptor() == r_atom::TIMESTAMP) {
		double diff = static_cast<float>( lhs.decodeTimestamp() ) - static_cast<float>( rhs.decodeTimestamp() );
		context.setResult( r_atom::Float(diff * 1e-6) );
	}
}
