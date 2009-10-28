#include "ExecutionContext.h"
#include "OperatorRegister.h"
#include "r_code.h"
#include <sys/time.h>
#include <stdio.h>
#include "print_expression.h"

using namespace r_code;

ExecutionContext::ExecutionContext(AtomArray input_)
	:RExpression(new ExecutionEnvironment(input_)), ownsEnv(true)
{
}

void ExecutionContext::setResultTimestamp(int64 timestamp)
{
	if (head().getAtomCount() >= 2) {
		env->value[index] = r_atom::Timestamp();
		env->value[index+1] = r_atom(timestamp >> 32);
		env->value[index+2] = r_atom(timestamp);
	} else {
		env->value[index] = r_atom::IPointer(env->value.size());
		env->value.push_back(r_atom::Timestamp());
		env->value.push_back(r_atom(timestamp >> 32));
		env->value.push_back(r_atom(timestamp));
	}
}

int64 ExecutionContext::now()
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return static_cast<int64>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

RExpression ExecutionContext::evaluate()
{
	switch(head().getDescriptor()) {
		case r_atom::OPERATOR:
			{
			uint16 opcode = head().asOpcode();
			Operator o(theOperatorRegister()[opcode]);
			if (o != 0) {
				setResult(r_atom::Nil());
				o(*this);
			} else {
				fprintf(stderr, "operator(%d) NYI\n", opcode);
			}
			break;
			}
		case r_atom::SET:
			setResult(head());
			for (int i = 1; i <= head().getAtomCount(); ++i)
				evaluateOperand(i);
			break;
		case r_atom::C_PTR:
			{
			// first, copy the c_ptr structure to the value array
			for (int i = 0; i <= head().getAtomCount(); ++i)
				env->value[index + i] = env->input[index + i];

			// then, evaluate the pointer
			ExecutionContext p(*this);
			++p.index;
			p.evaluate();

			break;
			}
		case r_atom::I_PTR:
			{
			ExecutionContext p(*this);
			p.index = head().asIndex();
			p.evaluate();
			} // fall through to VL_PTR (the only difference is that I_PTR gets evaluated while VL_PTR doesn't)
		case r_atom::VL_PTR:
			if (env->value[head().asIndex()].isStructural())
				setResult(head());
			else
				setResult(env->value[head().asIndex()]);
			break;
		case r_atom::TIMESTAMP:
			setResultTimestamp(decodeTimestamp());
			break;
		default:
			if (head().isFloat()) {
				setResult(head());
			} else {
				fprintf(stderr, "descriptor 0x%02x NYI\n", head().getDescriptor());
			}
	}
	RExpression result(*this);
	result.setValueAddressing(true);
	return result;
}

RExpression ExecutionContext::evaluateOperand(int index_)
{
	ExecutionContext p(*this);
	p.index = index + index_;
	RExpression r = p.evaluate();
	return r.dereference();
}

RExpression RExpression::dereference() const
{
	RExpression r(*this);
	switch(head().getDescriptor()) {
		case r_atom::I_PTR:
			r.index = head().asIndex();
			return r.dereference();
		case r_atom::VL_PTR:
			r.index = head().asIndex();
			r.setValueAddressing(true);
			return r.dereference();
		case r_atom::C_PTR:
			{
				RExpression p(r.child(1).dereference());
				for (int i = 2; i <= r.head().getAtomCount(); ++i) {
					p = p.child(r.child(i).head().asIndex()).dereference();
				}
				return p;
			}
		case r_atom::THIS:
		case r_atom::V_PTR:
			fprintf(stderr, "pointer type %02x NYI\n", head().getDescriptor());
			return r;
		default:
			return r;
	}
}

RExpression RExpression::copy() const
{
	// create an empty reference that will become the result
	RExpression result(*this);
	result.setValueAddressing(true);
	result.index = env->value.size();

	// copy the top-level expression.  At this point it will have back-links
	env->value.push_back(head());
	for (int i = 1; i <= head().getAtomCount(); ++i) {
		env->value.push_back(child(i).head());
	}

	// copy any children which reside in the value array
	for (int i = 1; i <= result.head().getAtomCount(); ++i) {
		RExpression c(result.child(i));
		if (c.head().getDescriptor() == r_atom::I_PTR)
			result.child(i).head() = c.copy().iptr();
		else if (c.head().getDescriptor() == r_atom::VL_PTR)
			result.child(i).head() = c.copy().vptr();
	}
	return result;
}

void ExecutionContext::beginResultSet()
{
}

void ExecutionContext::appendResultSetElement(r_atom a)
{
	resultSet.push_back(a);
}

void ExecutionContext::endResultSet()
{
	if (head().getAtomCount() >= resultSet.size()) { // result fits in place
		env->value[index] = r_atom::Set(resultSet.size());
		for (size_t i = 0; i < resultSet.size(); ++i)
			env->value[index + 1 + i] = resultSet[i];
	} else { // result doesn't fit; use pointer -> (append to end)
		env->value[index] = r_atom::IPointer(env->value.size());
		env->value.push_back(r_atom::Set(resultSet.size()));
		for (size_t i = 0; i < resultSet.size(); ++i)
			env->value.push_back(resultSet[i]);
	}
}

size_t REhash::operator() ( const RExpression& x) const
{
	if (x.head().isPointer())
		return (*this)(x.dereference());
	unsigned long __h = x.head().atom;
	for (int i = 0; i <= x.head().getAtomCount(); ++i) {
		RExpression c(x.child(i));
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
