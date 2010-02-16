OBJS= class.o compiler.o decompiler.o preprocessor.o r_comp.o segments.o string_utils.o structure_member.o
CPPFLAGS=-g -std=c++0x -fPIC

r_comp.so: $(OBJS)
	g++ $(CPPFLAGS) -shared -o r_comp.so $(OBJS)
