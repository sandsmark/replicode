#ifndef	operator_h
#define	operator_h

#include	"context.h"


namespace	r_exec{

	class	Operator{
	private:
		static	std::vector<Operator>	Operators;	//	indexed by opcodes.

		bool	(*_operator)(const	Context	&);
		bool	(*_overload)(const	Context	&);
	public:
		static	void		Register(uint16	opcode,bool	(*op)(const	Context	&));	//	first, register std operators; next register user-defined operators (may be registered as overloads).
		static	Operator	Get(uint16	opcode){	return	Operators[opcode];	}
		Operator():_operator(NULL),_overload(NULL){}
		Operator(bool	(*o)(const	Context	&)):_operator(o),_overload(NULL){}
		~Operator(){}

		void	setOverload(bool	(*o)(const	Context	&)){	_overload=o;	}

		bool	operator	()(const	Context	&context)	const{
			if(_operator(context))
				return	true;
			if(_overload)
				return	_overload(context);
			return	false;
		}
	};

	//	std operators	////////////////////////////////////////

	bool	now(const	Context	&context);

	bool	equ(const	Context	&context);
	bool	neq(const	Context	&context);
	bool	gtr(const	Context	&context);
	bool	lsr(const	Context	&context);
	bool	gte(const	Context	&context);
	bool	lse(const	Context	&context);

	bool	add(const	Context	&context);
	bool	sub(const	Context	&context);
	bool	mul(const	Context	&context);
	bool	div(const	Context	&context);

	bool	dis(const	Context	&context);

	bool	ln(const	Context	&context);
	bool	exp(const	Context	&context);
	bool	log(const	Context	&context);
	bool	e10(const	Context	&context);

	bool	syn(const	Context	&context);	//	TODO.

	bool	ins(const	Context	&context);

	bool	at(const	Context	&context);	//	TODO.

	bool	red(const	Context	&context);
	
	bool	com(const	Context	&context);	//	TODO.

	bool	spl(const	Context	&context);	//	TODO.
	bool	mrg(const	Context	&context);	//	TODO.

	bool	ptc(const	Context	&context);	//	TODO.

	bool	fvw(const	Context	&context);
}


#endif