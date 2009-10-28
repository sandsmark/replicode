#include "match.h"
#include "r_code.h"

using namespace r_code;

const r_atom RTrue = r_atom::Boolean(true);
const r_atom RWild = r_atom::Wildcard();
const r_atom RTailWild = r_atom::TailWildcard();


void nilSkel(ExecutionContext skel)
{
	skel.setResult(r_atom::Nil());
	for (int i = 1; i <= skel.head().getAtomCount(); ++i) {
		nilSkel(skel.xchild(i));
	}
}

bool matchSkel(RExpression input, ExecutionContext skel)
{
	r_atom s = skel.head();
	bool result = true;
	if (s != input.head() && s != RWild && s != RTailWild) {
		nilSkel(skel);
		return false;
	}
	skel.setResult(input.head());
	for (int i = 1; i <= skel.head().getAtomCount(); ++i) {
		if (!matchSkel(input.child(i), skel.xchild(i)))
			result = false;
	}
	return result;
}

bool match(RExpression input, ExecutionContext pattern)
{
	ExecutionContext skel(pattern.xchild(1));
	ExecutionContext guards(pattern.xchild(2));

	r_atom RTrue = r_atom::Boolean(true);
	bool matches = false;
	if (matchSkel(input, skel)) {
		matches = true;
		for (int i = 1; matches && i <= guards.head().getAtomCount(); ++i) {
			ExecutionContext guard(guards.xchild(i));
			RExpression result = guard.evaluate();
			if (result.dereference().head().atom != RTrue.atom)
				matches = false;
		}
	}
	return matches;
}
