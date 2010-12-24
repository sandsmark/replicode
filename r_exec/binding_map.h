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


namespace	r_exec{

	class	RGroup;

	//	Holds three kinds of bindings (i.e. pairs variable|value).
	class	BindingMap{
	public:
		UNORDERED_MAP<Code	*,P<Code> >			objects;	//	objects of class var, values are system objects.
		UNORDERED_MAP<Atom,Atom>				atoms;		//	numerical variables, values are atoms (Atom::Float).
		UNORDERED_MAP<Atom,std::vector<Atom> >	structures;	//	structural variables, values are structures embedded in code.

		uint16	unbound_var_count;

		BindingMap();

		void	clear();

		void	init(BindingMap	*source);
		void	init(RGroup	*source);

		void	add(BindingMap	*source);	//	add bindings from source to this: if this has a variable v0 and source also, then bind v0 with source[v0]; if source has a variable v1 not in this, add v1.

		void	bind_atom(const	Atom	&var,const	Atom	&val);
		void	bind_structure(const	Atom	&var,const	std::vector<Atom>	&val);
		void	bind_object(Code	*var,Code	*val);
		void	unbind_atom(const	Atom	&var);
		void	unbind_structure(const	Atom	&var);
		void	unbind_object(Code	*var);

		Code	*bind_object(Code	*original);
		bool	needs_binding(Code	*original);
	};
}


#endif