#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include "ReductionInstance.h"

namespace r_exec {

class Expression {
	friend class ReductionInstance;
public:
	Expression(ReductionInstance *instance_, int index_ = 0, bool isValue_ = false)
		:instance(instance_), index(index_), isValue(isValue_) {}
	r_code::Atom& head() const;
	Expression child(int index) const;
	int64 decodeTimestamp() const;
	void setValueAddressing(bool isValue_) { isValue = isValue_; }
	bool getValueAddressing() const { return isValue; }
	int getIndex() const { return index; }
	Expression dereference() const;
	Expression copy(ReductionInstance& dest) const;
	r_code::Atom iptr() const;
	r_code::Atom vptr() const;
	ReductionInstance& getInstance() { return *instance; }
protected:
	ReductionInstance *instance;
	int index;
	bool isValue;
};

inline Expression Expression::child(int offset) const {
    Expression c(*this);
    c.index = index + offset;
    if (c.head().isPointer())
        return c.dereference();
    else
        return c;
}

inline r_code::Atom& Expression::head() const {
    if (isValue) return instance->value[index];
    else return instance->input[index];
}

inline int64 Expression::decodeTimestamp() const
{
	return (static_cast<int64>(child(1).head().atom) << 32) + child(2).head().atom;
}

inline r_code::Atom Expression::iptr() const
{
	return r_code::Atom::IPointer(index);
}

inline r_code::Atom Expression::vptr() const
{
	return r_code::Atom::VLPointer(index);
}

}

#endif // __EXPRESSION_H
