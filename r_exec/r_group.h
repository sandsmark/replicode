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


namespace	r_exec{

	class	FwdController;
	class	GSMonitor;

	//	R-groups are the constituents of models. A model hold references to one r-group (the head), which in turn, holds references to its children.
	//	R-groups do not hold views on regular groups.
	//	Children shall be projected onto their parents first and then onto an abstraction group. Then, after abstraction is complete, a model can reference the head r-group.
	//	Controllers are built as follows:
	//		when the model is injected, the head's controller is built;
	//		when an r-group fires, it builds the controllers for its children.
	//	As the content of r-groups is being abstracted, binding programs (a.k. binders) are automatically generated and injected in the r-group.
	//	R-groups shall be created non c-active so that the binders are prevented from catching the initial content (not abstracted yet).
	class	r_exec_dll	RGroup:
	public	Group{
	private:
		RGroup			*parent;
		FwdController	*controller;

		CriticalSection														*substitutionsCS;
		UNORDERED_MAP<Code	*,std::pair<Code	*,std::list<RGroup	*> > >	*substitutions;	// temporary structure used while performing abstractions: not written in images.
																							// all children share the same substitutions as their one common ancestor.
		Code	*fwd_model;

		void	injectRGroup(View	*view);
	public:
		static	Code	*BindObject(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings);
		static	Code	*BindReference(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings);
		static	bool	NeedsBinding(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings);

		RGroup(r_code::Mem	*m=NULL);
		RGroup(r_code::SysObject	*source,r_code::Mem	*m);
		~RGroup();

		Code	*get_var(Code	*value);	// returns a variable object from the existing substitutions holding the same value.
											// if none, build a new variable object; in any case, inject the variable object in the group if not already there.

		Code	*get_fwd_model()	const;
		void	set_fwd_model(Code	*mdl);

		RGroup			*get_parent()	const;
		FwdController	*get_controller()	const;
		void			set_controller(FwdController	*c);

		void	instantiate_goals(std::vector<Code	*>				*initial_goals,
								  GSMonitor							*initial_monitor,
								  bool								sim,
								  bool								asmp,
								  Code								*inv_model,
								  UNORDERED_MAP<Code	*,P<Code> >	*bindings);
	};
}


#include	"r_group.inline.cpp"


#endif