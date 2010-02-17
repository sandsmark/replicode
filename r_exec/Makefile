TESTOBJS= SystemTest.o MemImpl.o CoreImpl.o ReductionInstance.o mbrane_imports/utils.o Expression.o \
	match.o ExecutionContext.o OperatorRegister.o Operators.o ../r_code/atom.o
CPPFLAGS=-g -Wno-deprecated -DARCH_32

rtest: $(TESTOBJS)
	c++ $(CPPFLAGS) $(TESTOBJS) -o rtest -lpthread

.cpp.o: Asynchronous.h ExecutionContext.h ObjectReceiver.h OperatorRegister.h Atom.h Core.h CoreImpl.h Expression.h Group.h Mem.h MemImpl.h Object.h ReductionInstance.h match.h opcodes.h operators.h types.h mbrane_imports/utils.h
