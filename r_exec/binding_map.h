//	binding_map.h
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

#ifndef	binding_map_h
#define	binding_map_h

#include	"../r_code/object.h"
#include	"dll.h"


using	namespace	r_code;

namespace	r_exec{

	//	Holds three kinds of bindings (i.e. pairs variable|value).
	class	r_exec_dll	BindingMap:
	public	_Object{
	private:
		UNORDERED_MAP<Code	*,P<Code> >			objects;	//	objects of class var, values are system objects.
		UNORDERED_MAP<Atom,Atom>				atoms;		//	numerical variables, values are atoms (Atom::Float).
		UNORDERED_MAP<Atom,std::vector<Atom> >	structures;	//	structural variables, values are structures embedded in code.

		bool	match_atom(Atom	o,Atom	p);
		bool	match_timestamp(Atom	*o,Atom	*p);
		bool	match_structure(Atom	*o,Atom	*p);

		bool	needs_binding(Code	*original)	const;

		bool	bind_float_variable(Atom	val,Atom	var);
		bool	bind_boolean_variable(Atom	val,Atom	var);
		bool	bind_structural_variable(Atom	*val,Atom	*var);
		bool	bind_object_variable(Code	*val,Code	*var);
	public:
		BindingMap();
		BindingMap(const	BindingMap	*source);

		void	init(const	BindingMap	*source);
		void	init(Code	*source);
		void	clear();

		Code	*bind_object(Code	*original)	const;

		Atom	get_atomic_variable(const	Code	*object,uint16	index);
		Atom	get_structural_variable(const	Code	*object,uint16	index);
		Code	*get_variable_object(Code	*object);
		void	set_variable_object(Code	*object);

		bool	match(Code	*object,Code	*pattern);

		void	copy(Code	*object,uint16	index)	const;
		void	load(const	Code	*object);	//	icst or imdl.
	};
}


#endif