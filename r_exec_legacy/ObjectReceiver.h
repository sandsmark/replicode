/*
 *  ObjectReceiver.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 22/12/2009.
 *
 */

#ifndef __OBJECT_RECEIVER_H
#define __OBJECT_RECEIVER_H
#include <vector>
#include "fwd.h"
#include "atom.h"

namespace r_exec {

struct ObjectReceiver {
	typedef enum {
		INPUT_GROUP = 0,
		OUTPUT_GROUP = 1
	} Destination;
	
	// When calling receive(), the caller is expected to hold a persistent
	// reference to Object (with an associated retain()).
	virtual void receive(
		Object *object,
		std::vector<r_code::Atom> viewData,
		int node_id,
		Destination dest
	) = 0;
	virtual void beginBatchReceive() = 0;
	virtual void endBatchReceive() = 0;
};

}

#endif // __OBJECT_RECEIVER_H
