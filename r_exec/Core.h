/*
 *  Core.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 20/12/2009.
 *
 * The computational work of the system is done by the Core.
 * There should be one Core per node, and one Core::Instance per group.
 *
 * The Core and Core::Instance interfaces are not multithread-safe.
 * In particular, it isn't safe to use different Core::Instance interfaces
 * from different threads (this second restriction could be lifted if necessary)
 *
 * Core is internally multi-threaded, controlled by the Asynchronous interface.
 */
 
#ifndef __CORE_H
#define __CORE_H
#include "fwd.h"
#include "Asynchronous.h"

namespace r_exec {

struct Core : public Asynchronous {
	struct Instance {
		virtual ~Instance() {}
		// activate() and deactivate() tell the Instance that a given program is
		// (or is not) active.  The activation or de-activation will take effect
		// after all inputs currently pending have been processed.
		virtual void activate(Object* program) = 0;
		virtual void deactivate(Object* program) = 0;
		// salientObject presents the given object as a possible input to all
		// of the active programs in the Instance.  The result of processing is
		// returned asynchronously, by a call to Group::reductionComplete() 
		virtual void salientObject(Object* object) = 0;
	};
	static Core* create();
	virtual ~Core() {}
	virtual Instance* createInstance(Group* group) = 0;
};

}

#endif // __CORE_H
