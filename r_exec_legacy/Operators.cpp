#include "ExecutionContext.h"
#include "match.h"
#include "hash_containers"
#include	"utils.h"

namespace r_exec {

using r_code::Atom;
using namespace std;

void operator_add(ExecutionContext& context)
{
	Expression lhs(context.evaluateOperand(1));
	Expression rhs(context.evaluateOperand(2));

	if(lhs.head().readsAsNil()	||	rhs.head().readsAsNil())
		context.setResult(Atom::UndefinedFloat());

	if (lhs.head().isFloat()) {
		if (rhs.head().isFloat()) {
			context.setResult(Atom::Float( rhs.head().asFloat() + lhs.head().asFloat()));
		} else if (rhs.head().getDescriptor() == Atom::TIMESTAMP) {
			if(rhs.head()!=Atom::Forever())
				context.setResultTimestamp( int(lhs.head().asFloat()) + rhs.decodeTimestamp() );
			else
				context.setResultTimestamp(-1);	//	forever
		}
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if (rhs.head().isFloat()) {
			if(lhs.head()!=Atom::Forever())
				context.setResultTimestamp( lhs.decodeTimestamp() + int(rhs.head().asFloat()));
			else
				context.setResultTimestamp(-1);	//	forever
		} else if (rhs.head().getDescriptor() == Atom::TIMESTAMP) {
			if(lhs.head()!=Atom::Forever()	&&	rhs.head()!=Atom::Forever())
				context.setResultTimestamp( lhs.decodeTimestamp() + rhs.decodeTimestamp() );
			else
				context.setResultTimestamp(-1);	//	forever
		}
	}
}

void operator_sub(ExecutionContext& context)
{
	Expression lhs(context.evaluateOperand(1));
	Expression rhs(context.evaluateOperand(2));

	if(lhs.head().readsAsNil()	||	rhs.head().readsAsNil())
		context.setResult(Atom::UndefinedFloat());

	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult( Atom::Float( lhs.head().asFloat() - rhs.head().asFloat() ));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(lhs.head()==Atom::Forever())
			context.setResultTimestamp(-1);	//	forever; NB: forever-forever=forever
		else	if(rhs.head()==Atom::Forever())
			context.setResultTimestamp(0);	//	NB: n-forever=0
		else{
			double diff = static_cast<float>( lhs.decodeTimestamp() ) - static_cast<float>( rhs.decodeTimestamp() );
			context.setResultTimestamp( diff );
		}
	}
}
void operator_mul(ExecutionContext& context)
{
	Expression lhs(context.evaluateOperand(1));
	Expression rhs(context.evaluateOperand(2));

	if(lhs.head().readsAsNil()	||	rhs.head().readsAsNil())
		context.setResult(Atom::UndefinedFloat());

	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult( Atom::Float( lhs.head().asFloat() * rhs.head().asFloat() ));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(rhs.head().getDescriptor() == Atom::TIMESTAMP)
			context.setResultTimestamp(-2);	//	undefined value
		else	if(lhs.head()==Atom::Forever())
			context.setResultTimestamp(-1);	//	forever; NB: forever*n=forever	
		else
			context.setResultTimestamp(lhs.decodeTimestamp()*rhs.head().asFloat());
	} else if (rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(rhs.head()==Atom::Forever())
			context.setResultTimestamp(-1);	//	forever; NB: forever*n=forever
		else
			context.setResultTimestamp(rhs.decodeTimestamp()*lhs.head().asFloat());
	}
}

void operator_div(ExecutionContext& context)
{
	Expression lhs(context.evaluateOperand(1));
	Expression rhs(context.evaluateOperand(2));

	if(lhs.head().readsAsNil()	||	rhs.head().readsAsNil())
		context.setResult(Atom::UndefinedFloat());

	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		if(rhs.head().asFloat()==0)
			context.setResult(Atom::UndefinedFloat());
		else
			context.setResult( Atom::Float( lhs.head().asFloat() / rhs.head().asFloat() ));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(rhs.head().getDescriptor() == Atom::TIMESTAMP)
			context.setResultTimestamp(-2);	//	undefined value
		else	if(lhs.head()==Atom::Forever())
			context.setResultTimestamp(-1);	//	forever; NB: forever/n=forever	
		else	if(rhs.head().asFloat()==0)
			context.setResult(Atom::UndefinedFloat());
		else
			context.setResultTimestamp(lhs.decodeTimestamp()/rhs.head().asFloat());
	} else if (rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		context.setResult(Atom::UndefinedFloat());
	}
}

