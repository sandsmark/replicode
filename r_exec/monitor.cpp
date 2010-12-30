//	rgrp_overlay.cpp
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

#include	"monitor.h"
#include	"rgrp_controller.h"
#include	"pgm_controller.h"
#include	"fact.h"
#include	"mem.h"


namespace	r_exec{
	
	PMonitor::PMonitor(FwdController	*c,Code	*target,uint64	expected_time,uint64	time_tolerance):_Object(),controller(c),target(target),success(false){

		expected_time_high=expected_time+time_tolerance;
		expected_time_low=expected_time-time_tolerance;
	}

	bool	PMonitor::is_alive(){

		return	controller->is_alive();
	}

	void	PMonitor::take_input(r_exec::View	*input){	//	executed (a) in the same thread as for FwdController::take_input() or, (b) in the same thread as FwdController::inject_productions().

		if(Any::Equal(input->object->get_reference(0),target->get_reference(0)->get_reference(0))){	//	first, check the objects pointed to by the facts.

			uint64	occurrence_time=Utils::GetTimestamp<Code>(input->object,FACT_TIME);	//	input->object is either a fact or a |fact.
			if(expected_time_low<=occurrence_time	&&	expected_time_high>=occurrence_time){

				if(input->object->code(0)==target->get_reference(0)->code(0))	//	positive match: expected a fact or |fact and got a fact or a |fact.
					controller->add_outcome(this,true,input->object->code(FACT_CFD).asFloat());
				else															//	negative match: expected a fact or |fact and got a |fact or a fact.
					controller->add_outcome(this,false,input->object->code(FACT_CFD).asFloat());

				successCS.enter();
				success=true;
				successCS.leave();
			}
		}
	}

