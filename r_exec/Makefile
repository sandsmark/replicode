TESTOBJS= MemImpl.o CoreImpl.o ReductionInstance.o mbrane_imports/utils.o Expression.o \
	match.o ExecutionContext.o OperatorRegister.o Operators.o 
CPPFLAGS=-g -Wno-deprecated -std=c++0x -fPIC

r_exec.so: $(TESTOBJS)
	c++ $(CPPFLAGS) $(TESTOBJS) -shared -o r_exec.so -lpthread

.cpp.o: Asynchronous.h ExecutionContext.h ObjectReceiver.h OperatorRegister.h Atom.h Core.h CoreImpl.h Expression.h Group.h Mem.h MemImpl.h Object.h ReductionInstance.h match.h opcodes.h operators.h types.h mbrane_imports/utils.h
