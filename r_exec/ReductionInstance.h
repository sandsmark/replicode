#ifndef __REDUCTION_INSTANCE_H
#define __REDUCTION_INSTANCE_H
#include <vector>
#include "../r_code/atom.h"
#include "fwd.h"

namespace r_exec {

namespace MemImpl { class ObjectImpl; class ObjectBase; class GroupImpl; }

template<class T> class boundsCheckedVector : public std::vector<T> {
	T& operator[](uint32 i) {
		if (i >= std::vector<T>::size())
			printf("out of bounds access\n");
		return std::vector<T>::operator[](i);
	}
};
		
class ReductionInstance {
	friend class Expression;
	friend class ExecutionContext;
	friend class MemImpl::ObjectBase;
	friend class MemImpl::ObjectImpl;
	friend class MemImpl::GroupImpl;
public:
	ReductionInstance(Group* g) :referenceCount(0), firstReusableCopiedObject(0), hash_value(0), group(g) {}
	ReductionInstance(const ReductionInstance& ri);
	~ReductionInstance();
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
	Object* extractObject(Expression expr); // generates the object whose head is given
	int16 getReferenceIndex(Object* o); // finds the index for o, creating a new reference if needed
	void syncSizes(); // enlarges the value or input array to make them the same size
	size_t hash() const;
	Group* getGroup() { return group; }
	void debug();
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
	Group* group;
};

}

#endif
