//	rgrp_controller.cpp
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

#include	"rgrp_controller.h"
#include	"pgm_controller.h"
#include	"mem.h"


#define	P_MONITOR_GRACE_PERIOD	5000

namespace	r_exec{
	
	FwdController::FwdController(r_code::View	*view):Controller(view){

		if(getObject()->code(0).asOpcode()==Opcodes::Fmd){	//	build the master overlay.

			rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
			rgrp->set_fwd_model(getObject());
			rgrp->set_controller(this);

			FwdOverlay	*o=new	FwdOverlay(this,rgrp,NULL,RDX_MODE_REGULAR);	//	master overlay.
			overlays.push_back(o);
		}else
			rgrp=(RGroup	*)getObject();
	}

	FwdController::~FwdController(){
	}

	void	FwdController::fire(BindingMap	*bindings,uint8	reduction_mode){	//	called by reduction cores (fwd overlays' reduction jobs).

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	fire.
			((FwdController	*)v->second->controller)->activate(bindings,reduction_mode);
	}

	void	FwdController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.
		
		Code	*input_object=input->object;
		if(get_position()==HEAD	&&	input_object->code(0).asOpcode()!=Opcodes::Fact	&&	input_object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
			return;

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	pass the input to children controllers.
			v->second->controller->take_input(input,this);

		overlayCS.enter();
		if(overlays.size()){

			if(tsc>0){	// kill all overlays older than tsc.

				uint64	now=Now();
				std::list<P<_Overlay> >::iterator	o=++overlays.begin();
				for(;o!=overlays.end();){	// start from the first overlay (master, oldest), and erase all of them that are older than tsc.

					if(now-((FwdOverlay	*)(*o))->get_birth_time()>tsc){
						
						((FwdOverlay	*)(*o))->kill();
						o=overlays.erase(o);
					}else
						break;
				}
			}

			std::list<P<_Overlay> >::const_iterator	o;
			for(o=overlays.begin();o!=overlays.end();++o){

				ReductionJob	*j=new	ReductionJob(new	View(input),*o);
				_Mem::Get()->pushReductionJob(j);
			}
		}else	//	not activated yet, or tail.
			pending_inputs.push_back(input);
		if(get_position()==TAIL)	//	WARNING: HACK: solves a race condition (practice disproves theory once again).
			Thread::Sleep(1);
		overlayCS.leave();

		if(!input_object->get_sim()	&&
			!input_object->get_asmp()	&&
			!input_object->get_pred()){	//	monitoring pred/asmp: we are only interested in actual facts: discard pred, asmp and hyp/sim.

			p_monitorsCS.enter();
			std::list<P<PMonitor> >::const_iterator	p;
			for(p=p_monitors.begin();p!=p_monitors.end();++p)
				(*p)->take_input(input);
			p_monitorsCS.leave();
		}

		if(get_position()!=TAIL	&&	!input_object->get_goal()){	//	monitoring goals: discard facts marked as goals; this does not mean that goals are discarded when they are the input.

			gs_monitorsCS.enter();
			std::list<P<GSMonitor> >::const_iterator	gs;
			for(gs=gs_monitors.begin();gs!=gs_monitors.end();++gs){

				ReductionJob	*j=new	ReductionJob(new	View(input),*gs);
				_Mem::Get()->pushReductionJob(j);
			}
			gs_monitorsCS.leave();
		}
	}

	void	FwdController::activate(BindingMap	*bindings,uint8	reduction_mode){	//	called when a parent fires (originally called by a reduction core).

		overlayCS.enter();
		if(get_position()==TAIL)	//	all variables shall be bound: inject productions.
			inject_productions(bindings,reduction_mode);
		else{

			FwdOverlay	*o=new	FwdOverlay(this,rgrp,bindings,reduction_mode);	//	master overlay.

			overlays.push_back(o);
			for(uint16	i=0;i<pending_inputs.size();++i){

				ReductionJob	*j=new	ReductionJob(new	View(pending_inputs[i]),o);
				_Mem::Get()->pushReductionJob(j);
			}
		}
		pending_inputs.clear();
		overlayCS.leave();
	}

