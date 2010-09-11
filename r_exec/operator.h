//	operator.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef	operator_h
#define	operator_h

#include	"context.h"


namespace	r_exec{

	bool	red(const	Context	&context,uint16	&index);

	bool	syn(const	Context	&context,uint16	&index);

	class	Operator{
	private:
		static	r_code::vector<Operator>	Operators;	//	indexed by opcodes.

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
			if(_operator(context,index))
				return	true;
			if(_overload)
				return	_overload(context,index);
			return	false;
		}

		bool	is_red()	const{	return	_operator==red;	}
		bool	is_syn()	const{	return	_operator==syn;	}
	};

	//	std operators	////////////////////////////////////////

	bool	now(const	Context	&context,uint16	&index);

	bool	rnd(const	Context	&context,uint16	&index);

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

	bool	ins(const	Context	&context,uint16	&index);
	
	bool	fvw(const	Context	&context,uint16	&index);
}


#endif