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

	RGRPOverlay::RGRPOverlay(__Controller	*c,RGroup	*group,UNORDERED_MAP<Code	*,P<Code> >	*bindings):Overlay(c){

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

	RGRPOverlay::RGRPOverlay(RGRPOverlay	*original):Overlay(original->controller){

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

			PGMController	*c=(PGMController	*)(*b)->controller;
			c->take_input(input,this);
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
				((RGRPMasterOverlay	*)controller)->fire(&bindings);
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

	RGRPMasterOverlay::RGRPMasterOverlay(RGRPController	*c,Code	*mdl,RGroup	*rgrp,UNORDERED_MAP<Code	*,P<Code> >	*bindings):_Controller<Overlay>(c->get_mem()){	//	when bindings==NULL, called by the controller of the head of the model.

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

	void	RGRPMasterOverlay::fire(UNORDERED_MAP<Code	*,P<Code> >	*bindings){	//	add one master overlay to the group's children' controller.
		
		((RGRPController	*)controller)->fire(bindings,&this->bindings);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RGRPController::RGRPController(_Mem	*mem,r_code::View	*view):Controller(mem,view){

		if(getObject()->code(0).asOpcode()==Opcodes::FMD){	//	build the master overlay.

			rgrp=(RGroup	*)getObject()->get_reference(getObject()->code(MD_HEAD).asIndex());
			rgrp->set_fwd_model(getObject());

			RGRPMasterOverlay	*m=new	RGRPMasterOverlay(this,getObject(),rgrp);
			overlays.push_back(m);
		}else
			rgrp=(RGroup	*)getObject();
	}
	
	RGRPController::~RGRPController(){
	}

	void	RGRPController::fire(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings){

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	fire.
			((RGRPController	*)v->second->controller)->activate(overlay_bindings,master_overlay_bindings);
	}

	void	RGRPController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		overlayCS.enter();
		if(overlays.size())
			reduce(input);
		else	//	not activated yet.
			pending_inputs.push_back(input);
		overlayCS.leave();

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->group_views.begin();v!=rgrp->group_views.end();++v)	//	pass the input to children controllers;
			v->second->controller->take_input(input);
	}

	void	RGRPController::activate(UNORDERED_MAP<Code	*,P<Code> >	*overlay_bindings,UNORDERED_MAP<Code	*,P<Code> >	*master_overlay_bindings){	//	called when a parent fires.

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
			injectProductions(&bindings);
		else{

			RGRPMasterOverlay	*m=new	RGRPMasterOverlay(this,rgrp->get_fwd_model(),rgrp,&bindings);

			overlayCS.enter();
			overlays.push_back(m);
			if(pending_inputs.size()){

				for(uint16	i=0;i<pending_inputs.size();++i)	//	pass the input to all master overlays.
					m->reduce(pending_inputs[i]);
				pending_inputs.clear();
			}
			overlayCS.leave();
		}
	}

	Code	*RGRPController::bind_object(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings)	const{

		Code	*bound_object=mem->buildObject(original->code(0));

		uint16	i;
		for(i=0;i<original->code_size();++i)	//	copy the code.
			bound_object->code(i)=original->code(i);
		for(i=0;i<original->references_size();++i){

			Code	*reference=original->get_reference(i);
			if(reference->code(0).asOpcode()==Opcodes::Var)
				bound_object->set_reference(i,(*bindings)[reference]);
			else
				bound_object->set_reference(i,reference);
		}

		return	bound_object;
	}

	Code	*RGRPController::get_mk_rdx(Code	*mdl,uint8	production_count,uint16	&extent_index)	const{

		extent_index=MK_GRDX_ARITY+1;

		Code	*mk_rdx=new	r_exec::LObject(mem);

		mk_rdx->code(0)=Atom::Marker(Opcodes::MkGRdx,MK_GRDX_ARITY);
		mk_rdx->code(MK_GRDX_MDL)=Atom::RPointer(0);				//	model.
		mk_rdx->code(MK_GRDX_PRODS)=Atom::IPointer(extent_index);	//	productions.
		mk_rdx->code(MK_GRDX_ARITY)=Atom::Float(1);					//	psln_thr.

		mk_rdx->code(extent_index++)=Atom::Set(production_count);

		mk_rdx->add_reference(mdl);

		return	mk_rdx;
	}

	void	RGRPController::injectProductions(UNORDERED_MAP<Code	*,P<Code> >	*bindings){

		Code	*fwd_model=rgrp->get_fwd_model();

		uint16	out_group_set_index=fwd_model->code(MD_OUT_GRPS).asIndex();
		uint16	out_group_count=fwd_model->code(out_group_set_index).getAtomCount();

		Code	*mk_rdx=NULL;
		uint16	write_index;
		if(fwd_model->code(MD_NFR).asBoolean())
			mk_rdx=get_mk_rdx(fwd_model,rgrp->other_views.size(),write_index);	//	write_index points at the first production.

		uint16	ntf_group_set_index=fwd_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=fwd_model->code(ntf_group_set_index).getAtomCount();

		uint64	now=Now();

		uint16	ref_index=0;
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=rgrp->other_views.begin();v!=rgrp->other_views.end();++v){	//	we are only interested in objects and markers.

			Code	*bound_object=bind_object(v->second->object,bindings);
			bound_object=mem->check_existence(bound_object);

			Code	*prediction=mem->buildObject(Atom::Object(Opcodes::Pred,PRED_ARITY));
			prediction->code(PRED_OBJ)=Atom::RPointer(0);
			prediction->code(PRED_TIME)=Atom::IPointer(PRED_ARITY+1);	//	iptr to time.
			Utils::SetTimestamp<Code>(prediction,PRED_TIME,now);
			prediction->code(PRED_CFD)=Atom::Float(1);		//	TODO: put the right (from where?) value for the confidence member.
			prediction->code(PRED_WHO)=Atom::RPointer(1);
			prediction->code(PRED_ARITY)=Atom::Float(1);	//	psln_thr.

			prediction->set_reference(0,bound_object);
			prediction->set_reference(1,fwd_model->get_reference(fwd_model->code(MD_WHO).asIndex()));

			if(mk_rdx){

				mk_rdx->code(write_index++)=Atom::RPointer(++ref_index);	//	0 is the reference to the model.
				mk_rdx->add_reference(prediction);
			}

			uint16	i;
			for(i=1;i<=out_group_count;++i){	//	inject in out groups.

				Code	*out_group=fwd_model->get_reference(fwd_model->code(out_group_set_index+i).asIndex());

				View	*view=new	View();
				view->code(VIEW_OPCODE)=Atom::SSet(Opcodes::PgmView,VIEW_ARITY);	//	Structured Set.
				view->code(VIEW_SYNC)=Atom::Boolean(true);			//	sync on front.
				view->code(VIEW_IJT)=Atom::IPointer(VIEW_ARITY+1);	//	iptr to ijt.
				Utils::SetTimestamp<View>(view,VIEW_IJT,now);		//	ijt.
				view->code(VIEW_SLN)=Atom::Float(1);				//	sln.
				view->code(VIEW_RES)=Atom::Float(1);				//	res.
				view->code(VIEW_HOST)=Atom::RPointer(0);			//	destination.
				view->code(VIEW_ORG)=Atom::RPointer(1);				//	host.

				view->references[0]=out_group;
				view->references[1]=rgrp;

				view->set_object(prediction);
				mem->inject(view);
			}
		}

		if(mk_rdx){

			for(uint16	i=1;i<=ntf_group_count;++i){

				NotificationView	*v=new	NotificationView(rgrp,(Group	*)fwd_model->get_reference(fwd_model->code(ntf_group_set_index+i).asIndex()),mk_rdx);
				mem->injectNotificationNow(v,false);
			}
		}
	}
}