	void	FwdController::inject_productions(BindingMap	*bindings,uint8	reduction_mode){

		Code	*fwd_model=rgrp->get_fwd_model();

		uint16	out_group_set_index=fwd_model->code(MD_OUT_GRPS).asIndex();
		uint16	out_group_count=fwd_model->code(out_group_set_index).getAtomCount();

		uint64	now=Now();

		uint16	ref_index=0;
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->other_views.begin();v!=rgrp->other_views.end();++v){	//	we are only interested in objects and markers.

			Code	*original=v->second->object;
			Code	*bound_object=bindings->bind_object(original);	//	the predicted object (class: fact or |fact).
																	//	no need for existence check: the timings are always different, hence the facts never the same.
			//	Production/monitoring rules:
			//	1 - if predicted time > now: bound_object marked as a regular prediction; monitor the outcome = catch occurrences of bound_object at the predicted time.
			//	2 - if predicted time <= now: bound_object marked as an assumption; check among the pending inputs; no monitoring.
			//	3 - if simulation: bound_object marked as a simulation result; no monitoring.
			//	4 - if assumption: bound_object marked as an assumption; no monitoring.
			uint64	predicted_time=Utils::GetTimestamp<Code>(bound_object,FACT_TIME);
			float32	multiplier=original->code(original->code(FACT_TIME).asIndex()+1).getMultiplier();
			uint64	time_tolerance=abs((float32)((int64)(predicted_time-now)))*multiplier;
			Code	*primary_mk;	//	prediction or assumption.
			bool	prediction;		//	false means assumption.
			uint64	resilience;		//	of the primary marker.
			Code	*mk_sim=NULL;	//	if needed.
			Code	*mk_asmp=NULL;	//	if needed.

			if(predicted_time>now){

				primary_mk=factory::Object::MkPred(bound_object,fwd_model,1,1);	//	TODO: put the right value (from where?) for the confidence member.
				resilience=predicted_time+time_tolerance;	//	predictions are killed by their monitor.
				prediction=true;
			}else{

				primary_mk=factory::Object::MkAsmp(bound_object,fwd_model,1,1);	//	TODO: put the right value (from where?) for the confidence member.
				resilience=1;	//	assumptions are checked immediately against the pending inputs (see below).
				prediction=false;
			}

			Code	*fact=factory::Object::Fact(primary_mk,now,1,1);

			PMonitor	*monitor=NULL;
			if(reduction_mode==RDX_MODE_REGULAR)
				monitor=new	PMonitor(this,primary_mk,predicted_time,time_tolerance);
			else	if(reduction_mode	&	RDX_MODE_SIMULATION)					//	inject a simulation marker on the bound object.
				mk_sim=factory::Object::MkSim(bound_object,fwd_model,1);
			else	if((reduction_mode	&	RDX_MODE_ASSUMPTION)	&&	prediction)	//	inject an assumption marker on the bound object.
				mk_asmp=factory::Object::MkAsmp(bound_object,fwd_model,1,1);	//	TODO: put the right value (from where?) for the confidence member.

			for(uint16	i=1;i<=out_group_count;++i){	//	inject the markers in the model's output groups.

				Group	*out_group=(Group	*)fwd_model->get_reference(fwd_model->code(out_group_set_index+i).asIndex());
				View	*view;
				if(prediction){	//	resilience is actually a time to live (in us); convert into actual resilience.

					uint64	base=_Mem::Get()->get_base_period()*out_group->get_upr();
					uint64	r=(resilience-now)/base;	//	always >0.
					if(resilience%base>1)
						++r;
					resilience=r;
				}
				view=new	View(true,now,1,resilience,out_group,rgrp,primary_mk);
				_Mem::Get()->inject(view);

				//	inject the fact (pointing to primary_mk).
				view=new	View(true,now,1,resilience,out_group,rgrp,fact);
				_Mem::Get()->inject(view);

				if(mk_sim){

					view=new	View(true,now,1,_Mem::Get()->get_sim_res(),out_group,rgrp,mk_sim);
					_Mem::Get()->inject(view);
				}

				if(mk_asmp){

					view=new	View(true,now,1,_Mem::Get()->get_asmp_res(),out_group,rgrp,mk_asmp);
					_Mem::Get()->inject(view);
				}
			}

			if(monitor){
				
				for(uint16	i=0;i<pending_inputs.size();++i)	//	check the prediction/assumption among the pending inputs.
					monitor->take_input(pending_inputs[i]);

				add_monitor(monitor);
				if(prediction)	//	predicted_time>now: monitor the prediction's outcome.
					_Mem::Get()->pushTimeJob(new	MonitoringJob<PMonitor>(monitor,predicted_time+time_tolerance));
				else	//	predicted_time<=now: give a chance to get the expected object.
					_Mem::Get()->pushTimeJob(new	MonitoringJob<PMonitor>(monitor,now+P_MONITOR_GRACE_PERIOD));
			}
		}
	}

	void	FwdController::add_outcome(PMonitor	*m,bool	success,float32	confidence)	const{	//	success==false: executed in the thread of a time core; otherwise, executed in the same thread as for FwdController::take_input().

		Model	*fwd_model=(Model	*)rgrp->get_fwd_model();

		fwd_model->register_outcome(success,confidence);
		Code	*marker=factory::Object::MkSuccess(m->target,fwd_model->get_success_rate(),fwd_model->get_failure_rate(),1);

		uint64	now=Now();

		Code	*fact;
		if(success)
			fact=factory::Object::Fact(marker,now,confidence,1);
		else	//	there is no mk.failure: failure=|fact on success.
			fact=factory::Object::AntiFact(marker,now,confidence,1);

		uint16	ntf_group_set_index=fwd_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=fwd_model->code(ntf_group_set_index).getAtomCount();
		for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

			Group				*ntf_group=(Group	*)fwd_model->get_reference(fwd_model->code(ntf_group_set_index+i).asIndex());
			NotificationView	*ntf_view=new	NotificationView(rgrp,ntf_group,marker);
			_Mem::Get()->injectNotificationNow(ntf_view,true);

			View	*fact_view=new	View(true,now,1,_Mem::Get()->get_ntf_mk_res(),ntf_group,rgrp,fact);
			_Mem::Get()->inject(fact_view);
		}

		m->target->kill();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	InvController::InvController(r_code::View	*view):Controller(view){

		rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
	}

	InvController::~InvController(){
	}

	void	InvController::take_input(r_exec::View	*input,Controller	*origin){	//	propagates the instantiation of goals in one single thread.

		Code	*input_object=input->object;
		if(input_object->code(0).asOpcode()!=Opcodes::Fact	&&	input_object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but fact or |fact.
			return;
		Code	*goal=input_object->get_reference(0);
		if(goal->code(0).asOpcode()!=Opcodes::MkGoal)	//	discard everything but goals.
			return;

		bool	asmp=input_object->get_asmp()!=NULL;
		bool	sim=(input_object->get_hyp()!=NULL	||	input_object->get_sim()!=NULL);

		uint8	reduction_mode=RDX_MODE_REGULAR;
		if(sim)
			reduction_mode|=RDX_MODE_SIMULATION;
		if(asmp)
			reduction_mode|=RDX_MODE_ASSUMPTION;

		P<InvOverlay>	o=new	InvOverlay(this);
		P<View>			view=new	View(true,Now(),0,1,rgrp,NULL,goal->get_reference(0));	//	temporary view; N.B.: binders do not exploit view data.

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->ipgm_views.begin();v!=rgrp->ipgm_views.end();++v){	//	attempt to bind (one binder after the other).

			((PGMController	*)v->second->controller)->take_input(view,(InvOverlay	*)o);
			if(o->success){	//	one binder matched: propagate and stop.

				std::vector<Code	*>	initial_goal;
				initial_goal.push_back(goal);
				rgrp->instantiate_goals(&initial_goal,NULL,reduction_mode,getObject(),o->get_bindings());	//	here, getObject() returns the inverse model.
				return;
			}
		}
	}
}