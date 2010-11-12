//	r_group.cpp
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

#include	"r_group.h"
#include	"mem.h"


namespace	r_exec{

	void	RGroup::injectRGroup(View	*view){

		if(!parent)	//	this assumes that no injection of r-groups occur after the r-group becomes c-active.
			substitutions=new	UNORDERED_MAP<Code	*,Var	*>();

		((RGroup	*)view->object)->parent=this;
		((RGroup	*)view->object)->substitutions=substitutions;
	}

	void	RGroup::cov(View	*v,uint64	t){
	}

	void	RGroup::cov(uint64	t){
	}

	Var	*RGroup::get_var(Code	*value){

		Var	*var;

		substitutionsCS.enter();

		// sharp matching on values. TODO: fuzzy matching.
		UNORDERED_MAP<Code	*,Var	*>::const_iterator	it=substitutions->find(value);
		if(it!=substitutions->end())
			var=it->second;
		else{
			
			var=(Var	*)((r_exec::_Mem*)mem)->buildObject(Atom::Variable(Opcodes::Var,VAR_ARITY));
			var->code(VAR_ORG)=Atom::Nil();
			var->code(VAR_ARITY)=Atom::Float(1);	//	psln_thr.

			(*substitutions)[value]=var;

			//	inject the variable in the r-grp.
			View	*var_view=new	View();
			var_view->code(VIEW_OPCODE)=Atom::SSet(Opcodes::View,VIEW_ARITY);	//	Structured Set.
			var_view->code(VIEW_SYNC)=Atom::Boolean(true);			//	sync on front.
			var_view->code(VIEW_IJT)=Atom::IPointer(VIEW_ARITY+1);	//	iptr to ijt.
			Utils::SetTimestamp<View>(var_view,VIEW_IJT,Now());		//	ijt.
			var_view->code(VIEW_SLN)=Atom::Float(0);				//	sln.
			var_view->code(VIEW_RES)=Atom::PlusInfinity();			//	res.
			var_view->code(VIEW_HOST)=Atom::RPointer(0);			//	host.
			var_view->code(VIEW_ORG)=Atom::Nil();					//	origin.

			var_view->references[0]=this;

			var_view->set_object(var);
			((r_exec::_Mem*)mem)->inject(var_view);
		}

		substitutionsCS.leave();

		return	var;
	}
}