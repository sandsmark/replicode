#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include "r_code.h"
#include "ExecutionContext.h"
#include "print_expression.h"

using namespace std;
using namespace r_code;

void readAtoms(const char* fileName, AtomArray& array)
{
	FILE* fp = fopen(fileName, "r");
	if (!fp) {
		perror(fileName);
		return;
	}
	char buf[1000];
	while (fgets(buf, sizeof(buf), fp)) {
		unsigned lineNumber;
		int atomCode;
		if (2 != sscanf(buf, "%u: [%x]", &lineNumber, &atomCode)) {
			fprintf(stderr, "invalid format: %s", buf);
			return;
		}
		if (lineNumber != array.size()) {
			fprintf(stderr, "invalid line number: %s", buf);
			return;
		}
		array.push_back(r_atom(atomCode));
	}
}

int main(int argc, const char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "testCore inputAtoms correctOutputAtoms\n");
		exit(1);
	}
	AtomArray input;
	AtomArray correctOutput;
	readAtoms(argv[1], input);
	readAtoms(argv[2], correctOutput);

	ExecutionContext exe(input);
	switch(input[0].getDescriptor()) {
		case r_atom::SET:
		case r_atom::OPERATOR:
			exe.evaluate();
			break;
		default:
			fprintf(stderr, "descriptor %02x NYI\n", input[0].getDescriptor());
			exit(1);
	}
	AtomArray output = exe.getValueArray();
	bool pass = true;
	if (output.size() != correctOutput.size()) {
		fprintf(stderr, "output size (%d) != correct output size (%d)\n",
			int(output.size()), int(correctOutput.size()));
		pass = false;
	}
	for (unsigned i = 0; pass && i < output.size(); ++i)  {
		if (output[i].atom != correctOutput[i].atom) {
			pass = false;
		}
	}
	if (!pass) {
		for (unsigned i = 0; i < output.size() && i < correctOutput.size(); ++i) {
			fprintf(stderr, "%d: output(%08x) %s correctOutput(%08x)\n",
				i, output[i].atom, output[i].atom == correctOutput[i].atom ? " =" : "!=", correctOutput[i].atom);
		}
		printf("OUTPUT\n");
		print_expression(output);
		printf("CORRECT OUTPUT\n");
		print_expression(correctOutput);
		exit(1);
	}
	exit(0);
}
