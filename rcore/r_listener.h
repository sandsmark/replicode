#ifndef __R_LISTENER_H
#define __R_LISTENER_H
#include "r_core.h"
#include "ExecutionContext.h"

class r_listener {
public:
	static r_listener *create(r_core *core, int index, ExecutionContext& pattern);
	virtual ~r_listener() {}
	static void onMessage(AtomArray& message);
};
#endif // __R_LISTENER_H
