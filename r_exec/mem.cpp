#include "mem.h"

using namespace r_exec;

Mem* Mem::Get()
{
	static Mem theMem;
	return &theMem;
}

void Mem::receive (r_code::Object *obj)
{
}
