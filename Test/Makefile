CPPFLAGS=-g -std=c++0x -fpermissive -fPIC
Test: Test.o $(LIBOBJS)
	g++ $(CPPFLAGS) Test.o ../r_comp/r_comp.so ../r_code/r_code.so -o Test
