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

#include	"rgrp_overlay.h"
#include	"mem.h"
#include	"fact.h"


#define	RDX_MODE_REGULAR	0
#define	RDX_MODE_SIMULATION	1
#define	RDX_MODE_ASSUMPTION	2

namespace	r_exec{

	RGRPOverlay::RGRPOverlay(__Controller	*c,RGroup	*group,UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode):Overlay(c),reduction_mode(reduction_mode){

		//	init bindings from r-group.
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=group->variable_views.begin();v!=group->variable_views.end();++v)
			this->bindings[v->second->object]=NULL;
		unbound_var_count=this->bindings.size();

		if(bindings){	//	get values already assigned (by c) to some of the variables in use in the r-group.

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b;
			for(b=this->bindings.begin();b!=this->bindings.end();++b){

				UNORDERED_MAP<Code	*,P<Code> >::const_iterator	_b=bindings->find(b->first);
				if(_b!=bindings->end()){

					this->bindings[b->first]=_b->second;
					--unbound_var_count;
				}
			}
		}

		//	start will all binders.
		for(v=group->ipgm_views.begin();v!=group->ipgm_views.end();++v)
			binders.push_back(v->second);

		birth_time=Now();
	}

	RGRPOverlay::RGRPOverlay(RGRPOverlay	*original):Overlay(original->controller),reduction_mode(original->reduction_mode){

		//	copy binders from the original.
		std::list<P<View> >::const_iterator	b;
		for(b=original->binders.begin();b!=original->binders.end();++b)
			binders.push_back(*b);

		//	copy bindings from the original.
		unbound_var_count=0;
		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	it;
		for(it=original->bindings.begin();it!=original->bindings.end();++it){

			bindings[it->first]=it->second;
			if(it->second==NULL)
				++unbound_var_count;
		}

		//	reintegrate the last bound variables.
		for(uint16	i=0;i<original->last_bound.size();++i)
			bindings[original->last_bound[i]]=NULL;
		unbound_var_count+=original->last_bound.size();

		birth_time=Now();
	}

	RGRPOverlay::~RGRPOverlay(){
	}

	void	RGRPOverlay::reduce(r_exec::View	*input){

		//	for all remaining binders in this overlay:
		//		binder_view->take_input(input,this); "this" is needed calling the overlay back from _subst.
		//		_subst: callback (bind()):
		//			store values for variables for this overlay.
		//			if an object of the r-grp becomes fully bound (how to know?), inject a copy (poitning to actual values instead of variables)
		//			in the out_grps.
		//	if at least one binder matched:
		//		create an overlay (binders and bindings as they were prior to the match).
		//		remove the binder from this overlay.
		//	if no more unbound values, fire and kill the overlay.

		reductionCS.enter();

		last_bound.clear();
		discard_bindings=false;

		std::list<P<View> >::const_iterator	b;
		for(b=binders.begin();b!=binders.end();++b){	//	binders called sequentially (at most one can bind an input).

			((PGMController	*)(*b)->controller)->take_input(input,this);
			if(discard_bindings)
				break;
			if(last_bound.size())
				break;
		}

		if(!discard_bindings	&&	last_bound.size()){	//	at least one variable was bound.

			RGRPOverlay	*offspring=new	RGRPOverlay(this);
			controller->add(offspring);

			if((reduction_mode	&	RDX_MODE_SIMULATION)==0	&&	input->object->get_sim())
				reduction_mode|=RDX_MODE_SIMULATION;
			if((reduction_mode	&	RDX_MODE_ASSUMPTION)==0	&&	input->object->get_asmp())
				reduction_mode|=RDX_MODE_ASSUMPTION;

			if(!unbound_var_count){

				controller->remove(this);
				((RGRPMasterOverlay	*)controller)->fire(&bindings,reduction_mode);
			}else
				binders.erase(b);
		}

		reductionCS.leave();
	}

	void	RGRPOverlay::bind(Code	*value,Code	*var){	//	called back by _subst.

		Code	*v=bindings[var];
		if(v){
			
			if(v!=value)
				discard_bindings=true;	//	when an object has at least one value differing from an existing binding.
			return;
		}

		bindings[var]=value;
		--unbound_var_count;
		last_bound.push_back(var);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RGRPMasterOverlay::RGRPMasterOverlay(FwdController	*c,Code	*mdl,RGroup	*rgrp,UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode):_Controller<Overlay>(){	//	when bindings==NULL, called by the controller of the head of the model.

		controller=c;
		alive=true;

		tsc=Utils::GetTimestamp<Code>(mdl,FMD_TSC);

		if(bindings)
			this->bindings=*bindings;

		RGRPOverlay	*m=new	RGRPOverlay(this,rgrp,bindings,reduction_mode);	//	master overlay.
		overlays.push_back(m);
	}
	
	RGRPMasterOverlay::~RGRPMasterOverlay(){
	}

	void	RGRPMasterOverlay::reduce(r_exec::View	*input){

		overlayCS.enter();

		if(tsc>0){	// kill all overlays older than tsc.

			uint64	now=Now();
			std::list<P<_Overlay> >::iterator	o;
			for(o=overlays.begin();o!=overlays.end();){	// start from the first overlay (oldest), and erase all of them that are older than tsc.

				if(now-((RGRPOverlay	*)(*o))->birth_time>tsc){
					
					((RGRPOverlay	*)(*o))->kill();
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

		overlayCS.leave();
	}

	void	RGRPMasterOverlay::fire(UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode){	//	add one master overlay to the group's children' controller.
		
		((FwdController	*)controller)->fire(bindings,&this->bindings,reduction_mode);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	FwdController::FwdController(r_code::View	*view):Controller(view){

		if(getObject()->code(0).asOpcode()==Opcodes::Fmd){	//	build the master overlay.

			rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
			rgrp->set_fwd_model(getObject());
			rgrp->set_controller(this);

			RGRPMasterOverlay	*m=new	RGRPMasterOverlay(this,getObject(),rgrp,NULL,RDX_MODE_REGULAR);
			overlays.push_back(m);
		}else
			rgrp=(RGroup	*)getObject();
	}
	
	FwdController::~FwdController(){
	}

	void	FwdController::fire(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,
								UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings,
								uint8							reduction_mode){

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	fire.
			((FwdController	*)v->second->controller)->activate(overlay_bindings,master_overlay_bindings,reduction_mode);
	}

	void	FwdController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		overlayCS.enter();
		if(overlays.size())
			reduce(input);
		else	//	not activated yet, or tail.
			pending_inputs.push_back(input);
		overlayCS.leave();

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	pass the input to children controllers;
			v->second->controller->take_input(input);

		monitorsCS.enter();
		std::list<P<Monitor> >::const_iterator	m;
		for(m=monitors.begin();m!=monitors.end();){

			if((*m)->take_input(input))
				m=monitors.erase(m);
			else
				++m;
		}
		monitorsCS.leave();
	}

	void	FwdController::activate(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,
									UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings,
									uint8							reduction_mode){	//	called when a parent fires.

		//	fuse the bindings.
		UNORDERED_MAP<Code	*,P<Code> >	bindings;
		UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b;
		for(b=overlay_bindings->begin();b!=overlay_bindings->end();++b)
			bindings[b->first]=b->second;
		for(b=master_overlay_bindings->begin();b!=master_overlay_bindings->end();++b){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	_b=overlay_bindings->find(b->first);
			if(_b==overlay_bindings->end())
				bindings[b->first]=b->second;
		}

		if(get_position()==TAIL)	//	all variables shall be bound: inject productions.
			inject_productions(&bindings,reduction_mode);
		else{

			RGRPMasterOverlay	*m=new	RGRPMasterOverlay(this,rgrp->get_fwd_model(),rgrp,&bindings,reduction_mode);

			overlayCS.enter();
			overlays.push_back(m);
			for(uint16	i=0;i<pending_inputs.size();++i)	//	pass the input to all master overlays.
				m->reduce(pending_inputs[i]);
			overlayCS.leave();
		}

		pending_inputs.clear();
	}

	void	FwdController::inject_productions(UNORDERED_MAP<Code	*,P<Code> >	*bindings,uint8	reduction_mode){

		Code	*fwd_model=rgrp->get_fwd_model();

		uint16	out_group_set_index=fwd_model->code(MD_OUT_GRPS).asIndex();
		uint16	out_group_count=fwd_model->code(out_group_set_index).getAtomCount();

		uint16	ntf_group_set_index=fwd_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=fwd_model->code(ntf_group_set_index).getAtomCount();

		uint64	now=Now();

		uint16	ref_index=0;
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->other_views.begin();v!=rgrp->other_views.end();++v){	//	we are only interested in objects and markers.

			Code	*bound_object=RGroup::BindObject(v->second->object,bindings);	//	that is the predicted object (class: fact or |fact).
																					//	no need for existence check: the timings are always different, hence the facts never the same.
			//	Production/monitoring rules:
			//	1 - if predicted time > now: bound_object marked as a regular prediction; monitor the outcome = catch occurrences of bound_object at the predicted time.
			//	2 - if predicted time <= now: bound_object marked as an assumption; check among the pending inputs; no monitoring.
			//	3 - if simulation: bound_object marked as a simulation result; no monitoring.
			//	4 - if assumption: bound_object marked as an assumption; no monitoring.
			uint64	predicted_time=Utils::GetTimestamp<Code>(bound_object->get_reference(1),VAL_VAL);
			uint64	time_tolerance=abs((float32)((int64)(predicted_time-now)))*bound_object->get_reference(1)->code(VAL_TOL).asFloat();
			Code	*primary_mk;	//	prediction or assumption.
			bool	prediction;		//	false means assumption.
			int32	resilience;		//	of the primary marker.
			Code	*mk_sim=NULL;	//	if needed.
			Code	*mk_asmp=NULL;	//	if needed.

			if(predicted_time>now){

				primary_mk=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkPred,MK_PRED_ARITY));
				primary_mk->code(MK_PRED_OBJ)=Atom::RPointer(0);
				primary_mk->code(MK_PRED_FMD)=Atom::RPointer(1);
				primary_mk->code(MK_PRED_CFD)=Atom::Float(1);	//	TODO: put the right value (from where?) for the confidence member.
				primary_mk->code(MK_PRED_ARITY)=Atom::Float(1);	//	psln_thr.
				primary_mk->set_reference(0,bound_object);
				primary_mk->set_reference(1,fwd_model);

				resilience=-1;	//	forever; predictions are killed by their monitor.
				prediction=true;
			}else{

				primary_mk=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkAsmp,MK_ASMP_ARITY));
				primary_mk->code(MK_ASMP_OBJ)=Atom::RPointer(0);
				primary_mk->code(MK_ASMP_MDL)=Atom::RPointer(1);
				primary_mk->code(MK_ASMP_CFD)=Atom::Float(1);	//	TODO: put the right value (from where?) for the confidence member.
				primary_mk->code(MK_ASMP_ARITY)=Atom::Float(1);	//	psln_thr.
				primary_mk->set_reference(0,bound_object);
				primary_mk->set_reference(1,fwd_model);

				resilience=_Mem::Get()->get_asmp_res();
				prediction=false;
			}

			PMonitor	*monitor=NULL;
			if(reduction_mode==RDX_MODE_REGULAR){

				monitor=new	PMonitor(this,primary_mk,predicted_time,time_tolerance);
				if(prediction)	//	monitor the prediction's outcome.
					add_monitor(monitor);
			}else	if(reduction_mode	&	RDX_MODE_SIMULATION){	//	inject a simulation marker on the bound object.

				mk_sim=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkSim,MK_SIM_ARITY));
				mk_sim->code(MK_SIM_OBJ)=Atom::RPointer(0);
				mk_sim->code(MK_SIM_MDL)=Atom::RPointer(1);
				mk_sim->code(MK_SIM_ARITY)=Atom::Float(1);	//	psln_thr.
				mk_sim->set_reference(0,bound_object);
				mk_sim->set_reference(1,fwd_model);
			}else	if((reduction_mode	&	RDX_MODE_ASSUMPTION)	&&	prediction){	//	inject an assumption marker on the bound object.

				mk_asmp=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkAsmp,MK_ASMP_ARITY));
				mk_asmp->code(MK_ASMP_OBJ)=Atom::RPointer(0);
				mk_asmp->code(MK_ASMP_MDL)=Atom::RPointer(1);
				mk_asmp->code(MK_ASMP_CFD)=Atom::Float(1);		//	TODO: put the right value (from where?) for the confidence member.
				mk_asmp->code(MK_ASMP_ARITY)=Atom::Float(1);	//	psln_thr.
				mk_asmp->set_reference(0,bound_object);
				mk_asmp->set_reference(1,fwd_model);
			}

			for(uint16	i=1;i<=out_group_count;++i){	//	inject the bound object in the model's output groups.

				Code	*out_group=fwd_model->get_reference(fwd_model->code(out_group_set_index+i).asIndex());
				View	*view=new	View(true,now,1,1,out_group,rgrp,bound_object);
				_Mem::Get()->inject(view);
			}

			for(uint16	i=1;i<=ntf_group_count;++i){	//	inject the markers in the model's notification groups.

				Code	*ntf_group=fwd_model->get_reference(fwd_model->code(ntf_group_set_index+i).asIndex());
				View	*view=new	View(true,now,1,resilience,ntf_group,rgrp,primary_mk);
				_Mem::Get()->inject(view);

				if(mk_sim){

					view=new	View(true,now,1,_Mem::Get()->get_sim_res(),ntf_group,rgrp,mk_sim);
					_Mem::Get()->inject(view);
				}

				if(mk_asmp){

					view=new	View(true,now,1,_Mem::Get()->get_asmp_res(),ntf_group,rgrp,mk_asmp);
					_Mem::Get()->inject(view);
				}
			}

			if(monitor){
				
				for(uint16	i=0;i<pending_inputs.size();++i)	//	check the prediction/assumption among the pending inputs.
					if(monitor->take_input(pending_inputs[i])){

						delete	monitor;
						monitor=NULL;
						break;
					}

				if(monitor)	//	predicted_time>now: wait for the predicted time.
					_Mem::Get()->pushTimeJob(new	MonitoringJob(monitor,predicted_time+time_tolerance));
			}
		}
	}

	void	FwdController::add_outcome(PMonitor	*m,bool	success){	//	success==false: executed in the thread of a time core; otherwise, executed in the same thread as for FwdController::take_input().

		Model	*fwd_model=(Model	*)rgrp->get_fwd_model();

		Code	*marker=_Mem::Get()->buildObject(Atom::Marker(success?Opcodes::MkSuccess:Opcodes::MkFailure,MK_SUCCESS_ARITY));	//	MK_FAILURE_ARITY has the same value.
		marker->code(MK_SUCCESS_OBJ)=Atom::RPointer(0);
		marker->code(MK_SUCCESS_RATE)=Atom::Float(fwd_model->update(success));
		marker->code(MK_SUCCESS_ARITY)=Atom::Float(1);	//	psln_thr.
		marker->set_reference(0,m->target);

		uint16	ntf_group_set_index=fwd_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=fwd_model->code(ntf_group_set_index).getAtomCount();
		for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

			Group				*ntf_group=(Group	*)fwd_model->get_reference(fwd_model->code(ntf_group_set_index+i).asIndex());
			NotificationView	*v=new	NotificationView(rgrp,ntf_group,marker);
			_Mem::Get()->injectNotificationNow(v,true);
		}

		m->target->kill();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Monitor::Monitor():_Object(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PMonitor::PMonitor(FwdController	*c,Code	*target,uint64	expected_time,uint64	time_tolerance):Monitor(),controller(c),target(target),success(false){

		expected_time_high=expected_time+time_tolerance;
		expected_time_low=expected_time-time_tolerance;
	}

	bool	PMonitor::is_alive(){

		return	controller->is_alive();
	}

	bool	PMonitor::take_input(r_exec::View	*input){	

		if(Fact::Equal(input->object,target->get_reference(0))){	//	executed in the same thread as for FwdController::take_input().

			int64	occurrence_time=Utils::GetTimestamp<Code>(input->object->get_reference(1),VAL_VAL);	//	input->object is either a fact or a |fact.
			if(expected_time_low<=occurrence_time	&&	expected_time_high>=occurrence_time){

				success=true;
				controller->add_outcome(this,true);
				return	true;	//	the caller will remove the monitor from its list.
			}
		}
		return	false;
	}

	void	PMonitor::update(){

		if(!success){

			controller->add_outcome(this,false);
			controller->remove_monitor(this);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GSMonitor::GSMonitor(Model			*inv_model,
						 FwdController	*c,
						 GSMonitor		*parent,
						 bool			sim,
						 bool			asmp):Monitor(),
											inv_model(inv_model),
											controller(c),
											sim(sim),
											asmp(asmp),
											parent(parent),
											alive(true){
	}

	bool	GSMonitor::is_alive(){

		bool	_alive;
		aliveCS.enter();
		_alive=alive;
		aliveCS.leave();
		return	_alive;
	}

	void	GSMonitor::kill(){

		aliveCS.enter();
		alive=false;
		aliveCS.leave();

		monitorsCS.enter();
		monitors.clear();
		monitorsCS.leave();
	}

	bool	GSMonitor::take_input(r_exec::View	*input){	//	input->object is either a fact or a |fact.

		if(!is_alive())
			return	false;

		monitorsCS.enter();
		UNORDERED_MAP<P<Monitor>,uint64,typename	MonitorHash>::const_iterator	m;
		for(m=monitors.begin();m!=monitors.end();++m)
			m->first->take_input(input);
		monitorsCS.leave();
		return	true;
	}

	void	GSMonitor::register_outcome(GMonitor	*m,Code	*object){	//	executed in the same thread as GSMonitor::take_input().

		int64	occurrence_time=Utils::GetTimestamp<Code>(object->get_reference(1),VAL_VAL);	//	object's class: fact or |fact.

		monitors[m]=occurrence_time;
	}

	void	GSMonitor::update(){
	}

	void	GSMonitor::add_outcome(GMonitor	*m,bool	outcome){	//	executed in the thread of a time core (from GSMonitor::update()).

		P<GMonitor>	monitor=m;

		Code	*marker=_Mem::Get()->buildObject(Atom::Marker(outcome?Opcodes::MkSuccess:Opcodes::MkFailure,MK_SUCCESS_ARITY));	//	MK_FAILURE_ARITY has the same value.
		marker->code(MK_SUCCESS_OBJ)=Atom::RPointer(0);
		marker->code(MK_SUCCESS_RATE)=Atom::Float(inv_model->update(outcome));
		marker->code(MK_SUCCESS_ARITY)=Atom::Float(1);	//	psln_thr.
		marker->set_reference(0,monitor->target);

		uint16	ntf_group_set_index=inv_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=inv_model->code(ntf_group_set_index).getAtomCount();
		for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

			Group				*ntf_group=(Group	*)inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
			NotificationView	*v=new	NotificationView(controller->get_rgrp(),ntf_group,marker);
			_Mem::Get()->injectNotificationNow(v,true);
		}

		uint32	monitor_count;
		monitorsCS.enter();
		monitors.erase(m);
		monitor_count=monitors.size();
		monitorsCS.leave();

		if(outcome){

			if(monitor_count==0){	//	no monitor left: all monitors succeeded.

				kill();

				uint16	out_group_set_index=inv_model->code(MD_OUT_GRPS).asIndex();
				uint16	out_group_count=inv_model->code(out_group_set_index).getAtomCount();

				uint64	now=Now();

				monitor->target->acq_markers();
				std::list<Code	*>::const_iterator	mk;
				for(mk=monitor->target->markers.begin();mk!=monitor->target->markers.end();++mk){	//	the monitor's target is marked by sub-goal markers pointing to the super-goals.

					if((*mk)->code(0).asOpcode()==Opcodes::MkSubGoal){	//	inject the super-goals and associated objects and associated sim/asmp if any.

						Code	*super_goal=(*mk)->get_reference(0);
						Code	*object=super_goal->get_reference(0);
						Code	*mk_sim=get_mk_sim(object);
						Code	*mk_asmp=get_mk_asmp(object);

						for(uint16	i=1;i<=out_group_count;++i){	//	inject the bound object in the model's output groups.

							Code	*out_group=inv_model->get_reference(inv_model->code(out_group_set_index+i).asIndex());
							View	*view=new	View(true,now,1,1,out_group,controller->get_rgrp(),object);
							_Mem::Get()->inject(view);
						}

						for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

							Group	*ntf_group=(Group	*)inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
							View	*view;

							view=new	View(true,now,1,_Mem::Get()->get_goal_res(),ntf_group,controller->get_rgrp(),super_goal);
							_Mem::Get()->inject(view);

							if(mk_sim){

								view=new	View(true,now,1,_Mem::Get()->get_sim_res(),ntf_group,controller->get_rgrp(),mk_sim);
								_Mem::Get()->inject(view);
							}

							if(mk_asmp){

								view=new	View(true,now,1,_Mem::Get()->get_asmp_res(),ntf_group,controller->get_rgrp(),mk_asmp);
								_Mem::Get()->inject(view);
							}
						}

						if(parent){	//	add a GMonitor to the parent GSMonitor.

							GMonitor	*m=new	GMonitor(parent,super_goal);
							uint64		expected_time=Utils::GetTimestamp<Code>(object->get_reference(1),VAL_VAL);

							parent->add_monitor(m);
							_Mem::Get()->pushTimeJob(new	MonitoringJob(m,expected_time));
						}
					}
				}
				monitor->target->rel_markers();
			}
		}else{	//	one monitor failed.

			/*recursive_*/kill();	//	kill this and recursively its parent.

			monitor->target->acq_markers();
			std::list<Code	*>::const_iterator	mk;
			for(mk=monitor->target->markers.begin();mk!=monitor->target->markers.end();++mk){	//	recursively kill the markers and the super-goals.

				if((*mk)->code(0).asOpcode()==Opcodes::MkSubGoal){	//	TODO.

				}
			}
			monitor->target->rel_markers();
		}

		//	TODO: when a super-goal dies, recursively kill all sub-goals and monitors.
		//	TODO: when a super-goal succeeds, recursively kill all sub-goals and monitors.

		//	TODO: when a super-goal changes saliency, use the sln propagation to replicate to sub-goals; but not the other way around.
		//	N.B.: shall a goal become non salient, this has no effect on its monitors, if any.

		if(!is_alive())
			controller->remove_monitor(this);
	}

	void	GSMonitor::update(GMonitor	*m){	//	executed in the thread of a time core.

		uint64	expected_time=Utils::GetTimestamp<Code>(m->target->get_reference(0)->get_reference(1),VAL_VAL);
		uint64	occurrence_time;
		
		monitorsCS.enter();
		occurrence_time=monitors[m];
		monitorsCS.leave();

		if(occurrence_time==expected_time)	//	TODO: define and use a tolerance on the time check.
			add_outcome(m,true);
		else
			add_outcome(m,false);
	}

	Code	*GSMonitor::get_mk_sim(Code	*object)	const{

		if(sim){

			Code	*mk_sim=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkSim,MK_SIM_ARITY));
			mk_sim->code(MK_SIM_OBJ)=Atom::RPointer(0);
			mk_sim->code(MK_SIM_MDL)=Atom::RPointer(1);
			mk_sim->code(MK_SIM_ARITY)=Atom::Float(1);	//	psln_thr.
			mk_sim->set_reference(0,object);
			mk_sim->set_reference(1,inv_model);
			return	mk_sim;
		}else
			return	NULL;
	}
	
	Code	*GSMonitor::get_mk_asmp(Code	*object)	const{

		if(asmp){

			Code	*mk_asmp=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkAsmp,MK_ASMP_ARITY));
			mk_asmp->code(MK_ASMP_OBJ)=Atom::RPointer(0);
			mk_asmp->code(MK_ASMP_MDL)=Atom::RPointer(1);
			mk_asmp->code(MK_ASMP_CFD)=Atom::Float(1);		//	TODO: put the right value (from where?) for the confidence member.
			mk_asmp->code(MK_ASMP_ARITY)=Atom::Float(1);	//	psln_thr.
			mk_asmp->set_reference(0,object);
			mk_asmp->set_reference(1,inv_model);
			return	mk_asmp;
		}else
			return	NULL;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GMonitor::GMonitor(GSMonitor	*p,Code	*target):Monitor(),parent(p),target(target){
	}

	bool	GMonitor::is_alive(){

		return	parent->is_alive();
	}

	bool	GMonitor::take_input(r_exec::View	*input){	//	input->object is either a fact or a |fact.

		if(input->object==target->get_reference(0))	//	TODO: use relaxed, content-based, comparison. WARNING: the goal may contain variables: needs binding.
			parent->register_outcome(this,input->object);
		return	true;
	}

	void	GMonitor::update(){

		parent->update(this);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class	InvOverlay:	//	mere convenience to enable _subst to call the controller back.
	public	Overlay{
	public:
		UNORDERED_MAP<Code	*,P<Code> >	bindings;	//	variable|value.

		InvOverlay(InvController	*c):Overlay(c){}

		void	bind(Code	*value,Code	*var){	//	the goal object being bound may contain unbound variables.

			if(!value->code(0).asOpcode()==Opcodes::Var)
				bindings[var]=value;
		}
	};

	InvController::InvController(r_code::View	*view):Controller(view){

		rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
	}
	
	InvController::~InvController(){
	}

	void	InvController::take_input(r_exec::View	*input,Controller	*origin){	//	propagates the instantiation of goals in one single thread.

		Code	*goal=input->object->get_goal();
		if(!goal)	//	discard everything but goals: monitors take their inputs from the fwd controllers.
			return;

		bool	asmp=input->object->get_asmp()!=NULL;
		bool	sim=(input->object->get_hyp()!=NULL	||	input->object->get_sim()!=NULL);

		InvOverlay	o(this);

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->ipgm_views.begin();v!=rgrp->ipgm_views.end();++v){	//	attempt to bind (one binder after the other).

			((PGMController	*)v->second->controller)->take_input(input,&o);
			if(o.bindings.size())	//	one binder matched.
				break;
		}

		std::vector<Code	*>	initial_goals;
		initial_goals.push_back(goal);
		if(o.bindings.size())	//	propagate.
			rgrp->get_parent()->instantiate_goals(&initial_goals,NULL,sim,asmp,getObject(),&o.bindings);	//	here, getObject() returns the inverse model.
	}
}