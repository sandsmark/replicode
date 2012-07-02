//	domain.h
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

#ifndef	domain_h
#define	domain_h

#include	"binding_map.h"


namespace	r_exec{
/*
	class	Range:
	public	_Object{
	public:
		virtual	void	add(BindingMap	*bm,uint32	i)=0;
		virtual	void	remove(BindingMap	*bm,uint32	i)=0;
		virtual	bool	contains(BindingMap	*bm,uint32	i)	const=0;
	};

	// Discrete range.
	// Use for bool, std::string, P<Code>.
	template<typename	T>	class	DRange:
	public	Range{
	private:
		UNORDERED_SET<T>	values;

		void	add(T	value){
			values.insert(value);
		}
		void	remove(T	value){
			values.erase(value);
		}
		bool	contains(T	value)	const{
			return	values.find(value)!=values.end();
		}
	public:
		void	add(BindingMap	*bm,uint32	i){}
		void	remove(BindingMap	*bm,uint32	i){}
		bool	contains(BindingMap	*bm,uint32	i)	const{	return	false;	}
	};

	// Continuous range.
	// Use for numerical intervals (float32 or uint64).
	template<typename	T>	class	CRange:
	public	Range{
	private:
		std::list<std::pair<T,T> >	values;

		void	add(T	value){

			std::list<std::pair<T,T> >::const_iterator	i;
			for(i=values.begin();i!=values.end();++i){

				
				
			}
		}
		void	remove(T	value){

			std::list<std::pair<T,T> >::const_iterator	i;
			for(i=values.begin();i!=values.end();++i){


			}
		}
		bool	contains(T	value)	const{

			std::list<std::pair<T,T> >::const_iterator	i;
			for(i=values.begin();i!=values.end();++i){

				if((*i).first<=value	&&	(*i).second>=value)
					return	true;
			}

			return	false;
		}
	public:
		void	add(BindingMap	*bm,uint32	i){}
		void	remove(BindingMap	*bm,uint32	i){}
		bool	contains(BindingMap	*bm,uint32	i)	const{	return	false;	}
	};

	// Last hidden reference of a model.
	class	Domain:
	public	Code{
	private:
		std::vector<P<Range> >	ranges;	// same indices as in a BindingMap.
	public:
		void	add(BindingMap	*bm);
		void	remove(BindingMap	*bm);
		bool	contains(BindingMap	*bm)	const;
	};*/
}


#endif
