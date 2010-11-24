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

		if(!parent	&&	!substitutions){	//	init the head; this assumes that children are injected from left to right.

			substitutionsCS=new	CriticalSection();
			substitutions=new	UNORDERED_MAP<Code	*,std::pair<Code	*,std::list<RGroup	*> > >();
		}

		((RGroup	*)view->object)->parent=this;
		((RGroup	*)view->object)->substitutions=substitutions;
		((RGroup	*)view->object)->substitutionsCS=substitutionsCS;

		view->controller=new	FwdController((r_exec::_Mem	*)mem,view);
	}

	Code	*RGroup::get_var(Code	*value){

		Code	*var;
		bool	inject=false;

		substitutionsCS->enter();

		// sharp matching on values. TODO: fuzzy matching.
		UNORDERED_MAP<Code	*,std::pair<Code	*,std::list<RGroup	*> > >::iterator	s=substitutions->find(value);
		if(s!=substitutions->end()){	//	variable already exists for the value.

			var=s->second.first;

			std::list<RGroup	*>::const_iterator	g;
			for(g=s->second.second.begin();g!=s->second.second.end();++g)
				if((*g)==this)
					break;

			if(g==s->second.second.end()){	//	variable not injected (yet) in the group.

				s->second.second.push_back(this);
				inject=true;
			}
		}else{	//	no variable exists yet for the value.
			
			var=((r_exec::_Mem*)mem)->buildObject(Atom::Object(Opcodes::Var,VAR_ARITY));
			var->code(VAR_ARITY)=Atom::Float(1);	//	psln_thr.

			std::pair<Code	*,std::list<RGroup	*> >	entry;
			entry.first=var;
			entry.second.push_back(this);
			(*substitutions)[value]=entry;

			inject=true;
		}

		substitutionsCS->leave();

		if(inject){	//	inject the variable in the group.

			View	*var_view=new	View(true,Now(),0,-1,this,NULL,var);
			((r_exec::_Mem*)mem)->inject(var_view);
		}

		return	var;
	}

	void	RGroup::instantiate_goals(UNORDERED_MAP<Code	*,P<Code> >	*bindings){

		if(parent)
			parent->instantiate_goals(bindings);
	}
}