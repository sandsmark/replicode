OBJS = atom.o execution_context.o expression.o image.o object.o operator.o r_code.o reduction_instance.o vector.o
CPPFLAGS=-g -std=c++0x -fPIC

r_code.so: $(OBJS)
	g++ $(CPPFLAGS) -shared -o r_code.so $(OBJS)
