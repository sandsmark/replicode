//	p_monitor.cpp
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

#include	"p_monitor.h"
#include	"mem.h"
#include	"mdl_controller.h"


namespace	r_exec{

	PMonitor::PMonitor(	MDLController	*controller,
						BindingMap		*bindings,
						Fact			*prediction,
						bool			rate_failures):Monitor(controller,bindings,prediction),rate_failures(rate_failures){	// prediction is f0->pred->f1->obj; not simulated.

		prediction_target=prediction->get_pred()->get_target();	// f1.
		uint64	now=Now();

		bindings->reset_fwd_timings(prediction_target);

		MonitoringJob<PMonitor>	*j=new	MonitoringJob<PMonitor>(this,prediction_target->get_before()+Utils::GetTimeTolerance());
		_Mem::Get()->pushTimeJob(j);
	}

	PMonitor::~PMonitor(){
	}

	bool	PMonitor::reduce(_Fact	*input){	// input is always an actual fact.

		if(target->is_invalidated()){//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" "<<std::hex<<this<<std::dec<<" target has been invalidated\n";
		return	true;}

		if(target->get_pred()->grounds_invalidated(input)){	// input is a counter-evidence for one of the antecedents: abort.

			target->invalidate();
			return	true;
		}

		Pred	*prediction=input->get_pred();
		if(prediction){

			switch(prediction->get_target()->is_evidence(prediction_target)){
			case	MATCH_SUCCESS_POSITIVE:	// predicted confirmation, skip.
				return	false;
			case	MATCH_SUCCESS_NEGATIVE:
				if(prediction->get_target()->get_cfd()>prediction_target->get_cfd()){

					target->invalidate();	// a predicted counter evidence is stronger than the target, invalidate and abort: don't rate the model.
					return	true;
				}else
					return	false;
			case	MATCH_FAILURE:
				return	false;
			}
		}else{
			//uint32	oid=input->get_oid();
			switch(((Fact	*)input)->is_evidence(prediction_target)){
			case	MATCH_SUCCESS_POSITIVE:
				//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" "<<std::hex<<this<<std::dec<<" target: "<<prediction_target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" positive\n";
				controller->register_pred_outcome(target,true,input,input->get_cfd(),rate_failures);
				return	true;
			case	MATCH_SUCCESS_NEGATIVE:
				//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" "<<std::hex<<this<<std::dec<<" target: "<<prediction_target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" negative\n";
				if(rate_failures)
					controller->register_pred_outcome(target,false,input,input->get_cfd(),rate_failures);
				return	true;
			case	MATCH_FAILURE:
				//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" "<<std::hex<<this<<std::dec<<" target: "<<prediction_target->get_reference(0)->code(MK_VAL_VALUE).asFloat()<<" reduced: "<<input->get_oid()<<" failure\n";
				return	false;
			}
		}
	}

	void	PMonitor::update(uint64	&next_target){	// executed by a time core, upon reaching the expected time of occurrence of the target of the prediction.

		if(!target->is_invalidated()){	// received nothing matching the target's object so far (neither positively nor negatively).

			if(rate_failures)
				controller->register_pred_outcome(target,false,NULL,1,rate_failures);
		}
		controller->remove_monitor(this);
		next_target=0;
	}
}