	void	PMonitor::update(){	//	executed by a time core, upon reaching the expected time of occurrence of the target of a pred/asmp.

		bool	s;
		successCS.enter();
		s=success;
		successCS.leave();

		if(!s){	//	received nothing matching the target's object so far (neither positively nor negatively).

			Model	*fwd_model=(Model	*)controller->get_rgrp()->get_fwd_model();
			fwd_model->inject_opposite(target->get_reference(0));

			controller->add_outcome(this,false,1);
			controller->remove_monitor(this);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//	N.B.: when a super-goal changes saliency, use the sln propagation to replicate to sub-goals; but not the other way around. Needs sub-goals though.
	//	N.B.: shall a goal become non salient, this has no effect on its monitors, if any.

	GSMonitor::GSMonitor(Model			*inv_model,
						 FwdController	*c,
						 GSMonitor		*parent,
						 RGroup			*group,
						 BindingMap		*bindings,
						 uint8			reduction_mode):BindingOverlay(c,group,bindings,reduction_mode),
														inv_model(inv_model),
														parent(parent),
														tsc(0),
														master(false),
														family(NULL){
	}

	GSMonitor::GSMonitor(GSMonitor	*original):BindingOverlay(original),master(false){

		parent=original->parent;
		inv_model=original->inv_model;
		goals=original->goals;
		tsc=original->tsc;
		family=original->family;
	}

	GSMonitor::~GSMonitor(){

		if(master)
			delete	family;
	}

	void	GSMonitor::reduce(r_exec::View	*input){	//	called by reduction cores; never called on tail monitors.

		if(!goals.size())	//	not instantiated yet.
			return;

		Code	*input_object=input->object;
		if(input_object->code(0).asOpcode()!=Opcodes::Fact	&&	input_object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
			return;

		reductionCS.enter();
		if(alive){

			last_bound_variable_objects.clear();
			last_bound_code_variables.clear();
			discard_bindings=false;
			no_binding_needed=false;

			std::list<P<View> >::const_iterator	b;
			for(b=binders.begin();b!=binders.end();++b){	//	binders called sequentially (at most one can bind an input).
				
				((PGMController	*)(*b)->controller)->take_input(input,this);	//	language constraint: abstraction programs shall not define constraints on facts' timings.
				
				if(discard_bindings)
					break;
				if(no_binding_needed	||	last_bound_variable_objects.size()	||	last_bound_code_variables.size()){	//	at least one variable was bound.
					
					if(!bindings.unbound_var_count){

						update_reduction_mode(input_object);

						if(parent==NULL){	//	tail.

							for(uint32	i=0;i<goals.size();++i)
								inject_success(goals[i],1);
						}else
							parent->instantiate(&bindings,reduction_mode);
						
						kill_family();
					}else{

						GSMonitor	*offspring=new	GSMonitor(this);
						((FwdController	*)controller)->add_monitor(offspring);
						family->push_back(offspring);
						_Mem::Get()->pushTimeJob(new	MonitoringJob<GSMonitor>(offspring,Now()+offspring->get_tsc()));

						update_reduction_mode(input_object);

						binders.erase(b);
					}
					break;
				}
			}
		}
		reductionCS.leave();
	}

	void	GSMonitor::update(){	//	occurs upon reaching the tsc; if still alive, that means that no success nor failure has been registered so far for some of the goals.

		if(!is_alive())
			return;

		for(uint32	i=0;i<goals.size();++i){

			goals[i]->kill();
			inject_failure(goals[i],1);
		}

		propagate_failure();

		for(uint32	i=0;i<goals.size();++i)
			inv_model->inject_opposite(goals[i]->get_reference(0));
	}

	void	GSMonitor::propagate_failure(){

		kill_family();
		if(parent!=NULL)
			parent->propagate_failure();
	}

	void	GSMonitor::inject_success(Code	*g,float32	confidence){

		uint64	now=Now();

		inv_model->register_outcome(true,confidence);
		Code	*marker=factory::Object::MkSuccess(g,inv_model->get_success_rate(),inv_model->get_failure_rate(),1);
		Code	*fact=factory::Object::Fact(marker,now,confidence,1);

		inject_outcome(fact,marker,now);
	}

	void	GSMonitor::inject_failure(Code	*g,float32	confidence){

		uint64	now=Now();

		inv_model->register_outcome(false,confidence);
		Code	*marker=factory::Object::MkSuccess(g,inv_model->get_success_rate(),inv_model->get_failure_rate(),1);
		Code	*fact=factory::Object::AntiFact(marker,now,confidence,1);

		inject_outcome(fact,marker,now);
	}

	void	GSMonitor::inject_outcome(Code	*fact,Code	*marker,uint64	t){

		Code	*mk_sim=get_mk_sim(marker->get_reference(0));
		Code	*mk_asmp=get_mk_asmp(marker->get_reference(0));

		RGroup	*rgrp=get_rgrp();
		uint16	ntf_group_set_index=inv_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=inv_model->code(ntf_group_set_index).getAtomCount();
		for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

			Group				*ntf_group=(Group	*)inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
			NotificationView	*ntf_view=new	NotificationView(get_rgrp(),ntf_group,marker);
			_Mem::Get()->injectNotificationNow(ntf_view,true);

			View	*view=new	View(true,t,1,_Mem::Get()->get_ntf_mk_res(),ntf_group,rgrp,fact);
			_Mem::Get()->inject(view);

			if(mk_sim){

				view=new	View(true,t,1,_Mem::Get()->get_sim_res(),ntf_group,rgrp,mk_sim);
				_Mem::Get()->inject(view);
			}

			if(mk_asmp){

				view=new	View(true,t,1,_Mem::Get()->get_asmp_res(),ntf_group,rgrp,mk_asmp);
				_Mem::Get()->inject(view);
			}
		}
	}

	void	GSMonitor::instantiate(){

		family=new	std::vector<GSMonitor	*>();
		master=true;
		family->push_back(this);

		uint64	now=Now();

		if(parent!=NULL){

			RGroup	*rgrp=get_rgrp();

			//	Build new objects and their respective new goal markers.
			UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
			for(v=rgrp->other_views.begin();v!=rgrp->other_views.end();++v){	//	we are only interested in objects and markers.

				Code	*original=v->second->object;
				Code	*bound_object=bindings.bind_object(original);
				Code	*goal=factory::Object::MkGoal(bound_object,inv_model,1);
				goals.push_back(goal);
				uint64	dt=Utils::GetTimestamp<Code>(goal->get_reference(0),FACT_TIME)-now;
				if(tsc<dt)
					tsc=dt;
			}

			uint16	out_group_set_index=inv_model->code(MD_OUT_GRPS).asIndex();
			uint16	out_group_count=inv_model->code(out_group_set_index).getAtomCount();

			for(uint32	i=0;i<goals.size();++i){	//	create a fact (confidence=1) pointing to each goal and inject it into the model's output groups.

				Code	*fact=factory::Object::Fact(goals[i],now,1,1);
				for(uint16	i=1;i<=out_group_count;++i){

					Code	*out_group=inv_model->get_reference(inv_model->code(out_group_set_index+i).asIndex());

					View	*view=new	View(true,now,1,1,out_group,rgrp,fact);
					_Mem::Get()->inject(view);

					Code	*mk_sim=get_mk_sim(fact);
					if(mk_sim){

						view=new	View(true,now,1,_Mem::Get()->get_sim_res(),out_group,rgrp,mk_sim);
						_Mem::Get()->inject(view);
					}
					
					Code	*mk_asmp=get_mk_asmp(fact);
					if(mk_asmp){

						view=new	View(true,now,1,_Mem::Get()->get_asmp_res(),out_group,rgrp,mk_asmp);
						_Mem::Get()->inject(view);
					}
				}
			}
		}

		_Mem::Get()->pushTimeJob(new	MonitoringJob<GSMonitor>(this,now+get_tsc()));
	}

	void	GSMonitor::instantiate(BindingMap	*bindings,uint8	reduction_mode){

		this->bindings.add(bindings);
		this->reduction_mode=reduction_mode;
		instantiate();
	}

	Code	*GSMonitor::get_mk_sim(Code	*object)	const{

		if(reduction_mode	&	RDX_MODE_SIMULATION)
			return	factory::Object::MkSim(object,inv_model,1);
		else
			return	NULL;
	}

	Code	*GSMonitor::get_mk_asmp(Code	*object)	const{

		if(reduction_mode	&	RDX_MODE_ASSUMPTION)
			return	factory::Object::MkAsmp(object,inv_model,1,1);	//	TODO: put the right value (from where?) for the confidence member.
		else
			return	NULL;
	}

	inline	RGroup	*GSMonitor::get_rgrp()	const{

		return	((FwdController	*)controller)->get_rgrp();
	}

	inline	uint64	GSMonitor::get_tsc()	const{

		return	tsc;
	}

	void	GSMonitor::set_goals(std::vector<Code	*>	&goals){

		uint64	now=Now();
		for(uint32	i=0;i<goals.size();++i){
			
			this->goals.push_back(goals[i]);
			uint64	dt=Utils::GetTimestamp<Code>(goals[i]->get_reference(0),FACT_TIME)-now;
			if(tsc<dt)
				tsc=dt;
		}
	}
}