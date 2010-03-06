/*
 *  Object.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 15/12/2009.
 *
 * The Object interface is used by the System and Core; the implementation
 * is private to the Mem.  The interface is not thread-safe -- callers must
 * arrange that:
 *  * calls to the non-const member functions are serial
 *  * calls to const member functions cannot occur at the same time as
 *    non-const member functions, or while the Mem could be modifying the
 *    underlying object.
 *
 * In practice, this constraint is achieved by:
 *  * The System retains references to objects, but only uses the interface
 *    during function calls from the Mem, and for newly-created objects
 *    not yet known to the Mem.
 *  * The Core retains references to objects, but only uses:
 *    * the const member functions while the Core is resumed (during which
 *      time the Mem is inactive)
 *    * retain() and release() in the final part of Core::suspend(), after all
 *      threads have been suspended and before returning control to the Mem
 */

#ifndef __ROBJECT_H
#define __ROBJECT_H
#include "fwd.h"
#include "../r_code/atom.h"
#include <vector>

namespace r_exec {

struct Object {
	// creates a new object, with (possibly-empty) references to other objects.
	// The objects referred to are not accessed or modified; in particular,
	// their reference counts are not incremented.  This is necessary --
	// create() is called spontaneously by the System while the Mem or
	// Core may be running, and so these operations wouldn't be safe.
	//
	// But will those referenced objects be improperly destroyed?  No:
	//  * The System had previously retained a reference to the object
	//  * The System notices that the referenced objects are currently in-use,
	//    and avoids releasing references to such objects for a period of time
	//  * The Mem accepts the newly-created object, performing the needed
	//    reference count increments (and marker set updates, and ...)
	//    at a high frequency, before the System could possibly release
	//    the reference.
	static Object* create(
		std::vector<r_code::Atom> atoms,
		std::vector<Object*> references
	);
	
	// retain and release increase and decrease the reference count.
	// if the reference count reaches 0, the object is destroyed.
	virtual void retain(const char* msg) = 0;
	virtual void release(const char* msg) = 0;

	// TODO: methods for accessing object content for transmission
	// copy copies the content of the object.
	// copy (and all of the other copy... methods) copies the requested Atoms
	// into the provided ReductionInstance, and returns an Expression
	// pointing at the head of the copied expression.
	virtual Expression copy(ReductionInstance& dest) const = 0;
	
	// copyMarkerSet returns a set of references to all objects which mark
	// this object.
	virtual Expression copyMarkerSet(ReductionInstance& dest) const = 0;
	
	// copyViewSet returns all views of the object
	virtual Expression copyViewSet(ReductionInstance& dest) const = 0;
	
	// copyVisibleView returns:
	// * the view of the object for the group, if one exists
	// * otherwise, the most-salient view of the object in a c-salient group
	//   visible from the specified group, if one exists
	// * otherwise, a nil-filled view
	virtual Expression copyVisibleView(ReductionInstance& dest, const Group* group) const = 0;
	
	// copyLocalView returns:
	// * the view of the object for the group, if one exists
	// * otherwise, a nil-filled view
	virtual Expression copyLocalView(ReductionInstance& dest, const Group* group) const = 0;
	
	virtual Object* getReference(int index) const = 0;
	
	// returns the object as a group, if it is one.  If not, returns NULL.
	virtual const Group* asGroup() const = 0;
};

}

#endif // __ROBJECT_H
