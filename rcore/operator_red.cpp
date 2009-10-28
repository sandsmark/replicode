#include "ExecutionContext.h"

#include "match.h"
#include <tr1/unordered_set>

using namespace r_code;
using namespace std;
using namespace tr1;

void _operator_red(ExecutionContext& context, bool matchOrNotMatch)
{
	context.beginResultSet();

	unordered_set<RExpression, REhash> produced;
	RExpression inputs(context.evaluateOperand(1));
	ExecutionContext productions(context.xchild(2));
	for (int i = 1; i <= inputs.head().getAtomCount(); ++i) {
		RExpression input(inputs.child(i));
		for (int j = 1; j <= productions.head().getAtomCount(); ++j) {
			ExecutionContext pss(productions.xchild(j));
			if (pss.head().getAtomCount() == 2) {
				ExecutionContext guards(pss.xchild(1));
				ExecutionContext prods(pss.xchild(2));
				bool guardsMatch = true;
				for (int k = 1; guardsMatch && k <= guards.head().getAtomCount(); ++k) {
					ExecutionContext guard(guards.xchild(k));
					if (!match(input, guard))
						guardsMatch = false;
				}

				if (guardsMatch == matchOrNotMatch) {
					for (int k = 1; k <= prods.head().getAtomCount(); ++k) {
						ExecutionContext prod(prods.xchild(k));
						RExpression prodv = prod.evaluate();

						// see if we've produced this result before
						if (produced.find(prodv) != produced.end())
							continue;

						// we will be be over-writing prodv if we re-evaluate this production later, so we must make a copy
						prodv = prodv.copy();

						// put this produced element into the produced set, so we won't produce it again
						produced.insert(prodv);
						context.appendResultSetElement(prodv.iptr());
					}
				}
			}
		}
	}

	context.endResultSet();
}

void operator_red(ExecutionContext& context)
{
	return _operator_red(context, true);
}

void operator_notred(ExecutionContext& context)
{
	return _operator_red(context, false);
}
