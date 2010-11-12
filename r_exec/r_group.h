//	r_group.h
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

#ifndef	r_group_h
#define	r_group_h

#include	"group.h"
#include	"var.h"


namespace	r_exec{

	//	The content of r-groups is abstracted unitl the r-group becomes c-active.
	//	The latter is the signal for switching abstractors to binders (see InputLessPGMOverlay::injectproductions(), case of _subst).
	class	r_exec_dll	RGroup:
	public	Group{
	private:
		RGroup	*parent;

		CriticalSection					substitutionsCS;
		UNORDERED_MAP<Code	*,Var	*>	*substitutions;	// temporary structure used while performing abstractions: not written in images.
														// all children share the same substitutions as their one common ancestor.
		void	injectRGroup(View	*view);
		void	cov(View	*view,uint64	t);	//	does nothing since there is no cov for r-groups.
	public:
		RGroup(r_code::Mem	*m=NULL);
		RGroup(r_code::SysObject	*source,r_code::Mem	*m);
		~RGroup();

		uint16	get_out_grp_count();
		Group	*get_out_grp(uint16	i);	// i starts at 1.

		Var		*get_var(Code	*value);	// returns a variable object from the existing substitutions holding the same value; if none, build a new variable object.

		//	These functions are called by the rMem.
		void	cov(uint64	t);	//	does nothing since there is no cov for r-groups.
	};
}


#include	"r_group.inline.cpp"


#endif