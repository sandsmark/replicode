#ifndef __REDUCTION_INSTANCE_H
#define __REDUCTION_INSTANCE_H
#include <vector>
#include "../r_code/atom.h"
#include "fwd.h"

namespace r_exec {

namespace MemImpl { class ObjectImpl; class ObjectBase; class GroupImpl; }

class ReductionInstance {
	friend class Expression;
	friend class ExecutionContext;
	friend class MemImpl::ObjectBase;
	friend class MemImpl::ObjectImpl;
	friend class MemImpl::GroupImpl;
public:
	ReductionInstance() :referenceCount(0), firstReusableCopiedObject(0), hash_value(0) {}
	// retain and release are not thread-safe, and do not allow for the
	// ReductionInstance to change after the first retain().  The reason
	// for this second restriction is that implementing it would require the
	// use of Object's retain() method, which isn't thread-safe, and this
	// would then mean that modification of a ReductionInstance not thread-safe.
	void retain();
	void release();

	struct ptr_hash {
		size_t operator()(const ReductionInstance* ri) const;
	};
	ReductionInstance* reduce(Object* input);
	ReductionInstance* reduce(std::vector<ReductionInstance*> inputs);
	ReductionInstance* split(ExecutionContext location);
	void merge(ExecutionContext location, ReductionInstance* ri);
	Object* objectForExpression(Expression expr);
	size_t hash() const;
private:
	struct CopiedObject {
		Object* object;
		int position;
	};
	
	int referenceCount;
	std::vector<r_code::Atom> input;
	std::vector<r_code::Atom> value;
	std::vector<CopiedObject> copies;
	int firstReusableCopiedObject;
	std::vector<Object*> references;
	size_t hash_value;
};

}

#endif
