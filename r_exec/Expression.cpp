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
			}
			return p;
		}
        case Atom::R_PTR: {
			uint16 idx = head().asIndex();
			Object* o = instance->references[idx];
			if (o) {
				for (int i = instance->firstReusableCopiedObject; i < instance->copies.size(); ++i) {
					if (instance->copies[i].object == o)
						return Expression(instance, instance->copies[i].position);
				}
				return o->copy(*instance);
			} // HACK: else, treat as a THIS pointer
		}
        case Atom::THIS: {
			int objectIndex = 0;
			for (int i = 0; i < instance->copies.size(); ++i) {
				if (instance->copies[i].position > index)
					break;
				objectIndex = instance->copies[i].position;
			}
			return Expression(instance, objectIndex);
		}
		case Atom::VIEW: {
			Object* o = instance->objectForExpression(*this);
			Group* g = instance->getGroup();
			return o->copyVisibleView(*instance, g);
		}
		case Atom::MKS: {
			Object* o = instance->objectForExpression(*this);
			return o->copyMarkerSet(*instance);
		}
		case Atom::VWS: {
			Object* o = instance->objectForExpression(*this);
			return o->copyViewSet(*instance);
		}
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
		Expression rc(result.child(i, false));
		Expression c(child(i, false));
		if (rc.head().getDescriptor() == Atom::I_PTR) {
			c = Expression(instance, c.head().asIndex(), false);
			rc.head() = c.copy(dest).iptr();
		} else if (c.head().getDescriptor() == Atom::VL_PTR) {
			c = Expression(instance, c.head().asIndex(), false);
			rc.head() = c.copy(dest).vptr();
		}
	}
	dest.syncSizes();
	return result;
}

}
