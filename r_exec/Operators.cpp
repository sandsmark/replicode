#include "ExecutionContext.h"
#include "match.h"
#include "hash_containers"
#include <sys/time.h>

namespace r_exec {

using r_code::Atom;

void operator_add(ExecutionContext& context)
{
	Expression lhs(context.evaluateOperand(1));
	Expression rhs(context.evaluateOperand(2));
	if (lhs.head().isFloat()) {
		if (rhs.head().isFloat()) {
			context.setResult(Atom::Float( rhs.head().asFloat() + lhs.head().asFloat()));
		} else if (rhs.head().getDescriptor() == Atom::TIMESTAMP) {
			context.setResultTimestamp( int(lhs.head().asFloat() * 1e6) + rhs.decodeTimestamp() );
		}
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if (rhs.head().isFloat()) {
			context.setResultTimestamp( lhs.decodeTimestamp() + int(rhs.head().asFloat() * 1e6));
		}
	}
}

bool operator == (const Expression& lhs, const Expression& rhs)
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
		Atom::Boolean(
			context.evaluateOperand(1) == context.evaluateOperand(2)
		)
	);
}

void operator_neq(ExecutionContext& context)
{
	context.setResult(
		Atom::Boolean(! (
			context.evaluateOperand(1) == context.evaluateOperand(2)
		))
	);
}

void operator_gte(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() >= rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		context.setResult(Atom::Boolean(
			lhs.decodeTimestamp() >= rhs.decodeTimestamp()));
	}
}

void operator_gtr(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() > rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		context.setResult(Atom::Boolean(
			lhs.decodeTimestamp() > rhs.decodeTimestamp()));
	}
}

void operator_lse(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() <= rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		context.setResult(Atom::Boolean(
			lhs.decodeTimestamp() <= rhs.decodeTimestamp()));
	}
}

void operator_lsr(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() < rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		context.setResult(Atom::Boolean(
			lhs.decodeTimestamp() < rhs.decodeTimestamp()));
	}
}

void operator_now(ExecutionContext& context)
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	int64 now = static_cast<int64>(tv.tv_sec) * 1000000 + tv.tv_usec;
	context.setResultTimestamp( now );
}

struct Ehash {
	size_t operator() (const Expression&) const;
};

size_t Ehash::operator() ( const Expression& x) const
{
	if (x.head().isPointer())
		return (*this)(x.dereference());
	unsigned long __h = x.head().atom;
	for (int i = 0; i <= x.head().getAtomCount(); ++i) {
		Expression c(x.child(i));
		uint32 d;
		if (c.head().isPointer()) {
			d = (*this)(c.dereference());
		} else {
			d = c.head().atom;
		}
		__h = 5 * __h + d;
	}
	return __h;
}

void _operator_red(ExecutionContext& context, bool matchOrNotMatch)
{
	// TODO: vws, mks
	context.beginResultSet();

	UNORDERED_SET<Expression, Ehash> produced;
	Expression inputs(context.evaluateOperand(1));
	ExecutionContext productions(context.xchild(2));
	for (int i = 1; i <= inputs.head().getAtomCount(); ++i) {
		Expression input(inputs.child(i));
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
						prod.evaluate();

						Expression prodv = prod;
						prodv.setValueAddressing(true);

						// see if we've produced this result before
						if (produced.find(prodv) != produced.end())
							continue;

						// we will be be over-writing prodv if we re-evaluate this production later, so we must make a copy
						prodv = prodv.copy(prodv.getInstance());

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
void operator_sub(ExecutionContext& context)
{
	Expression lhs(context.evaluateOperand(1));
	Expression rhs(context.evaluateOperand(2));

	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult( Atom::Float( lhs.head().asFloat() - rhs.head().asFloat() ));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		double diff = static_cast<float>( lhs.decodeTimestamp() ) - static_cast<float>( rhs.decodeTimestamp() );
		context.setResult( Atom::Float(diff * 1e-6) );
	}
}

}