bool operator == (const Expression& lhs, const Expression& rhs)
{
	if (lhs.getIndex() == rhs.getIndex() && lhs.getValueAddressing() == rhs.getValueAddressing())
		return true;
	if (lhs.head().atom != rhs.head().atom)
		return false;
	if (lhs.head().isStructural()) {
		for (int i = 1; i <= lhs.head().getAtomCount(); ++i) {
			if (! (lhs.child(i) == rhs.child(i)))
				return false;
		}
	}
	return true;
}

void operator_equ(ExecutionContext& context)
{
	context.setResult(
		Atom::Boolean(
			context.evaluateOperand(1) == context.evaluateOperand(2)
		)
	);
}

void operator_neq(ExecutionContext& context)
{
	context.setResult(
		Atom::Boolean(! (
			context.evaluateOperand(1) == context.evaluateOperand(2)
		))
	);
}

void operator_gte(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() >= rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(lhs.head()==Atom::Forever())
			context.setResult(Atom::Boolean(true));
		else	if(rhs.head()==Atom::Forever())
			context.setResult(Atom::Boolean(false));
		else
			context.setResult(Atom::Boolean(lhs.decodeTimestamp() >= rhs.decodeTimestamp()));
	}
}

void operator_gtr(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() > rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(lhs.head()==Atom::Forever()){
			if(rhs.head()==Atom::Forever())
				context.setResult(Atom::Boolean(false));
			else
				context.setResult(Atom::Boolean(true));
		}else	if(rhs.head()==Atom::Forever())
			context.setResult(Atom::Boolean(false));
		else
			context.setResult(Atom::Boolean(lhs.decodeTimestamp() > rhs.decodeTimestamp()));
	}
}

void operator_lse(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() <= rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(lhs.head()==Atom::Forever())
			context.setResult(Atom::Boolean(false));
		else	if(rhs.head()==Atom::Forever())
			context.setResult(Atom::Boolean(true));
		else
			context.setResult(Atom::Boolean(lhs.decodeTimestamp() <= rhs.decodeTimestamp()));
	}
}

void operator_lsr(ExecutionContext& context)
{
	Expression lhs = context.evaluateOperand(1);
	Expression rhs = context.evaluateOperand(2);
	if (lhs.head().isFloat() && rhs.head().isFloat()) {
		context.setResult(Atom::Boolean(lhs.head().asFloat() < rhs.head().asFloat()));
	} else if (lhs.head().getDescriptor() == Atom::TIMESTAMP && rhs.head().getDescriptor() == Atom::TIMESTAMP) {
		if(lhs.head()==Atom::Forever()){
			if(rhs.head()==Atom::Forever())
				context.setResult(Atom::Boolean(false));
			else
				context.setResult(Atom::Boolean(false));
		}else	if(rhs.head()==Atom::Forever())
			context.setResult(Atom::Boolean(true));
		else
			context.setResult(Atom::Boolean(lhs.decodeTimestamp() < rhs.decodeTimestamp()));
	}
}

void operator_now(ExecutionContext& context)
{
	int64	now=Time::Get();
	context.setResultTimestamp( now );
}

struct Ehash {
	size_t operator() (const Expression&) const;
};

size_t Ehash::operator() ( const Expression& x) const
{
	if (x.head().isPointer())
		return (*this)(x.dereference());
	unsigned long __h = x.head().atom;
	for (int i = 0; i <= x.head().getAtomCount(); ++i) {
		Expression c(x.child(i));
		uint32 d;
		if (c.head().isPointer()) {
			d = (*this)(c.dereference());
		} else {
			d = c.head().atom;
		}
		__h = 5 * __h + d;
	}
	return __h;
}

