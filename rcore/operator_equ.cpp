#include "ExecutionContext.h"
#include "operator_equ.h"

using namespace r_code;

bool operator == (const RExpression& lhs, const RExpression& rhs)
{
	if (lhs.head().atom != rhs.head().atom)
		return false;
	if (lhs.head().isStructural()) {
		for (int i = 1; i <= lhs.head().getAtomCount(); ++i) {
			if (! (lhs.child(i) == rhs.child(i)))
				return false;
		}
	}
	return true;
}

void operator_equ(ExecutionContext& context)
{
	context.setResult(
		r_atom::Boolean(
			context.evaluateOperand(1) == context.evaluateOperand(2)
		)
	);
}

void operator_neq(ExecutionContext& context)
{
	context.setResult(
		r_atom::Boolean(! (
			context.evaluateOperand(1) == context.evaluateOperand(2)
		))
	);
}
