#ifndef __EXECUTION_CONTEXT_H
#define __EXECUTION_CONTEXT_H
#include "r_code.h"
#include "container.h"

class ExecutionEnvironment {
public:
	ExecutionEnvironment(AtomArray& input_) :input(input_), value(input_.size(), r_code::r_atom::Undefined()) {}
	AtomArray input;
	AtomArray value;
};

class RExpression {
public:
	RExpression(ExecutionEnvironment *env_, int index_ = 0, bool isValue_ = false) :env(env_), index(index_), isValue(isValue_) {}
	r_code::r_atom& head() const;
	RExpression child(int index) const;
	int64 decodeTimestamp() const;
	void setValueAddressing(bool isValue_) { isValue = isValue_; }
	bool getValueAddressing() const { return isValue; }
	int getIndex() const { return index; }
	RExpression dereference() const;
	RExpression copy() const;
	r_code::r_atom iptr() const;
	r_code::r_atom vptr() const;
protected:
	ExecutionEnvironment *env;
	int index;
	bool isValue;
	std::vector<r_code::r_atom> resultSet;
};

bool operator == (const RExpression& x, const RExpression& y);

class REhash {
public:
	size_t operator() (const RExpression&) const;
};

class ExecutionContext : public RExpression {
public:
	ExecutionContext(AtomArray input_);
	ExecutionContext(const ExecutionContext& e) :RExpression(e), ownsEnv(false) {}
	~ExecutionContext() { if (ownsEnv) delete env; }
	RExpression evaluateOperand(int index);
	RExpression evaluate();
	ExecutionContext xchild(int index) const;

	void setResult(r_code::r_atom result);
	void setResultTimestamp(int64 timestamp);
	void beginResultSet();
	void appendResultSetElement(r_code::r_atom element);
	void endResultSet();
	void undefinedResult();

	AtomArray getValueArray() { return env->value; } // TODO: remove this after integration
	int64 now();
private:
	bool ownsEnv;
};

inline r_code::r_atom& RExpression::head() const {
	if (isValue) return env->value[index];
	else return env->input[index];
}
inline RExpression RExpression::child(int offset) const {
	RExpression c(*this);
	c.index = index + offset;
	if (c.head().isPointer())
		return c.dereference();
	else
		return c;
}

inline ExecutionContext ExecutionContext::xchild(int offset) const
{
	ExecutionContext c(*this);
	c.index = index + offset;
	while (c.head().getDescriptor() == r_code::r_atom::I_PTR)
		c.index = c.head().asIndex();
	return c;
}

inline void ExecutionContext::setResult(r_code::r_atom result) { env->value[index] = result; }

inline int64 RExpression::decodeTimestamp() const
{
	return (static_cast<int64>(child(1).head().atom) << 32) + child(2).head().atom;
}

inline r_code::r_atom RExpression::iptr() const
{
	return r_code::r_atom::IPointer(index);
}

inline r_code::r_atom RExpression::vptr() const
{
	return r_code::r_atom::VLPointer(index);
}
#endif
