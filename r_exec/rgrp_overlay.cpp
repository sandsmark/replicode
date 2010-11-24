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


namespace	r_exec{

	RGRPOverlay::RGRPOverlay(__Controller	*c,RGroup	*group,UNORDERED_MAP<Code	*,P<Code> >	*bindings):Overlay(c),simulation(false){

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

	RGRPOverlay::RGRPOverlay(RGRPOverlay	*original):Overlay(original->controller),simulation(false){

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

		View	*_input;
		Conclusion	c=RAW;
		if(input->object->code(0).asOpcode()==Opcodes::Sim	||
			input->object->code(0).asOpcode()==Opcodes::Hyp)
			c=SIM;
		else	if(input->object->code(0).asOpcode()==Opcodes::Asmp)
			c=ASM;

		if(c!=RAW){

			_input=new	View(input);
			_input->object=input->object->get_reference(0);
		}else
			_input=input;

		last_bound.clear();
		discard_bindings=false;

		std::list<P<View> >::const_iterator	b;
		for(b=binders.begin();b!=binders.end();++b){	//	binders called sequentially (at most one can bind an input).

			PGMController	*c=(PGMController	*)(*b)->controller;
			c->take_input(_input,this);
			if(discard_bindings)
				break;
			if(last_bound.size())
				break;
		}

		if(!discard_bindings	&&	last_bound.size()){	//	at least one variable was bound.

			RGRPOverlay	*offspring=new	RGRPOverlay(this);
			controller->add(offspring);

			if(!unbound_var_count){

				controller->remove(this);
				((RGRPMasterOverlay	*)controller)->fire(&bindings,c);
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

	RGRPMasterOverlay::RGRPMasterOverlay(FwdController	*c,Code	*mdl,RGroup	*rgrp,UNORDERED_MAP<Code	*,P<Code> >	*bindings):_Controller<Overlay>(c->get_mem()){	//	when bindings==NULL, called by the controller of the head of the model.

		controller=c;
		alive=true;

		tsc=Utils::GetTimestamp<Code>(mdl,FMD_TSC);

		if(bindings)
			this->bindings=*bindings;

		RGRPOverlay	*m=new	RGRPOverlay(this,rgrp,bindings);	//	master overlay.
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
			mem->pushReductionJob(j);
		}

		overlayCS.leave();
	}

	void	RGRPMasterOverlay::fire(UNORDERED_MAP<Code	*,P<Code> >	*bindings,Conclusion	c){	//	add one master overlay to the group's children' controller.
		
		((FwdController	*)controller)->fire(bindings,&this->bindings,c);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	FwdController::FwdController(_Mem	*mem,r_code::View	*view):Controller(mem,view){

		if(getObject()->code(0).asOpcode()==Opcodes::Fmd){	//	build the master overlay.

			rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
			rgrp->set_fwd_model(getObject());

			RGRPMasterOverlay	*m=new	RGRPMasterOverlay(this,getObject(),rgrp);
			overlays.push_back(m);
		}else
			rgrp=(RGroup	*)getObject();
	}
	
	FwdController::~FwdController(){
	}

	void	FwdController::fire(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings,Conclusion	c){

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	fire.
			((FwdController	*)v->second->controller)->activate(overlay_bindings,master_overlay_bindings,c);
	}

	void	FwdController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		bool	monitor_prediction=false;
		View	*_input;
		//	8 cases:
		//	a - raw input.
		//	b - prediction.
		//	c - hypothesis.
		//	d - simulation result.
		//	e - simulated prediction.
		//	f - simulated assumption.
		//	g - assumed prediction.
		//	h - assumption.
		//	We have to process the content instead of the qualifier (cases b to h).
		if(input->object->code(0).asOpcode()==Opcodes::Pred){			//	case b.

			_input=new	View(input);
			_input->object=input->object->get_reference(0);
			monitor_prediction=true;
		}else	if(input->object->code(0).asOpcode()==Opcodes::Hyp){	//	case c.

			_input=new	View(input);
			_input->object=input->object->get_reference(0);
		}else	if(input->object->code(0).asOpcode()==Opcodes::Sim){

			Code	*ref=input->object->get_reference(0);
			if(ref->code(0).asOpcode()==Opcodes::Pred	||
				ref->code(0).asOpcode()==Opcodes::Asmp){				//	cases e and f.

				_input=new	View(input);
				_input->object=ref->get_reference(0);
			}else{														//	case d.

				_input=new	View(input);
				_input->object=ref;
			}
		}else	if(input->object->code(0).asOpcode()==Opcodes::Asmp){

			Code	*ref=input->object->get_reference(0);
			if(ref->code(0).asOpcode()==Opcodes::Pred){					//	case g.

				_input=new	View(input);
				_input->object=ref->get_reference(0);
			}else{														//	case h.

				_input=new	View(input);
				_input->object=ref;
			}
		}else{															//	case a.
		
			_input=input;
			monitor_prediction=true;
		}

		overlayCS.enter();
		if(overlays.size())
			reduce(_input);
		else	//	not activated yet.
			pending_inputs.push_back(_input);
		overlayCS.leave();

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	pass the input to children controllers;
			v->second->controller->take_input(_input);

		if(!monitor_prediction)	//	we don't monitor the outcome of hypotheses, of simulation results nor of assumptions.
			return;

		monitorsCS.enter();
		UNORDERED_MAP<P<Monitor>,uint64,typename	MonitorHash>::const_iterator	m;
		for(m=monitors.begin();m!=monitors.end();++m)
			m->first->take_input(input);
		monitorsCS.leave();
	}

	void	FwdController::activate(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings,Conclusion	c){	//	called when a parent fires.

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

		if(get_position()==TAIL)	//	all variables shall be bound, just inject productions.
			injectProductions(&bindings,c);
		else{

			RGRPMasterOverlay	*m=new	RGRPMasterOverlay(this,rgrp->get_fwd_model(),rgrp,&bindings);

			overlayCS.enter();
			overlays.push_back(m);
			for(uint16	i=0;i<pending_inputs.size();++i)	//	pass the input to all master overlays.
				m->reduce(pending_inputs[i]);
			pending_inputs.clear();
			overlayCS.leave();
		}
	}

	Code	*FwdController::bind_object(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings)	const{

		Code	*bound_object=mem->buildObject(original->code(0));

		uint16	i;
		for(i=0;i<original->code_size();++i)		//	copy the code.
			bound_object->code(i)=original->code(i);
		for(i=0;i<original->references_size();++i)	//	bind references when needed.
			bound_object->set_reference(i,bind_reference(original->get_reference(i),bindings));

		return	bound_object;
	}

	Code	*FwdController::bind_reference(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings)	const{

		if(original->code(0).asOpcode()==Opcodes::Var)
			return	(*bindings)[original];

		for(uint16	i=0;i<original->references_size();++i){

			Code	*reference=original->get_reference(i);
			if(needs_binding(reference))
				return	bind_object(original,bindings);
		}

		return	original;
	}

	bool	FwdController::needs_binding(Code	*object)	const{

		if(object->code(0).asOpcode()==Opcodes::Var)
			return	true;

		for(uint16	i=0;i<object->references_size();++i){

			if(needs_binding(object->get_reference(i)))
				return	true;
		}

		return	false;
	}

	void	FwdController::injectProductions(UNORDERED_MAP<Code	*,P<Code> >	*bindings,Conclusion	c){

		Code	*fwd_model=rgrp->get_fwd_model();

		uint16	out_group_set_index=fwd_model->code(MD_OUT_GRPS).asIndex();
		uint16	out_group_count=fwd_model->code(out_group_set_index).getAtomCount();

		uint16	ntf_group_set_index=fwd_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=fwd_model->code(ntf_group_set_index).getAtomCount();

		uint64	now=Now();

		uint16	ref_index=0;
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->other_views.begin();v!=rgrp->other_views.end();++v){	//	we are only interested in objects and markers.

			Code	*bound_object=bind_object(v->second->object,bindings);	//	that is the predicted object (class: fact).
																			//	no need for existence check: the timings are always different, hence the facts never the same.
			//	Production/monitoring rules:
			//	1 - if !simulation and predicted time > now: regular prediction; monitor the outcome = catch occurrences of the predicted object at the predicted time.
			//	2 - if simulation and predicted time > now: simulated prediction; simulation => no monitoring.
			//	3 - if !simulation and predicted time <= now: assumption; for now, no monitoring. TODO: determine what to monitor (if anything).
			//	4 - if simulation and predicted time <= now: simulated assumption; simulation => no monitoring.
			uint64	predicted_time=Utils::GetTimestamp<Code>(bound_object->get_reference(1),VAL_VALUE);
			Code	*object_to_inject;
			uint32	resilience;
			if(predicted_time>now){	//	cases 1 and 2.

				Code	*prediction=mem->buildObject(Atom::Object(Opcodes::Pred,PRED_ARITY));
				prediction->code(PRED_OBJ)=Atom::RPointer(0);
				prediction->code(PRED_TIME)=Atom::IPointer(PRED_ARITY+1);	//	iptr to time.
				Utils::SetTimestamp<Code>(prediction,PRED_TIME,now);
				prediction->code(PRED_CFD)=Atom::Float(1);					//	TODO: put the right value (from where?) for the confidence member.
				prediction->code(PRED_FMD)=Atom::RPointer(1);
				prediction->code(PRED_ARITY)=Atom::Float(1);				//	psln_thr.
				prediction->set_reference(0,bound_object);
				prediction->set_reference(1,fwd_model);

				switch(c){
				case	RAW:{	//	inject the prediction.
					object_to_inject=prediction;

					resilience=mem->get_pred_res();

					//	Monitor the prediction's outcome.
					Monitor	*m=new	Monitor(this,prediction);
					monitorsCS.enter();
					monitors[m]=0;
					monitorsCS.leave();
					mem->pushTimeJob(new	MonitoringJob(m,predicted_time));
					break;
				}case	SIM:	//	inject a simulated prediction.
					object_to_inject=mem->buildObject(Atom::Object(Opcodes::Sim,SIM_ARITY));
					object_to_inject->code(SIM_OBJ)=Atom::RPointer(0);
					object_to_inject->code(SIM_TIME)=Atom::IPointer(SIM_ARITY+1);	//	iptr to time.
					Utils::SetTimestamp<Code>(object_to_inject,SIM_TIME,now);
					object_to_inject->code(SIM_CFD)=Atom::Float(1);					//	TODO: put the right value (from where?) for the confidence member.
					object_to_inject->code(SIM_ARITY)=Atom::Float(1);				//	psln_thr.
					object_to_inject->set_reference(0,prediction);

					resilience=mem->get_sim_res();
					break;
				case	ASM:	//	inject an assumed prediction.
					object_to_inject=mem->buildObject(Atom::Object(Opcodes::Asmp,ASMP_ARITY));
					object_to_inject->code(ASMP_OBJ)=Atom::RPointer(0);
					object_to_inject->code(ASMP_TIME)=Atom::IPointer(ASMP_ARITY+1);	//	iptr to time.
					Utils::SetTimestamp<Code>(object_to_inject,ASMP_TIME,now);
					object_to_inject->code(ASMP_CFD)=Atom::Float(1);				//	TODO: put the right value (from where?) for the confidence member.
					object_to_inject->code(ASMP_ARITY)=Atom::Float(1);				//	psln_thr.
					object_to_inject->set_reference(0,prediction);

					resilience=mem->get_asmp_res();
					break;
				}
			}else{					//	cases 3 and 4.

				Code	*assumption=mem->buildObject(Atom::Object(Opcodes::Asmp,ASMP_ARITY));
				assumption->code(ASMP_OBJ)=Atom::RPointer(0);
				assumption->code(ASMP_TIME)=Atom::IPointer(ASMP_ARITY+1);	//	iptr to time.
				Utils::SetTimestamp<Code>(assumption,ASMP_TIME,now);
				assumption->code(ASMP_CFD)=Atom::Float(1);					//	TODO: put the right value (from where?) for the confidence member.
				assumption->code(ASMP_ARITY)=Atom::Float(1);				//	psln_thr.
				assumption->set_reference(0,bound_object);

				switch(c){
				case	RAW:
				case	ASM:	//	inject an assumption.
					object_to_inject=assumption;

					resilience=mem->get_asmp_res();
					break;
				case	SIM:	//	inject a simulated assumption.
					object_to_inject=mem->buildObject(Atom::Object(Opcodes::Sim,SIM_ARITY));
					object_to_inject->code(SIM_OBJ)=Atom::RPointer(0);
					object_to_inject->code(SIM_TIME)=Atom::IPointer(SIM_ARITY+1);	//	iptr to time.
					Utils::SetTimestamp<Code>(object_to_inject,SIM_TIME,now);
					object_to_inject->code(SIM_CFD)=Atom::Float(1);					//	TODO: put the right value (from where?) for the confidence member.
					object_to_inject->code(SIM_ARITY)=Atom::Float(1);				//	psln_thr.
					object_to_inject->set_reference(0,assumption);

					resilience=mem->get_sim_res();
					break;
				}
			}

			for(uint16	i=1;i<=out_group_count;++i){	//	inject in out groups.

				Code	*out_group=fwd_model->get_reference(fwd_model->code(out_group_set_index+i).asIndex());

				View	*view=new	View(true,now,1,resilience,out_group,rgrp,object_to_inject);
				mem->inject(view);
			}
		}
	}

	void	FwdController::register_outcome(Monitor	*m,bool	outcome){

		Model	*fwd_model=(Model	*)rgrp->get_fwd_model();

		Code	*marker=mem->buildObject(Atom::Marker(Opcodes::MkFailure,MK_FAILURE_ARITY));	//	MK_SUCCESS_ARITY has the same value.
		marker->code(MK_SUCCESS_OBJ)=Atom::RPointer(0);
		marker->code(MK_SUCCESS_RATE)=Atom::Float(fwd_model->update(outcome));
		marker->code(MK_SUCCESS_ARITY)=Atom::Float(1);	//	psln_thr.

		marker->set_reference(0,m->prediction);

		uint16	ntf_group_set_index=fwd_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=fwd_model->code(ntf_group_set_index).getAtomCount();
		uint64	now=Now();

		for(uint16	i=1;i<=ntf_group_count;++i){	//	inject notification in ntf groups.

			Group				*ntf_group=(Group	*)fwd_model->get_reference(fwd_model->code(ntf_group_set_index+i).asIndex());
			NotificationView	*v=new	NotificationView(rgrp,ntf_group,marker);
			mem->injectNotificationNow(v,true);
		}

		monitorsCS.enter();
		monitors.erase(m);
		monitorsCS.leave();
	}

	void	FwdController::register_object(Monitor	*m,Code	*object){

		monitorsCS.enter();
		monitors[m]=Now();
		monitorsCS.leave();
	}

	void	FwdController::check_prediction(Monitor	*m){

		uint64	predicted_time=Utils::GetTimestamp<Code>(m->prediction->get_reference(0),FACT_TIME);
		uint64	occurrence_time;
		
		monitorsCS.enter();
		occurrence_time=monitors[m];
		monitorsCS.leave();

		if(occurrence_time==predicted_time)	//	TODO: define and use a tolerance on the time check.
			register_outcome(m,true);
		else
			register_outcome(m,false);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Monitor::Monitor(FwdController	*c,Code	*prediction):_Object(),controller(c){

		this->prediction=prediction;
	}

	bool	Monitor::is_alive()	const{

		return	controller->is_alive();
	}

	void	Monitor::take_input(r_exec::View	*input){

		if(input->object==prediction->get_reference(0))
			controller->register_object(this,input->object);
	}

	void	Monitor::update(){

		controller->check_prediction(this);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class	InvOverlay:	//	mere convenience to enable _subst to call the controller back.
	public	Overlay{
	public:
		UNORDERED_MAP<Code	*,P<Code> >	bindings;	//	variable|value.

		InvOverlay(InvController	*c):Overlay(c){}

		void	bind(Code	*value,Code	*var){

			bindings[var]=value;
		}
	};

	InvController::InvController(_Mem	*mem,r_code::View	*view):Controller(mem,view){

		rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
	}
	
	InvController::~InvController(){
	}

	void	InvController::take_input(r_exec::View	*input,Controller	*origin){	//	input points to a goal.

		if(input->object->code(0).asOpcode()!=Opcodes::Goal)	//	discard everything but goals.
			return;

		View	*_input=new	View(input);	//	process the target object instead of the goal object itself.
		_input->object=input->object->get_reference(0);

		InvOverlay	o(this);

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->ipgm_views.begin();v!=rgrp->ipgm_views.end();++v){	//	attempt to bind (one binder after the other).

			PGMController	*c=(PGMController	*)v->second->controller;
			c->take_input(input,&o);

			if(o.bindings.size())	//	one binder matched.
				break;
		}

		if(o.bindings.size())
			rgrp->get_parent()->instantiate_goals(&o.bindings);
	}
}