//	auto_focus.cpp
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

#include	"auto_focus.h"
#include	"ast_controller.h"


namespace	r_exec{

	AutoFocusController::AutoFocusController(r_code::View	*view):Controller(view){

		// Load arguments: pass_through, acquire_models, decompile_models, list of output groups: 1st must be the primary, 2nd the secondary, then other groups.
		Code	*icpp_pgm=getObject();
		uint16	arg_set_index=icpp_pgm->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=icpp_pgm->code(arg_set_index).getAtomCount();
		_pass_through=icpp_pgm->code(arg_set_index+1).asBoolean();
		_acquire_models=icpp_pgm->code(arg_set_index+2).asBoolean();
		_decompile_models=icpp_pgm->code(arg_set_index+3).asBoolean();
		for(uint16	i=3;i<arg_count;++i)
			output_groups.push_back((Group	*)icpp_pgm->get_reference(i-3));

		cross_buffer.set_thz(_Mem::Get()->get_tpx_time_horizon());
		cross_buffer.reserve(1024);
		uint64	thz=2*((r_exec::View*)view)->get_host()->get_upr()*Utils::GetBasePeriod();	// thz==2*sampling period.
		cache.set_thz(thz);
		cache.reserve(128);
	}

	AutoFocusController::~AutoFocusController(){
	}

	Code	*AutoFocusController::get_core_object()	const{

		return	getObject();	// icpp_pgm.
	}

	inline	void	AutoFocusController::inject_input(View	*input,uint32	start){

		Group	*origin=input->get_host();
		for(uint16	i=start;i<output_groups.size();++i){

			Group	*output_group=output_groups[i];
			View	*view=new	View(input,true);
			view->references[0]=output_group;
			view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
			_Mem::Get()->inject(view);
		}
	}

	inline	void	AutoFocusController::inject_input(View	*input,_Fact	*abstract_input,BindingMap	*bm){

		View	*primary_view=inject_input(input);
		cross_buffer.push_back(Input(primary_view,abstract_input,bm));
		std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" inj: "<<input->object->get_oid()<<"|"<<primary_view->object->get_oid();
		if(input->get_sync()==View::SYNC_HOLD)std::cout<<" HOLD ";
		std::cout<<std::endl;
	}

