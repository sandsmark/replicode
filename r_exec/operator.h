#ifndef	operator_h
#define	operator_h

#include	"context.h"


namespace	r_exec{

	class	Operator{
	private:
		static	std::vector<Operator>	Operators;	//	indexed by opcodes.

		bool	(*_operator)(const	Context	&,uint16	&);
		bool	(*_overload)(const	Context	&,uint16	&);
	public:
		static	void		Register(uint16	opcode,bool	(*op)(const	Context	&,uint16	&));	//	first, register std operators; next register user-defined operators (may be registered as overloads).
		static	Operator	Get(uint16	opcode){	return	Operators[opcode];	}
		Operator():_operator(NULL),_overload(NULL){}
		Operator(bool	(*o)(const	Context	&,uint16	&)):_operator(o),_overload(NULL){}
		~Operator(){}

		void	setOverload(bool	(*o)(const	Context	&,uint16	&)){	_overload=o;	}

		bool	operator	()(const	Context	&context,uint16	&index)	const{
			if(!_operator(context,index))
				return	true;
			if(_overload)
				return	_overload(context,index);
			return	false;
		}
	};

	//	std operators	////////////////////////////////////////

	bool	now(const	Context	&context,uint16	&index);

	bool	equ(const	Context	&context,uint16	&index);
	bool	neq(const	Context	&context,uint16	&index);
	bool	gtr(const	Context	&context,uint16	&index);
	bool	lsr(const	Context	&context,uint16	&index);
	bool	gte(const	Context	&context,uint16	&index);
	bool	lse(const	Context	&context,uint16	&index);

	bool	add(const	Context	&context,uint16	&index);
	bool	sub(const	Context	&context,uint16	&index);
	bool	mul(const	Context	&context,uint16	&index);
	bool	div(const	Context	&context,uint16	&index);

	bool	dis(const	Context	&context,uint16	&index);

	bool	ln(const	Context	&context,uint16	&index);
	bool	exp(const	Context	&context,uint16	&index);
	bool	log(const	Context	&context,uint16	&index);
	bool	e10(const	Context	&context,uint16	&index);

	bool	syn(const	Context	&context,uint16	&index);	//	TODO.

	bool	ins(const	Context	&context,uint16	&index);

	bool	at(const	Context	&context,uint16	&index);	//	TODO.

	bool	red(const	Context	&context,uint16	&index);
	
	bool	com(const	Context	&context,uint16	&index);	//	TODO.

	bool	spl(const	Context	&context,uint16	&index);	//	TODO.
	bool	mrg(const	Context	&context,uint16	&index);	//	TODO.

	bool	ptc(const	Context	&context,uint16	&index);	//	TODO.

	bool	fvw(const	Context	&context,uint16	&index);
}


#endif