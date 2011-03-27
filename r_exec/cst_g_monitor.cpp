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
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include	"cst_g_monitor.h"
#include	"mem.h"
#include	"cst_controller.h"


namespace	r_exec{

	CSTGMonitor::CSTGMonitor(	CSTController	*controller,
								BindingMap		*bindings,
								Code			*super_goal,
								Code			*matched_pattern,
								uint64			deadline):GMonitor(	controller,
																	bindings,
																	NULL,
																	super_goal,
																	matched_pattern,
																	deadline,
																	deadline){
	}

	CSTGMonitor::~CSTGMonitor(){
	}

	bool	CSTGMonitor::is_alive(){

		return	controller->is_alive();
	}

	bool	CSTGMonitor::reduce(Code	*input){

		matchCS.enter();
		if(bindings->match(input->get_reference(0),goal->get_reference(0)->get_reference(0))){	//	first, check the objects pointed to by the facts.

			uint64	occurrence_time=Utils::GetTimestamp<Code>(input,FACT_TIME);	//	input is either a fact or a |fact.
			if(expected_time_low<=occurrence_time	&&	expected_time_high>=occurrence_time){

				if(input->code(0)==goal->get_reference(0)->code(0)){	//	positive match: expected a fact or |fact and got a fact or a |fact.

					((CSTController	*)controller)->produce_goals(super_goal,bindings,matched_pattern);
					controller->add_outcome(goal,true,input->code(FACT_CFD).asFloat());
				}else													//	negative match: expected a fact or |fact and got a |fact or a fact.
					controller->add_outcome(goal,false,input->code(FACT_CFD).asFloat());
				match=true;
			}
		}
		matchCS.leave();
		return	match;
	}

	void	CSTGMonitor::update(){	//	executed by a time core, upon reaching the expected time of occurrence of the target of the goal.

		matchCS.enter();
		bool	m=match;
		matchCS.leave();

		if(!m){	//	received nothing matching the target's object so far (neither positively nor negatively).

			controller->add_outcome(goal,false,1);
			controller->remove_monitor(this);
		}
	}
}