	inline	View	*AutoFocusController::inject_input(View	*input){

		_Fact	*input_fact=(_Fact	*)input->object;

		Group	*origin=input->get_host();
		Group	*ref_group=output_groups[0];

		uint64	now=Now();

		View	*primary_view;
		_Fact	*copy;
		switch(input->get_sync()){
		case	View::SYNC_ONCE:		// no copy, morph res; N.B.: cmds are sync_once.
			for(uint16	i=0;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
				_Mem::Get()->inject(view);
				if(i==0)
					primary_view=view;
			}
			break;
		case	View::SYNC_PERIODIC:	// inject a copy, morph res, add a controller.
			if(input_fact->is_anti_fact())
				copy=new	AntiFact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			else
				copy=new	Fact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			for(uint16	i=0;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
				view->object=copy;
				_Mem::Get()->inject(view);
				if(i==0){
					
					primary_view=view;
					if(_acquire_models)
						_Mem::Get()->inject_null_program(new	PASTController(this,view),output_group,output_group->get_upr()*Utils::GetBasePeriod(),true);
				}
			}
			break;
		case	View::SYNC_HOLD:{		// inject a copy, add a controller, sync_once, morph res, after=now+time_tolerance (de-sync as it can have the same effect as a cmd), before=now+output_grp.upr+time_tolerance.
			uint64	offset=2*Utils::GetTimeTolerance();
			if(input_fact->is_anti_fact())
				copy=new	AntiFact(input_fact->get_reference(0),now+offset,now+offset+ref_group->get_upr()*Utils::GetBasePeriod(),1,1);
			else
				copy=new	Fact(input_fact->get_reference(0),now+offset,now+offset+ref_group->get_upr()*Utils::GetBasePeriod(),1,1);
			for(uint16	i=0;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_SYNC)=Atom::Float(View::SYNC_ONCE);
				view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
				view->object=copy;
				_Mem::Get()->inject(view);
				if(i==0){
					
					primary_view=view;
					if(_acquire_models)
						_Mem::Get()->inject_null_program(new	HASTController(this,view,input_fact),output_group,output_group->get_upr()*Utils::GetBasePeriod(),true);
				}
			}//std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" AF sync hold "<<input_fact->get_oid()<<"|"<<copy->get_oid()<<std::endl;
			break;
		}case	View::SYNC_AXIOM:		// inject a copy, sync_once, res=1, fact.before=next output_grp upr.
			if(input_fact->is_anti_fact())
				copy=new	AntiFact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			else
				copy=new	Fact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			for(uint16	i=0;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_SYNC)=Atom::Float(View::SYNC_ONCE_AXIOM);
				view->code(VIEW_RES)=Atom::Float(1);
				view->object=copy;
				_Mem::Get()->inject(view);
				if(i==0)
					primary_view=view;
			}
			break;
		}

		return	primary_view;
	}

	inline	void	AutoFocusController::notify(_Fact	*target,View	*input,TPXMap	&map){

		TPXMap::const_iterator	m=map.find(target);
		if(m!=map.end()){	// shall always be the case.

			m->second->signal(input);	// will spawn a ReductionJob holding a P<> on m->second.
			map.erase(m);
		}
	}

	inline	void	AutoFocusController::dispatch_pred_success(_Fact	*predicted_f,TPXMap	&map){

		TPXMap::const_iterator	m;
		for(m=map.begin();m!=map.end();++m)
			m->second->ack_pred_success(predicted_f);
	}

	inline	void	AutoFocusController::dispatch(View	*input,_Fact	*abstract_input,BindingMap	*bm,bool	&injected,TPXMap	&map){

		TPXMap::const_iterator	m;
		for(m=map.begin();m!=map.end();++m){

			if(m->second->take_input(input,abstract_input,bm)){

				if(!injected){

					injected=true;
					inject_input(input,abstract_input,bm);
				}
			}
		}
	}

	inline	void	AutoFocusController::dispatch_no_inject(View	*input,_Fact	*abstract_input,BindingMap	*bm,TPXMap	&map){

		TPXMap::const_iterator	m;
		for(m=map.begin();m!=map.end();++m)
			m->second->take_input(input,abstract_input,bm);
	}

	inline	void	AutoFocusController::rate(_Fact	*target,bool	success,TPXMap	&map,RatingMap	&ratings){
/*
		TPXMap::iterator	m=map.find(target);
		if(m!=map.end()){	// shall always be the case.

			_Fact	*pattern=m->second->get_pattern();
			RatingMap::iterator	r=ratings.find(pattern);
			if(r!=ratings.end()){	// shall always be the case.

				r->second.add_evidence(success);
				if(Rating::DSR(r->second.dSR))	// target for which we don't see much improvement over time.
					m->second=new	TPX(m->second);
			}
		}*/
	}

	void	AutoFocusController::take_input(r_exec::View	*input){

		if(is_invalidated())
			return;
		if(	input->object->code(0).asOpcode()==Opcodes::Fact	||
			input->object->code(0).asOpcode()==Opcodes::AntiFact	||
			input->object->code(0).asOpcode()==Opcodes::MkRdx)	// discard everything but facts, |facts and mk.rdx.
			Controller::__take_input<AutoFocusController>(input);// std::cout<<"A/F::TI: "<<get_host()->get_oid()<<" > "<<input->object->get_oid()<<std::endl;
	}

	void	AutoFocusController::reduce(r_exec::View	*input){

		Code	*input_object=input->object;
		uint16	opcode=input_object->code(0).asOpcode();

		reductionCS.enter();

		if(opcode==Opcodes::MkRdx){
			
			Code	*production=input_object->get_reference(MK_RDX_MDL_PRODUCTION_REF);	// fact, if an ihlp was the producer.
			Fact	*f_ihlp=(Fact	*)input_object->get_reference(MK_RDX_IHLP_REF);
			BindingMap	*bm=((MkRdx	*)input_object)->bindings;
			if(f_ihlp->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl){	// handle new goals/predictions as new targets.

				Code	*mdl=f_ihlp->get_reference(0)->get_reference(0);
				Code	*unpacked_mdl=mdl->get_reference(mdl->references_size()-MDL_HIDDEN_REFS);
				uint16	obj_set_index=unpacked_mdl->code(MDL_OBJS).asIndex();

				_Fact	*pattern;
				TPX		*tpx;
				Goal	*goal=((_Fact	*)production)->get_goal();
				if(goal!=NULL){	// build a tpx to find models like M:[A -> B] where B is the goal target.

					pattern=(_Fact	*)unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index+1).asIndex());	// lhs.
					tpx=build_tpx<GTPX>(goal->get_target(),pattern,bm,goal_ratings,f_ihlp,f_ihlp->get_reference(0)->code(I_HLP_WR_E).asBoolean());
					goals.insert(std::pair<P<Code>,P<TPX>	>((_Fact	*)production,tpx));std::cout<<Utils::RelativeTime(Now())<<" new GTPX\n";
				}else{	
					
					Pred	*pred=((_Fact	*)production)->get_pred();
					if(pred!=NULL){	// build a tpx to find models like M:[A -> |imdl M0] where M0 is the model that produced the prediction.

						pattern=(_Fact	*)unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index+2).asIndex());	// rhs.
						tpx=build_tpx<PTPX>((_Fact	*)f_ihlp,pattern,bm,prediction_ratings,f_ihlp,f_ihlp->get_reference(0)->code(I_HLP_WR_E).asBoolean());
						predictions.insert(std::pair<P<Code>,P<TPX>	>((_Fact	*)production,tpx));
					}
				}
			}
		}else{	
			
			bool	success=(opcode==Opcodes::Fact);
			if(success	||	opcode==Opcodes::AntiFact){	// discard everything but facts.

				Code	*payload=input_object->get_reference(0);
				uint16	opcode=payload->code(0).asOpcode();
				if(opcode==Opcodes::Success){	// input_object is f->success->payload, where payload is f->g or f->p; trim down the target list, rate targets, signal tpx. 

					_Fact	*target=(_Fact	*)payload->get_reference(0);
					Goal	*goal=target->get_goal();
					if(goal!=NULL){

						//rate(target,success,goals,goal_ratings);
						notify(target,input,goals);std::cout<<Utils::RelativeTime(Now())<<" notify & delete GTPX\n";
					}else{	// prediction.

						//rate(target,success,predictions,prediction_ratings);
						notify(target,input,predictions);
						if(success)	// a mdl has correctly predicted a GTPX's target: the GTPX shall not produce anything: we need to pass the prediction to all GTPX.
							dispatch_pred_success((_Fact	*)target->get_pred()->get_reference(0),goals);
					}
				}else	if(opcode==Opcodes::Perf)
					inject_input(input,2);	// inject in all output groups but the primary and secondary.
				else{	// filter according to targets: inject (once) when possible and pass to TPX if any.

					if(_pass_through)
						inject_input(input);
					else{

						P<BindingMap>	bm=new	BindingMap();
						if(opcode==Opcodes::ICst){	// dispatch but don't inject again (since it comes from inside).

							bm=((ICST	*)payload)->bindings;
							_Fact	*abstract_f_ihlp=bm->abstract_f_ihlp((_Fact	*)input_object);
							dispatch_no_inject(input,abstract_f_ihlp,bm,goals);
							dispatch_no_inject(input,abstract_f_ihlp,bm,predictions);
							cross_buffer.push_back(Input(input,abstract_f_ihlp,bm));
						}else{

							P<_Fact>	abstract_input=(_Fact	*)bm->abstract_object(input_object,false);
							bool		injected=false;
							dispatch(input,abstract_input,bm,injected,goals);
							dispatch(input,abstract_input,bm,injected,predictions);
						}
					}
				}
			}
		}

		reductionCS.leave();
	}

	void	AutoFocusController::inject_hlps(const	std::vector<P<Code> >	&hlps)	const{	// inject in the primary group; models will be injected in the secondary group automatically.

		std::vector<View	*>	views;
		
		uint64	now=Now();

		std::vector<P<Code> >::const_iterator	hlp;
		for(hlp=hlps.begin();hlp!=hlps.end();++hlp){

			View	*view=new	View(View::SYNC_ONCE,now,0,-1,output_groups[0],NULL,*hlp,1);	// SYNC_ONCE,sln=0,res=forever,act=1.
			view->references[0]=output_groups[0];
			views.push_back(view);
		}
		
		_Mem::Get()->inject_hlps(views,output_groups[0]);
	}

	void	AutoFocusController::copy_cross_buffer(r_code::list<Input>	&destination){	// copy inputs so they can be flagged independently by the tpxs that share the cross buffer.

		reductionCS.enter();
		time_buffer<Input>::iterator	i;
		for(i=cross_buffer.begin(Now());i!=cross_buffer.end();++i)
			destination.push_back(Input(*i));
		reductionCS.leave();
	}
}