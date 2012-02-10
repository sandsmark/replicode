//	cst_g_monitor.cpp
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
//	EXPRESS OR IMPLIED WARNTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// DOOMED FOR REMOVAL.

/*
#include	"cst_g_monitor.h"
#include	"mem.h"
#include	"cst_controller.h"


namespace	r_exec{

	CSTGMonitor::CSTGMonitor(	HLPController	*controller,
								BindingMap		*bindings,
								Code			*goal,			// f->g->f->(picst controller->getObject() [args])
								Code			*super_goal,	// f->g->f->(icst controller->getObject() [args])
								uint64			before,
								uint64			after):Monitor(controller,bindings,goal,before,after){

		this->super_goal=super_goal;
	}

	CSTGMonitor::~CSTGMonitor(){
	}

	bool	CSTGMonitor::reduce(Code	*input){	// catches fact (or |fact) -> (picst controller->getObject() [args]).

		Code	*f_picst=target;	// deviation from the standard where f_g is f->g->f->picst; here we only need f->picst: passed directly to the constructor; the goal is omitted.
		matchCS.enter();
		if(bindings->match(input->get_reference(0),f_picst->get_reference(0))){	// first, check the objects pointed by the facts.

			uint64	occurrence_time=Utils::GetTimestamp<Code>(input,FACT_AFTER);	// input is either a fact or a |fact; after==before.
			if(check_time(occurrence_time)){

				if(input->code(0)==f_picst->code(0)){	// positive match: expected a fact and got a fact.

					((CSTController	*)controller)->produce_cmd_goals(bindings,super_goal);
					match=1;
				}
			}
		}
		matchCS.leave();
		return	match;
	}

	void	CSTGMonitor::update(){	// executed by a time core, upon reaching the expected time of occurrence of the goal target.

		matchCS.enter();
		bool	m=match;
		matchCS.leave();

		if(!m)	// received nothing matching the f->picst so far.
			controller->remove_monitor(this);
	}
}

*/