void evaluateProductions(ExecutionContext& resultContext, ExecutionContext& prods)
{
	UNORDERED_SET<Expression, Ehash> produced;
	for (int i = 1; i <= prods.head().getAtomCount(); ++i) {
		ExecutionContext prod(prods.xchild(i));
		prod.evaluate();

		Expression prodv = prod;
		prodv.setValueAddressing(true);

		// see if we've produced this result before
		if (produced.find(prodv) != produced.end())
			continue;

		// we will be be over-writing prodv if we re-evaluate this production later, so we must make a copy
		prodv = prodv.copy(prodv.getInstance());

		// put this produced element into the produced set, so we won't produce it again
		produced.insert(prodv);
		resultContext.appendResultSetElement(prodv.iptr());
	}
}

void operator_red(ExecutionContext& context)
{
	context.beginResultSet();

	Expression inputs(context.evaluateOperand(1));
	ExecutionContext positiveSection(context.xchild(2));
	ExecutionContext negativeSection(context.xchild(3));
	for (int i = 1; i <= inputs.head().getAtomCount(); ++i) {
		Expression input(inputs.child(i));
		if (positiveSection.head().getAtomCount() == 2) {
			ExecutionContext positivePatterns(positiveSection.xchild(1));
			ExecutionContext positiveProds(positiveSection.xchild(2));
			bool positivePatternsMatch = true;
			for (int j = 1; positivePatternsMatch && j <= positivePatterns.head().getAtomCount(); ++j) {
				ExecutionContext pattern(positivePatterns.xchild(j));
				if (!match(input, pattern))
					positivePatternsMatch = false;
			}

			if (positivePatternsMatch) {
				evaluateProductions(context, positiveProds);
			} else {
				bool negativePatternsMatch = true;
				ExecutionContext negativePatterns(negativeSection.xchild(1));
				ExecutionContext negativeProds(negativeSection.xchild(2));
				for (int j = 1; negativePatternsMatch && j <= negativePatterns.head().getAtomCount(); ++j) {
					ExecutionContext pattern(negativePatterns.xchild(j));
					if (!match(input, pattern))
						negativePatternsMatch = false;
				}
				if (negativePatternsMatch) {
					evaluateProductions(context, negativeProds);
				}
			}
		}
	}

	context.endResultSet();
}

void operator_ins(ExecutionContext& context)
{
	Expression pgm(context.evaluateOperand(1));
	Expression templateParameters(context.evaluateOperand(2));
	int numParameters = templateParameters.head().getAtomCount();
	vector<Expression> formalParameters;
	for (int i = 1; i <= numParameters; ++i) {
		formalParameters.push_back(context.getEndExpression());
		context.pushResultAtom(opcodeRegister["val_pair"]);
		context.pushResultAtom(templateParameters.child(i).iptr());
		context.pushResultAtom(Atom::Float(i));
	}
	Expression formalParameterSet = context.getEndExpression();
	context.pushResultAtom(Atom::Set(numParameters));
	for (int i = 0; i < formalParameters.size(); ++i)
		context.pushResultAtom(formalParameters[i].iptr());
	Expression result(context.getEndExpression());
	Atom a = pgm.head();
	if (a == opcodeRegister["pgm"] || a == opcodeRegister["|pgm"]) {
		context.pushResultAtom(opcodeRegister["ipgm"]);
	} else if (a == opcodeRegister["fmd"] || a == opcodeRegister["|fmd"]) {
		context.pushResultAtom(opcodeRegister["ifmd"]);
	} else if (a == opcodeRegister["imd"] || a == opcodeRegister["|imd"]) {
		context.pushResultAtom(opcodeRegister["iimd"]);
	} else if (a == opcodeRegister["gol"] || a == opcodeRegister["|gol"]) {
		context.pushResultAtom(opcodeRegister["igol"]);
	} else {
		printf("ins: unrecognized reactive object type %08x\n", pgm.head().atom);
		context.pushResultAtom(Atom::Nil());
	}
	context.pushResultAtom(pgm.iptr());
	context.pushResultAtom(formalParameterSet.iptr());
	context.pushResultAtom(Atom::View());
	context.pushResultAtom(Atom::Mks());
	context.pushResultAtom(Atom::Vws());
	context.pushResultAtom(Atom::Float(0));
	context.setResult(result.iptr());
}

}
