COMPILERTESTOBJS= objs/CompilerTest.o objs/compiler.o objs/rview.o objs/payload.o \
	objs/object.o objs/memory.o objs/class_register.o objs/utils.o objs/message.o \
	objs/compatibility.o
COMPILERTESTLIBS= -lpthread -ldl
CXXFLAGS=-std=c++0x

CompilerTest: $(COMPILERTESTOBJS)
	$(CXX) $(CXXFLAGS) $(COMPILERTESTOBJS) $(COMPILERTESTLIBS) -o CompilerTest 

objs/CompilerTest.o: Replicode/Test/Test.cpp
	$(CXX) $(CXXFLAGS) Replicode/Test/Test.cpp -c -o objs/CompilerTest.o

objs/compiler.o: Replicode/Replicode/compiler.cpp
	$(CXX) $(CXXFLAGS) Replicode/Replicode/compiler.cpp -c -o objs/compiler.o

objs/rview.o: Replicode/Replicode/rview.cpp
	$(CXX) $(CXXFLAGS) Replicode/Replicode/rview.cpp -c -o objs/rview.o

objs/payload.o: mbrane/Core/payload.cpp
	$(CXX) $(CXXFLAGS) mbrane/Core/payload.cpp -c -o objs/payload.o

objs/object.o: mbrane/Core/object.cpp
	$(CXX) $(CXXFLAGS) mbrane/Core/object.cpp -c -o objs/object.o

objs/memory.o: mbrane/Core/memory.cpp
	$(CXX) $(CXXFLAGS) mbrane/Core/memory.cpp -c -o objs/memory.o

objs/class_register.o: mbrane/Core/class_register.cpp
	$(CXX) $(CXXFLAGS) mbrane/Core/class_register.cpp -c -o objs/class_register.o

objs/utils.o: mbrane/Core/utils.cpp
	$(CXX) $(CXXFLAGS) mbrane/Core/utils.cpp -c -o objs/utils.o

objs/message.o: mbrane/Core/message.cpp
	$(CXX) $(CXXFLAGS) mbrane/Core/message.cpp -c -o objs/message.o

objs/compatibility.o: compatibility.cpp
	$(CXX) $(CXXFLAGS) compatibility.cpp -c -o objs/compatibility.o
