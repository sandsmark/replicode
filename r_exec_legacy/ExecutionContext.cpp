#include <stdio.h>
#include "ExecutionContext.h"
#include "OperatorRegister.h"

namespace r_exec {

using r_code::Atom;

ExecutionContext ExecutionContext::xchild(int offset) const
{
	ExecutionContext c(*this);
	c.index = index + offset;
	while (c.head().getDescriptor() == r_code::Atom::I_PTR) {
		c.setResult(c.head());
		c.index = c.head().asIndex();
	}
	return c;
}

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
		case Atom::S_SET:
			setResult(head());
			for (int i = 1; i <= head().getAtomCount(); ++i)
				evaluateOperand(i);
			break;
		case Atom::MARKER:
		case Atom::OBJECT:
			{
			setResult(head());
			for (int i = 1; i <= head().getAtomCount(); ++i)
				xchild(i).evaluate();
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
		case Atom::C_PTR:
			for (int i = 0; i <= head().getAtomCount(); ++i)
				instance->value[index + i] = instance->input[index + i];
			break;
		case Atom::DEVICE:
		case Atom::DEVICE_FUNCTION:
		case Atom::NIL:
		case Atom::VIEW:
		case Atom::VWS:
		case Atom::MKS:
		case Atom::R_PTR:
		case Atom::THIS:
			setResult(head());
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
	ExecutionContext p(xchild(index_));
	Expression r = p.evaluate();
	return r.dereference();
}

void ExecutionContext::setResultTimestamp(int64 timestamp)
{
	Atom	descriptor;
	switch(timestamp){
	case	-1:	descriptor=Atom::Forever();	break;
	case	-2:	descriptor=Atom::UndefinedTimestamp();	break;
	default:descriptor=Atom::Timestamp();	break;
	}

	if (head().getAtomCount() >= 2) {
		instance->value[index] = descriptor;
		instance->value[index+1] = Atom(timestamp >> 32);
		instance->value[index+2] = Atom(timestamp);
	} else {
		instance->value[index] = Atom::IPointer(instance->value.size());
		instance->value.push_back(descriptor);
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

void ExecutionContext::pushResultAtom(r_code::Atom a)
{
	instance->value.push_back(a);
}

Expression ExecutionContext::getEndExpression() const
{
	return Expression(instance, instance->value.size());
}

}
