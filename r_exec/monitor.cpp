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

	bool	PMonitor::take_input(r_exec::View	*input){	//	executed in the same thread as for FwdController::take_input().

		if(input->object->get_sim()	||	input->object->get_asmp()	||	input->object->get_pred())	//	we are only interested in actual facts: discard pred, asmp and hyp/sim.
			return	false;

		if(Any::Equal(input->object->get_reference(0),target->get_reference(0)->get_reference(0))){	//	first, check the objects pointed to by the facts.

			uint64	occurrence_time=Utils::GetTimestamp<Code>(input->object,FACT_TIME);	//	input->object is either a fact or a |fact.
			if(expected_time_low<=occurrence_time	&&	expected_time_high>=occurrence_time){

				if(input->object->code(0)==target->get_reference(0)->code(0)){	//	positive match: expected a fact or |fact and got a fact or a |fact.

					controller->add_outcome(this,true,input->object->code(FACT_CFD).asFloat());
					success=true;
					return	true;	//	the caller will remove the monitor from its list (will not take any more inputs).
				}else{															//	negative match: expected a fact or |fact and got a |fact or a fact.

					controller->add_outcome(this,false,input->object->code(FACT_CFD).asFloat());
					success=true;
					return	false;	//	stays in the controller's monitors list (will take more inputs).
				}
			}
		}
		return	false;
	}

	void	PMonitor::update(){

		if(!success){	//	received nothing matching the target's object so far (neither positively nor negatively).

			uint64	now=Now();
			Code	*f;	//	generate the opposite of the target's fact (negative findings).
			if(target->get_reference(0)->code(0).asOpcode()==Opcodes::Fact)
				f=factory::Object::AntiFact(target->get_reference(0)->get_reference(0),now,1,1);
			else
				f=factory::Object::Fact(target->get_reference(0)->get_reference(0),now,1,1);

			Code	*fwd_model=controller->get_rgrp()->get_fwd_model();
			uint16	out_group_set_index=fwd_model->code(MD_OUT_GRPS).asIndex();
			uint16	out_group_count=fwd_model->code(out_group_set_index).getAtomCount();

			for(uint16	i=1;i<=out_group_count;++i){	//	inject the negative findings in the ouptut groups.

				Code	*out_group=fwd_model->get_reference(fwd_model->code(out_group_set_index+i).asIndex());
				View	*view=new	View(true,now,1,1,out_group,NULL,f);	//	inject in stdin: TODO: allow specifying the destination group.
				_Mem::Get()->inject(view);
			}

			controller->add_outcome(this,false,1);
			controller->remove_monitor(this);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void	GSMonitor::KillSubGoals(Code	*mk_goal){	//	recursively kill all the target's sub-goals and their related monitors.

		mk_goal->acq_markers();
		std::list<Code	*>::const_iterator	mk;
		for(mk=mk_goal->markers.begin();mk!=mk_goal->markers.end();++mk){

			if((*mk)->code(0).asOpcode()==Opcodes::MkSubGoal){

				Code	*sub_goal=(*mk)->get_reference(1);
				if(mk_goal==sub_goal)
					continue;
				KillSubGoals(sub_goal);
				sub_goal->kill();
			}
		}
		mk_goal->rel_markers();
	}

	void	GSMonitor::KillSuperGoals(Code	*mk_goal){	//	recursively kill all the target's super-goals and their related monitors.

		mk_goal->acq_markers();
		std::list<Code	*>::const_iterator	mk;
		for(mk=mk_goal->markers.begin();mk!=mk_goal->markers.end();++mk){

			if((*mk)->code(0).asOpcode()==Opcodes::MkSubGoal){

				Code	*super_goal=(*mk)->get_reference(0);
				if(mk_goal==super_goal)
					continue;
				KillSuperGoals(super_goal);
				super_goal->kill();
			}
		}
		mk_goal->rel_markers();
	}

	void	GSMonitor::KillRelatedGoals(Code	*mk_goal){	//	recursively kill all the target's sub- and super-goals and their related monitors.

		mk_goal->acq_markers();
		std::list<Code	*>::const_iterator	mk;
		for(mk=mk_goal->markers.begin();mk!=mk_goal->markers.end();++mk){

			if((*mk)->code(0).asOpcode()==Opcodes::MkSubGoal){

				Code	*sub_goal=(*mk)->get_reference(1);
				Code	*super_goal=(*mk)->get_reference(0);
				if(mk_goal==sub_goal){

					KillSuperGoals(super_goal);
					super_goal->kill();
				}else{

					KillSubGoals(sub_goal);
					sub_goal->kill();
				}
			}
		}
		mk_goal->rel_markers();
	}

	GSMonitor::GSMonitor(Model			*inv_model,
						 FwdController	*c,
						 GSMonitor		*parent,
						 RGroup			*group,
						 BindingMap		*bindings,
						 uint8			reduction_mode):BindingOverlay(c,group,bindings,reduction_mode),
														inv_model(inv_model),
														parent(parent){
	}

	GSMonitor::GSMonitor(GSMonitor	*original):BindingOverlay(original){

		parent=original->parent;
		inv_model=original->inv_model;
		goals=original->goals;
	}

	GSMonitor::~GSMonitor(){
	}

	bool	GSMonitor::take_input(r_exec::View	*input){	//	when returns true, the monitor is removed from its controller.

		if(!is_alive())
			return	true;

		Code	*input_object=input->object;
		if(input_object->get_goal())	//	discard object marked as goals; this does not mean that goals are discarded when they are the input.
			return	false;

		bool	r=false;

		reductionCS.enter();
		last_bound_variable_objects.clear();
		last_bound_code_variables.clear();
		discard_bindings=false;

		std::list<P<View> >::const_iterator	b;
		for(b=binders.begin();b!=binders.end();++b){	//	binders called sequentially (at most one can bind an input).

			((PGMController	*)(*b)->controller)->take_input(input,this);	//	language constraint: abstraction programs shall not define constraints on facts' timings.
			if(discard_bindings)
				break;
			if(last_bound_variable_objects.size()	||	last_bound_code_variables.size()){	//	at least one variable was bound.

				GSMonitor	*offspring=new	GSMonitor(this);
				((FwdController	*)controller)->add_monitor(offspring);

				if(input_object->get_sim())
					reduction_mode|=RDX_MODE_SIMULATION;
				if(input_object->get_asmp())
					reduction_mode|=RDX_MODE_ASSUMPTION;

				if(!bindings.unbound_var_count){

					controller->remove(this);
					//	register success.
					r=true;
				}else
					binders.erase(b);
				break;
			}
		}
		reductionCS.leave();

		return	r;
	}

	void	GSMonitor::update(){	//	occurs on tsc.

		if(!bindings.unbound_var_count){	//	success.


		}else{


		}
	}

	void	GSMonitor::add_outcome(Outcome	outcome,float32	confidence){	//	executed in the thread of a time core (from GMonitor::update()).

		if(!is_alive())
			return;

		uint64	now=Now();

		switch(outcome){
		case	SUCCESS:{
/*
			//	Shall the goal target be marked with a hyp/sim, an asmp or a pred, the notification shall also be marked by similar markers.

			//	Notify.
			inv_model->register_outcome(true,confidence);
			Code	*marker=factory::Object::MkSuccess(monitor->target,inv_model->get_success_rate(),inv_model->get_failure_rate(),1);
			Code	*fact=factory::Object::Fact(marker,now,confidence,1);

			uint16	ntf_group_set_index=inv_model->code(MD_NTF_GRPS).asIndex();
			uint16	ntf_group_count=inv_model->code(ntf_group_set_index).getAtomCount();
			for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

				Group				*ntf_group=(Group	*)inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
				NotificationView	*ntf_view=new	NotificationView(get_rgrp(),ntf_group,marker);
				_Mem::Get()->injectNotificationNow(ntf_view,true);

				View	*fact_view=new	View(true,now,1,_Mem::Get()->get_ntf_mk_res(),ntf_group,get_rgrp(),fact);
				_Mem::Get()->inject(fact_view);
			}

			KillSubGoals(monitor->target);

			uint32	monitor_count;
			reductionCS.enter();
			monitor_count=monitors.size();
			reductionCS.leave();

			if(monitor_count==1){	//	no monitor left but the caller, i.e. all monitors succeeded: activate the supergoals of the goal (m->target).

				uint16	out_group_set_index=inv_model->code(MD_OUT_GRPS).asIndex();
				uint16	out_group_count=inv_model->code(out_group_set_index).getAtomCount();

				monitor->target->acq_markers();
				std::list<Code	*>::const_iterator	mk;
				for(mk=monitor->target->markers.begin();mk!=monitor->target->markers.end();++mk){	//	the monitor's target is marked by sub-goal markers pointing to the super-goals.

					if((*mk)->code(0).asOpcode()==Opcodes::MkSubGoal){	//	inject the super-goals and target objects, plus associated sim/asmp if any.

						Code	*super_goal=(*mk)->get_reference(0);
						if(m->target==super_goal)
							continue;
						Code	*goal_target=super_goal->get_reference(0);
						Code	*mk_sim=get_mk_sim(goal_target);
						Code	*mk_asmp=get_mk_asmp(goal_target);

						for(uint16	i=1;i<=out_group_count;++i){	//	inject the super-goal target in the model's output groups.

							Code	*out_group=inv_model->get_reference(inv_model->code(out_group_set_index+i).asIndex());
							View	*view=new	View(true,now,1,1,out_group,get_rgrp(),goal_target);
							_Mem::Get()->inject(view);
						}

						for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups: super-goal and sim/asmp.

							Group	*ntf_group=(Group	*)inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
							View	*view;

							view=new	View(true,now,1,_Mem::Get()->get_goal_res(),ntf_group,get_rgrp(),super_goal);
							_Mem::Get()->inject(view);

							if(mk_sim){

								view=new	View(true,now,1,_Mem::Get()->get_sim_res(),ntf_group,get_rgrp(),mk_sim);
								_Mem::Get()->inject(view);
							}

							if(mk_asmp){

								view=new	View(true,now,1,_Mem::Get()->get_asmp_res(),ntf_group,get_rgrp(),mk_asmp);
								_Mem::Get()->inject(view);
							}
						}

						if(parent!=NULL){	//	add a GMonitor to the parent GSMonitor.

							uint64		expected_time=Utils::GetTimestamp<Code>(goal_target,FACT_TIME);
							float32		multiplier=goal_target->code(goal_target->code(FACT_TIME).asIndex()).getMultiplier();	//	tolerance is stored in the timestamp atom of the bound object (here: goal_target).
							uint64		time_tolerance=abs((float32)((int64)(expected_time-now)))*multiplier;
							GMonitor	*m=new	GMonitor(parent,super_goal,expected_time,time_tolerance);

							parent->add_monitor(m);
							_Mem::Get()->pushTimeJob(new	MonitoringJob<GMonitor>(m,expected_time));
						}
					}
				}
				monitor->target->rel_markers();

				kill();
				return;
			}*/
			return;
		}case	FAILURE:{
/*
			//	Notify.
			inv_model->register_outcome(false,confidence);
			Code	*marker=factory::Object::MkSuccess(monitor->target,inv_model->get_success_rate(),inv_model->get_failure_rate(),1);
			Code	*fact=factory::Object::AntiFact(marker,now,confidence,1);

			uint16	ntf_group_set_index=inv_model->code(MD_NTF_GRPS).asIndex();
			uint16	ntf_group_count=inv_model->code(ntf_group_set_index).getAtomCount();
			for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

				Group				*ntf_group=(Group	*)inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
				NotificationView	*ntf_view=new	NotificationView(get_rgrp(),ntf_group,marker);
				_Mem::Get()->injectNotificationNow(ntf_view,true);

				View	*fact_view=new	View(true,now,1,_Mem::Get()->get_ntf_mk_res(),ntf_group,get_rgrp(),fact);
				_Mem::Get()->inject(fact_view);
			}

			KillRelatedGoals(monitor->target);
			kill();*/
			return;
		}case	INVALIDATED:{	//	all of the target (mk.goal)'s views are dead, i.e. the goal is no longer pursued.
/*
			KillRelatedGoals(monitor->target);
			kill();*/
			return;
		}
		}

		//	N.B.: when a super-goal changes saliency, use the sln propagation to replicate to sub-goals; but not the other way around.
		//	N.B.: shall a goal become non salient, this has no effect on its monitors, if any.
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
/*
	GMonitor::GMonitor(GSMonitor	*p,Code	*target,uint64	expected_time,uint64	time_tolerance):_Object(),parent(p),target(target),success(false){

		expected_time_high=expected_time+time_tolerance;
		expected_time_low=expected_time-time_tolerance;
	}

	GMonitor::~GMonitor(){
	}

	bool	GMonitor::is_alive(){

		return	parent->is_alive();
	}

	bool	GMonitor::take_input(r_exec::View	*input){	//	executed by reduction cores.

		if(target->is_invalidated()){

			parent->add_outcome(this,GSMonitor::INVALIDATED,1);
			return	true;
		}

		//	The goal may contain variables: Any::Equal is variable-aware.
		if(Any::Equal(input->object->get_reference(0),target->get_reference(0)->get_reference(0))){	//	first, check the objects pointed to by the facts.

			int64	occurrence_time=Utils::GetTimestamp<Code>(input->object,FACT_TIME);	//	input->object is either a fact or a |fact.
			if(expected_time_low<=occurrence_time	&&	expected_time_high>=occurrence_time){

				if(input->object->get_sim()	||	input->object->get_asmp()	||	input->object->get_pred()){	//	TODO.
				}

				if(input->object->code(0)==target->get_reference(0)->code(0)){	//	positive match: expected a fact or |fact and got a fact or a |fact.

					parent->add_outcome(this,GSMonitor::SUCCESS,input->object->code(FACT_CFD).asFloat());
					success=true;
					return	true;	//	the caller will remove the monitor from its list (will not take any more inputs).
				}else{															//	negative match: expected a fact or |fact and got a |fact or a fact.

					parent->add_outcome(this,GSMonitor::FAILURE,input->object->code(FACT_CFD).asFloat());
					success=true;
					return	false;	//	stays in the controller's monitors list (will take more inputs).
				}
			}
		}
		return	false;
	}

	void	GMonitor::update(){	//	parent will kill itself (along with its monitors).

		if(target->is_invalidated())
			parent->add_outcome(this,GSMonitor::INVALIDATED,1);
		else	if(!success){	//	received nothing matching the target's object so far (neither positively nor negatively).

			uint64	now=Now();
			Code	*f;	//	generate the opposite of the target's fact (negative findings).
			if(target->get_reference(0)->code(0).asOpcode()==Opcodes::Fact)
				f=factory::Object::AntiFact(target->get_reference(0)->get_reference(0),now,1,1);
			else
				f=factory::Object::Fact(target->get_reference(0)->get_reference(0),now,1,1);

			Code	*fwd_model=parent->get_rgrp()->get_fwd_model();
			uint16	out_group_set_index=fwd_model->code(MD_OUT_GRPS).asIndex();
			uint16	out_group_count=fwd_model->code(out_group_set_index).getAtomCount();

			for(uint16	i=1;i<=out_group_count;++i){	//	inject the negative findings in the ouptut groups.

				Code	*out_group=fwd_model->get_reference(fwd_model->code(out_group_set_index+i).asIndex());
				View	*view=new	View(true,now,1,1,out_group,NULL,f);	//	inject in stdin: TODO: allow specifying the destination group.
				_Mem::Get()->inject(view);
			}

			parent->add_outcome(this,GSMonitor::FAILURE,1);
		}
	}*/
}