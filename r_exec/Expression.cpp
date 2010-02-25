#include <stdio.h>
#include "Expression.h"
#include "Object.h"

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
		case Atom::C_PTR: {
			Expression p(r.child(1).dereference());
			for (int i = 2; i <= r.head().getAtomCount(); ++i) {
				p = p.child(r.child(i).head().asIndex()).dereference();
				if (p.head() == Atom::VIEW) {
					Object* o = instance->objectForExpression(*this);
					Group* g = instance->getGroup();
					p = o->copyVisibleView(*instance, g);
				}
			}
			return p;
		}
        case Atom::R_PTR: {
			uint16 idx = head().asIndex();
			Object* o = instance->references[idx];
			for (int i = instance->firstReusableCopiedObject; i < instance->copies.size(); ++i) {
				if (instance->copies[i].object == o)
					return Expression(instance, instance->copies[i].position);
			}
			return o->copy(*instance);
		}
        case Atom::THIS:
			// TODO
            fprintf(stderr, "pointer type %02x NYI\n", head().getDescriptor());
            return r;
        default:
            return r;
    }
}

Expression Expression::copy(ReductionInstance& dest) const
{
	dest.syncSizes();
	// create an empty reference that will become the result
	Expression result(&dest);
	result.setValueAddressing(true);
	result.index = dest.value.size();

	// copy the top-level expression.  At this point it will have back-links
	dest.value.push_back(head());
	for (int i = 1; i <= head().getAtomCount(); ++i) {
		dest.value.push_back(child(i, false).head());
	}

	// copy any children which reside in the value array
	for (int i = 1; i <= result.head().getAtomCount(); ++i) {
		Expression c(result.child(i, false));
		if (c.head().getDescriptor() == Atom::I_PTR)
			c.head() = c.copy(dest).iptr();
		else if (c.head().getDescriptor() == Atom::VL_PTR)
			c.head() = c.copy(dest).vptr();
	}
	dest.syncSizes();
	return result;
}

}
