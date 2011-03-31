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
								Code			*goal,				//	(mk.goal (fact (icst controller->getObject() [args] ...)...)...)
								Code			*super_goal,		//	(fact (mk.goal ...) ...).
								Code			*matched_pattern,	//	the pattern the matching of which triggered the need for ensuring requirements; never NULL.
								uint64			deadline):GMonitor(	controller,
																	bindings,
																	goal,
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

	bool	CSTGMonitor::reduce(Code	*input){	//	catches (icst controller->getObject() [args] ...).

		Code	*_input=input;
		Code	*_input_fact_object=_input->get_reference(0);
		Code	*goal_icst=goal->get_reference(0)->get_reference(0);
		if(	_input_fact_object->code(0).asOpcode()==Opcodes::MkPred){	//	we may have got fact or |fact -> pred -> fact -> icst referring to this cst.

			Code	*pred_fact_object=_input_fact_object->get_reference(0)->get_reference(0);
			if(	pred_fact_object->code(0).asOpcode()==goal_icst->code(0).asOpcode()	&&
				pred_fact_object->get_reference(0)==controller->getObject())
			_input_fact_object=pred_fact_object;
		}

		matchCS.enter();
		if(bindings->match(_input_fact_object,goal_icst)){	//	first, check the objects pointed to by the facts.

			uint64	occurrence_time=Utils::GetTimestamp<Code>(input,FACT_TIME);	//	input is either a fact or a |fact.
			if(expected_time_low<=occurrence_time	&&	expected_time_high>=occurrence_time){

				if(_input->code(0)==goal->get_reference(0)->code(0)){	//	positive match: expected a fact or |fact and got a fact or a |fact.

					((CSTController	*)controller)->produce_goals(super_goal,bindings,matched_pattern);
					controller->add_outcome(goal,true,_input->code(FACT_CFD).asFloat());
				}else													//	negative match: expected a fact or |fact and got a |fact or a fact.
					controller->add_outcome(goal,false,_input->code(FACT_CFD).asFloat());
				match=true;
			}
		}
		matchCS.leave();
		return	match;
	}
}