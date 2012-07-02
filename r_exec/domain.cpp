//	domain.cpp
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

#include	"domain.h"


namespace	r_exec{
/*
	void	Domain::add(BindingMap	*bm){

		for(uint32	i=0;i<ranges.size();++i)
			ranges[i]->add(bm,i);
	}

	void	Domain::remove(BindingMap	*bm){

		for(uint32	i=0;i<ranges.size();++i)
			ranges[i]->remove(bm,i);
	}

	bool	Domain::contains(BindingMap	*bm)	const{

		if(ranges.size()==0)
			return	false;

		for(uint32	i=0;i<ranges.size();++i){

			if(!ranges[i]->contains(bm,i));
				return	false;
		}

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	template<>	void	DRange<bool>::add(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			add(a->asBoolean());	
	}

	template<>	void	DRange<bool>::remove(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			remove(a->asBoolean());
	}

	template<>	bool	DRange<bool>::contains(BindingMap	*bm,uint32	i)	const{
		
		Atom	*a=bm->get_code(i);
		if(a)
			return	contains(a->asBoolean());
		return	true;
	}

	template<>	void	DRange<std::string>::add(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			add(Utils::GetString(a));	
	}

	template<>	void	DRange<std::string>::remove(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			remove(Utils::GetString(a));
	}

	template<>	bool	DRange<std::string>::contains(BindingMap	*bm,uint32	i)	const{
		
		Atom	*a=bm->get_code(i);
		if(a)
			return	contains(Utils::GetString(a));	
		return	true;
	}

	template<>	void	DRange<P<Code> >::add(BindingMap	*bm,uint32	i){
	
		P<Code>	o=bm->get_object(i);
		if(!!o)
			add(o);
	}

	template<>	void	DRange<P<Code> >::remove(BindingMap	*bm,uint32	i){
		
		P<Code>	o=bm->get_object(i);
		if(!!o)
			remove(o);	
	}

	template<>	bool	DRange<P<Code> >::contains(BindingMap	*bm,uint32	i)	const{
		
		P<Code>	o=bm->get_object(i);
		if(!!o)
			return	contains(o);
		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	template<>	void	CRange<float32>::add(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			add(a->asFloat());	
	}

	template<>	void	CRange<float32>::remove(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			remove(a->asFloat());
	}

	template<>	bool	CRange<float32>::contains(BindingMap	*bm,uint32	i)	const{
		
		Atom	*a=bm->get_code(i);
		if(a)
			return	contains(a->asFloat());
		return	true;
	}

	template<>	void	CRange<uint64>::add(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			add(Utils::GetTimestamp(a));	
	}

	template<>	void	CRange<uint64>::remove(BindingMap	*bm,uint32	i){
		
		Atom	*a=bm->get_code(i);
		if(a)
			remove(Utils::GetTimestamp(a));
	}

	template<>	bool	CRange<uint64>::contains(BindingMap	*bm,uint32	i)	const{
		
		Atom	*a=bm->get_code(i);
		if(a)
			return	contains(Utils::GetTimestamp(a));	
		return	true;
	}*/
}