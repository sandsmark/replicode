#ifndef __R_CORE_H
#define __R_CORE_H
#include "r_code.h"
#include "container.h"
#include "ExecutionContext.h"

// these interfaces will change as r_mem is implemented.

class r_core {
public:
	static r_core *create(AtomArray& program);
	virtual void onInput(int index, AtomArray& input) = 0;
	virtual ~r_core() {}
};
#endif // __R_CORE_H
