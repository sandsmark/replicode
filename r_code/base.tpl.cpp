//	base.tpl.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

namespace	r_code{

	template<class	C>	inline	P<C>::P():object(NULL){
	}

	template<class	C>	inline	P<C>::P(C	*o):object(o){

		if(object)
			object->incRef();
	}

	template<class	C>	inline	P<C>::P(const P<C>	&p):object(p.object){

		if(object)
			object->incRef();
	}

	template<class	C>	inline	P<C>::~P(){

		if(object)
			object->decRef();
	}

	template<class	C>	inline	C	*P<C>::operator	->()	const{

		return	(C	*)object;
	}

	template<class	C>	inline	bool	P<C>::operator	==(C	*c)	const{

		return	object==c;
	}

	template<class	C>	inline	bool	P<C>::operator	!=(C	*c)	const{

		return	object!=c;
	}

	template<class	C>	template<class	D>	inline	bool	P<C>::operator	==(P<D>	&p)	const{

		return	object==p.object;
	}

	template<class	C>	template<class	D>	inline	bool	P<C>::operator	!=(P<D>	&p)	const{

		return	object!=p.object;
	}

	template<class	C>	inline	bool	P<C>::operator	!()	const{

		return	!object;
	}

	template<class	C>	inline	P<C>&	P<C>::operator	=(C	*c){

		if(object==c)
			return	*this;
		if(object)
			object->decRef();
		if(object=c)
			object->incRef();

		return	*this;
	}

	template<class	C>	template<class	D>	inline	P<C>	&P<C>::operator	=(const P<D>	&p){

		return	this->operator	=((C	*)p.object);
	}

	template<class	C>	inline	P<C>	&P<C>::operator	=(const P<C>	&p){

		return	this->operator	=((C	*)p.object);
	}
}