/*
 *  asynchronous.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 22/12/2009.
 *
 * Instances of classes which implement the Asynchronous interface run in a
 * separate thread.  They run between calls to resume() and suspend(), and are
 * initialized in the suspended state.
 */

#ifndef __ASYNCHRONOUS_H
#define __ASYNCHRONOUS_H

namespace r_exec {

struct Asynchronous {
	virtual void resume() = 0;
	virtual void suspend() = 0;
};

}
#endif // __ASYNCHRONOUS_H
