#include <stdio.h>
#include "Expression.h"

namespace r_exec {

using r_code::Atom;

Expression Expression::dereference() const
{
    Expression r(*this);
    switch(head().getDescriptor()) {
        case Atom::I_PTR:
            r.index = head().asIndex();
            return r.dereference();
        case Atom::VL_PTR:
            r.index = head().asIndex();
            r.setValueAddressing(true);
            return r.dereference();
        case Atom::C_PTR:
            {
                Expression p(r.child(1).dereference());
                for (int i = 2; i <= r.head().getAtomCount(); ++i) {
                    p = p.child(r.child(i).head().asIndex()).dereference();
                }
                return p;
            }
        case Atom::THIS:
        case Atom::R_PTR:
			// TODO
            fprintf(stderr, "pointer type %02x NYI\n", head().getDescriptor());
            return r;
        default:
            return r;
    }
}

Expression Expression::copy(ReductionInstance& dest) const
{
	// create an empty reference that will become the result
	Expression result(&dest);
	result.setValueAddressing(true);
	result.index = instance->value.size();

	// copy the top-level expression.  At this point it will have back-links
	instance->value.push_back(head());
	for (int i = 1; i <= head().getAtomCount(); ++i) {
		instance->value.push_back(child(i).head());
	}

	// copy any children which reside in the value array
	for (int i = 1; i <= result.head().getAtomCount(); ++i) {
		Expression c(result.child(i));
		if (c.head().getDescriptor() == Atom::I_PTR)
			result.child(i).head() = c.copy(dest).iptr();
		else if (c.head().getDescriptor() == Atom::VL_PTR)
			result.child(i).head() = c.copy(dest).vptr();
	}
	return result;
}

}
