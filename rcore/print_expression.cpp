#include "print_expression.h"
#include <stdio.h>

using namespace r_code;

void print(const RExpression& e, int depth)
{
	if (e.head().isPointer()) {
		print(e.dereference(), depth);
		return;
	}
	printf("%03d%*s", e.getIndex(), 4*depth, "");
	const char* tail = "";
	switch(e.head().getDescriptor()) {
		case r_atom::SET:
			printf("[] ; (%d)\n", e.head().getAtomCount());
			break;
		case r_atom::UNDEFINED:
			if (e.getValueAddressing())
				printf("VALUE\n");
			else
				printf("undefined\n");
			break;
		case r_atom::OPERATOR:
			tail = ")";
			switch(e.head().asOpcode()) {
				case 14: printf("(red\n"); break;
				case 5:  printf("(<\n"); break;
				case 4: printf("(>\n"); break;
				default: printf("(operator{%x}\n", e.head().asOpcode()); break;
			}
			break;
		case r_atom::OBJECT:
			tail = ")";
			switch(e.head().asOpcode()) {
				case 6: printf("(ptn\n"); break;
				default: printf("(object{%x}\n", e.head().asOpcode()); break;
			}
			break;
		case r_atom::WILDCARD:
			printf("wildcard\n");
			break;
		case r_atom::TIMESTAMP:
			printf("timestamp(%.6f)\n", e.decodeTimestamp() * 1e-6);
			return; // avoid printing children
		default:
			if (e.head().isFloat())
				printf("float(%g)\n", e.head().asFloat());
			else
				printf("unknown descriptor %02x\n", e.head().getDescriptor());
			break;
	}
	for (int i = 1; i <= e.head().getAtomCount(); ++i)
		print(e.child(i), depth+1);
	if (*tail != '\0')
		printf("XXX%*s%s\n", 4*depth, "", tail);
}

void print_expression(AtomArray& a)
{
	ExecutionContext ctxt(a);
	print(ctxt, 0);
}
