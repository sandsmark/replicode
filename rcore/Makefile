TESTOBJS= testCore.o operator_add.o operator_equ.o operator_gte.o \
	operator_gtr.o operator_lse.o operator_lsr.o operator_now.o operator_sub.o \
	operator_red.o match.o print_expression.o \
	ExecutionContext.o OperatorRegister.o \
	r_core.o r_listener.o

CXXFLAGS=-g -Wall

testCore: $(TESTOBJS)
	g++ $(CXXFLAGS) $(TESTOBJS) -o testCore

testCore.o : r_code.h container.h
operator_add.o: r_code.h ExecutionContext.h container.h
operator_equ.o: r_code.h ExecutionContext.h container.h
operator_gte.o: r_code.h ExecutionContext.h container.h
operator_gtr.o: r_code.h ExecutionContext.h container.h
operator_lse.o: r_code.h ExecutionContext.h container.h
operator_lsr.o: r_code.h ExecutionContext.h container.h
operator_now.o: r_code.h ExecutionContext.h container.h
operator_sub.o: r_code.h ExecutionContext.h container.h
operator_red.o: r_code.h ExecutionContext.h container.h
ExecutionContext.o: r_code.h ExecutionContext.h container.h
OperatorRegister.o: r_code.h ExecutionContext.h opcodes.h container.h
match.o: r_code.h ExecutionContext.h opcodes.h container.h
print_expression.o: r_code.h ExecutionContext.h opcodes.h container.h
r_core.o: r_code.h ExecutionContext.h opcodes.h container.h r_core.h r_listener.h
r_listener.o: r_code.h ExecutionContext.h opcodes.h container.h r_core.h r_listener.h
