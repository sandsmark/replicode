/*
 *  Group.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 15/12/2009.
 *
 * The Group interface is used by the Core to return the result of reduction
 * to the Mem.  It's not thread-safe (though it should be straightforward
 * to permit simultaneous access to different Group interfaces)
 */

#ifndef __GROUP_H
#define __GROUP_H
#include "fwd.h"
#include <vector>

namespace r_exec {

struct Group {
	// receive is used to send reduced programs back to the Mem so that
	// the commands and notifications can be injected.  Once this is done,
	// the Mem will release the ReductionInstance.
	virtual void receive(ReductionInstance *reduction) = 0;
	
	virtual Object* asObject() = 0;
};

}

#endif // __GROUP_H
