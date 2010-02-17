#include <stdio.h>
#include "ExecutionContext.h"
#include "OperatorRegister.h"

namespace r_exec {

using r_code::Atom;

Expression ExecutionContext::evaluate()
{
	switch(head().getDescriptor()) {
		case Atom::OPERATOR:
			{
			uint16 opcode = head().asOpcode();
			Operator o(theOperatorRegister()[opcode]);
			if (o != 0) {
				setResult(Atom::Nil());
				o(*this);
			} else {
				fprintf(stderr, "operator(%d) NYI\n", opcode);
			}
			break;
			}
		case Atom::SET:
			setResult(head());
			for (int i = 1; i <= head().getAtomCount(); ++i)
				evaluateOperand(i);
			break;
		case Atom::C_PTR:
			{
			// first, copy the c_ptr structure to the value array
			for (int i = 0; i <= head().getAtomCount(); ++i)
				instance->value[index + i] = instance->input[index + i];

			// then, evaluate the pointer
			ExecutionContext p(*this);
			++p.index;
			p.evaluate();

			break;
			}
		case Atom::I_PTR:
			{
			ExecutionContext p(*this);
			p.index = head().asIndex();
			p.evaluate();
			} // fall through to VL_PTR (the only difference is that I_PTR gets evaluated while VL_PTR doesn't)
		case Atom::VL_PTR:
			if (instance->value[head().asIndex()].isStructural())
				setResult(head());
			else
				setResult(instance->value[head().asIndex()]);
			break;
		case Atom::TIMESTAMP:
			setResultTimestamp(decodeTimestamp());
			break;
		default:
			if (head().isFloat()) {
				setResult(head());
			} else {
				fprintf(stderr, "descriptor 0x%02x NYI\n", head().getDescriptor());
			}
	}
	Expression result(*this);
	result.setValueAddressing(true);
	return result;
}

Expression ExecutionContext::evaluateOperand(int index_)
{
	ExecutionContext p(*this);
	p.index = index + index_;
	Expression r = p.evaluate();
	return r.dereference();
}

void ExecutionContext::setResultTimestamp(int64 timestamp)
{
	if (head().getAtomCount() >= 2) {
		instance->value[index] = Atom::Timestamp();
		instance->value[index+1] = Atom(timestamp >> 32);
		instance->value[index+2] = Atom(timestamp);
	} else {
		instance->value[index] = Atom::IPointer(instance->value.size());
		instance->value.push_back(Atom::Timestamp());
		instance->value.push_back(Atom(timestamp >> 32));
		instance->value.push_back(Atom(timestamp));
	}
}

void ExecutionContext::beginResultSet()
{
}

void ExecutionContext::appendResultSetElement(Atom a)
{
	resultSet.push_back(a);
}

void ExecutionContext::endResultSet()
{
	if (head().getAtomCount() >= resultSet.size()) { // result fits in place
		instance->value[index] = Atom::Set(resultSet.size());
		for (size_t i = 0; i < resultSet.size(); ++i)
			instance->value[index + 1 + i] = resultSet[i];
	} else { // result doesn't fit; use pointer -> (append to end)
		instance->value[index] = Atom::IPointer(instance->value.size());
		instance->value.push_back(Atom::Set(resultSet.size()));
		for (size_t i = 0; i < resultSet.size(); ++i)
			instance->value.push_back(resultSet[i]);
	}
}

void ExecutionContext::merge(Expression value)
{
	if (head().getAtomCount() == value.head().getAtomCount()) {
		
	}
}

}
