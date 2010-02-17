#include "match.h"

namespace r_exec {

using r_code::Atom;

const Atom True = Atom::Boolean(true);
const Atom Wild = Atom::Wildcard();
const Atom TailWild = Atom::TailWildcard();


void nilSkel(ExecutionContext skel)
{
	skel.setResult(Atom::Nil());
	for (int i = 1; i <= skel.head().getAtomCount(); ++i) {
		nilSkel(skel.xchild(i));
	}
}

bool matchSkel(Expression input, ExecutionContext skel)
{
	Atom s = skel.head();
	bool result = true;
	if (s != input.head() && s != Wild && s != TailWild) {
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

bool match(Expression input, ExecutionContext pattern)
{
	ExecutionContext skel(pattern.xchild(1));
	ExecutionContext guards(pattern.xchild(2));

	Atom True = Atom::Boolean(true);
	bool matches = false;
	if (matchSkel(input, skel)) {
		matches = true;
		for (int i = 1; matches && i <= guards.head().getAtomCount(); ++i) {
			guards.xchild(i).evaluate();
			Expression result(guards.child(i));
			result.setValueAddressing(true);
			if (result.dereference().head().atom != True.atom)
				matches = false;
		}
	}
	return matches;
}

}
