CPPFLAGS=-g -std=c++0x -fpermissive -fPIC -DUSE_MEM
LIBS = ../r_comp/r_comp.so ../r_code/r_code.so ../r_exec/r_exec.so
Test: Test.o $(LIBS)
	g++ $(CPPFLAGS) Test.o $(LIBS) -o Test
