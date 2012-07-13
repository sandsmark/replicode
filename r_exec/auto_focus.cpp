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
	}

	AutoFocusController::~AutoFocusController(){
	}

	Code	*AutoFocusController::get_core_object()	const{

		return	getObject();	// icpp_pgm.
	}

	inline	void	AutoFocusController::inject_input(View	*input,uint32	start)	const{

		_Fact	*input_fact=(_Fact	*)input->object;

		Group	*origin=input->get_host();
		Group	*ref_group=output_groups[0];

		uint64	now=Now();

		_Fact	*copy;
		switch(input->get_sync()){
		case	View::SYNC_ONCE:		// no copy, morph res; N.B.: cmds are sync_once.
			for(uint16	i=start;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
				_Mem::Get()->inject(view);
			}
			break;
		case	View::SYNC_PERIODIC:	// inject a copy, morph res, add a controller.
			if(input_fact->is_anti_fact())
				copy=new	AntiFact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			else
				copy=new	Fact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			for(uint16	i=start;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
				view->object=copy;
				_Mem::Get()->inject(view);
				if(i==0	&&	_acquire_models)
					_Mem::Get()->inject_null_program(new	PASTController(this,copy),output_group,output_group->get_upr()*Utils::GetBasePeriod(),true);
			}
			break;
		case	View::SYNC_HOLD:{		// inject a copy, add a controller, sync_once, morph res, after=now+time_tolerance (de-sync as it can have the same effect as a cmd), before=now+output_grp.upr+time_tolerance.
			uint64	offset=2*Utils::GetTimeTolerance();
			if(input_fact->is_anti_fact())
				copy=new	AntiFact(input_fact->get_reference(0),now+offset,now+offset+ref_group->get_upr()*Utils::GetBasePeriod(),1,1);
			else
				copy=new	Fact(input_fact->get_reference(0),now+offset,now+offset+ref_group->get_upr()*Utils::GetBasePeriod(),1,1);
			for(uint16	i=start;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_SYNC)=Atom::Float(View::SYNC_ONCE);
				view->code(VIEW_RES)=Atom::Float(Utils::GetResilience(view->code(VIEW_RES).asFloat(),origin->get_upr(),output_group->get_upr()));
				//Utils::SetTimestamp<View>(view,VIEW_IJT,now+offset);
				view->object=copy;
				_Mem::Get()->inject(view);
				if(i==0	&&	_acquire_models)
					_Mem::Get()->inject_null_program(new	HASTController(this,copy),output_group,output_group->get_upr()*Utils::GetBasePeriod(),true);
			}
			break;
		}case	View::SYNC_AXIOM:		// inject a copy, sync_once, res=1, fact.before=next output_grp upr.
			if(input_fact->is_anti_fact())
				copy=new	AntiFact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			else
				copy=new	Fact(input_fact->get_reference(0),ref_group->get_prev_upr_time(now),ref_group->get_next_upr_time(now),1,1);
			for(uint16	i=start;i<output_groups.size();++i){

				Group	*output_group=output_groups[i];
				View	*view=new	View(input,true);
				view->references[0]=output_group;
				view->code(VIEW_SYNC)=Atom::Float(View::SYNC_ONCE);
				view->code(VIEW_RES)=Atom::Float(1);
				view->object=copy;
				_Mem::Get()->inject(view);
			}
			break;
		}
	}

	inline	void	AutoFocusController::notify(_Fact	*target,View	*input,TPXMap	&map){

		TPXMap::const_iterator	m=map.find(target);
		if(m!=map.end()){	// shall always be the case.

			if(m->first->is_invalidated()){

				map.erase(m);
				return;
			}

			m->second->signal(input);	// will spawn a ReductionJob holding a P<> on m->second.
			map.erase(m);
		}
	}

	inline	void	AutoFocusController::notify_dispatch(_Fact	*target,View	*input){

		BindingMap	*bm=NULL;
		P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(input->object,bm);

		TPXMap::const_iterator	m;
		for(m=predictions.begin();m!=predictions.end();){

			if(m->first->is_invalidated())
				m=predictions.erase(m);
			else	if(m->first==target){

				m->second->signal(input);	// will spawn a ReductionJob holding a P<> on m->second.
				m=predictions.erase(m);
			}else{

				Input	*_input=new	Input(input->object,abstract_input,bm);
				m->second->take_input(_input);
				++m;
			}
		}

		dispatch_no_inject((_Fact	*)input->object,abstract_input,bm,goals);
	}

	inline	void	AutoFocusController::dispatch(View	*input,_Fact	*abstract_input,BindingMap	*bm,bool	&injected,TPXMap	&map){

		TPXMap::const_iterator	m;
		for(m=map.begin();m!=map.end();++m){

			if(m->first->is_invalidated())
				m=map.erase(m);
			else{

				Input	*_input=new	Input((_Fact	*)input->object,abstract_input,bm);
				if(m->second->take_input(_input)){

					if(!injected){

						inject_input(input,0);
						injected=true;
					}
				}
				++m;
			}
		}
	}

	inline	void	AutoFocusController::dispatch_no_inject(_Fact	*input,_Fact	*abstract_input,BindingMap	*bm,TPXMap	&map){

		TPXMap::const_iterator	m;
		for(m=map.begin();m!=map.end();){

			if(m->first->is_invalidated())
				m=map.erase(m);
			else{

				Input	*_input=new	Input(input,abstract_input,bm);
				m->second->take_input(_input);
				++m;
			}
		}
	}

	inline	void	AutoFocusController::dispatch(_Fact	*input,_Fact	*abstract_input,BindingMap	*bm,bool	&injected,TPXMap	&map){

		View	*view=new	View(View::SYNC_ONCE,Now(),1,1,NULL,NULL,input);	// groups are set in inject_input().
		dispatch(view,abstract_input,bm,injected,map);
	}

	inline	void	AutoFocusController::rate(_Fact	*target,bool	success,TPXMap	&map,RatingMap	&ratings){

		TPXMap::iterator	m=map.find(target);
		if(m!=map.end()){	// shall always be the case.

			if(m->first->is_invalidated()){

				map.erase(m);
				return;
			}

			_Fact	*pattern=m->second->get_pattern();
			RatingMap::iterator	r=ratings.find(pattern);
			if(r!=ratings.end()){	// shall always be the case.

				r->second.add_evidence(success);
				if(Rating::DSR(r->second.dSR))	// target for which we don't see much improvement over time.
					m->second=new	TPX(m->second);
			}
		}
	}

	void	AutoFocusController::take_input(r_exec::View	*input){

		if(is_invalidated())
			return;

		if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
			input->object->code(0).asOpcode()!=Opcodes::AntiFact)	// discard everything but facts and |facts.
			return;	// std::cout<<"A/F::TI: "<<get_host()->get_oid()<<" > "<<input->object->get_oid()<<std::endl;
		Controller::__take_input<AutoFocusController>(input);
	}
	void	AutoFocusController::reduce(r_exec::View	*input){

		Code	*input_object=input->object;
		uint16	opcode=input_object->code(0).asOpcode();

		reductionCS.enter();

		if(opcode==Opcodes::MkRdx){
			
			Code	*production=input_object->get_reference(MK_RDX_MDL_PRODUCTION_REF);		// fact, if an ihlp was the producer.
			_Fact	*f_ihlp=(_Fact	*)input_object->get_reference(MK_RDX_IHLP_REF);
			BindingMap	*bm=((MkRdx	*)input_object)->bindings;
			if(f_ihlp->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl){	// handle new goals/predictions as new targets.

				Code	*mdl=f_ihlp->get_reference(0)->get_reference(0);
				Code	*unpacked_mdl=mdl->get_reference(mdl->references_size()-MDL_HIDDEN_REFS);
				uint16	obj_set_index=unpacked_mdl->code(MDL_OBJS).asIndex();

				_Fact	*pattern;
				TPX		*tpx;
				Goal	*goal=((_Fact	*)production)->get_goal();
				if(goal!=NULL){

					pattern=(_Fact	*)unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index+1).asIndex());	// lhs.
					tpx=build_tpx<GTPX>((_Fact	*)production,pattern,bm,goal_ratings,f_ihlp->get_reference(0)->code(I_HLP_WR_E).asBoolean());
					goals.insert(std::pair<P<Code>,P<TPX>	>((_Fact	*)production,tpx));
				}else{	
					
					Pred	*pred=((_Fact	*)production)->get_pred();
					if(pred!=NULL){

						pattern=(_Fact	*)unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index+2).asIndex());	// rhs.
						tpx=build_tpx<PTPX>((_Fact	*)production,pattern,bm,prediction_ratings,f_ihlp->get_reference(0)->code(I_HLP_WR_E).asBoolean());
						predictions.insert(std::pair<P<Code>,P<TPX>	>((_Fact	*)production,tpx));
					}
				}
			}
		}else	if(bool	success=(opcode==Opcodes::Fact)	||	opcode==Opcodes::AntiFact){	// discard everything but facts.

			Code	*payload=input_object->get_reference(0);
			uint16	opcode=payload->code(0).asOpcode();
			if(opcode==Opcodes::Success){	// input_object is f->success->payload, where payload is f->g or f->p; trim down the target list, rate targets, signal tpx. 

				_Fact	*target=(_Fact	*)payload->get_reference(0);
				Goal	*goal=target->get_goal();
				if(goal!=NULL){

					rate(target,success,goals,goal_ratings);
					notify(target,input,goals);
				}else{	// prediction.

					rate(target,success,predictions,prediction_ratings);
					if(success)
						notify_dispatch(target,input);	// reason: success->p->t and t==tpx->target means a model can solve the goal/failure of prediction.
					else
						notify(target,input,predictions);
				}
			}else	if(opcode==Opcodes::Perf)
				inject_input(input,2);	// inject in all output groups but the primary and secondary.
			else{	// filter according to targets: inject (once) when possible and pass to TPX if any.

				if(_pass_through)
					inject_input(input,0);
				else{

					BindingMap	*bm;
					if(opcode==Opcodes::ICst){	// dispatch but don't inject again (since it comes from inside).

						bm=((ICST	*)payload)->bindings;
						_Fact	*abstract_f_ihlp=bm->abstract_f_ihlp((_Fact	*)input_object);
						dispatch_no_inject((_Fact	*)input_object,abstract_f_ihlp,bm,goals);
						dispatch_no_inject((_Fact	*)input_object,abstract_f_ihlp,bm,predictions);
					}else{

						P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(input_object,bm);
						bool		injected=false;
						dispatch(input,abstract_input,bm,injected,goals);
						dispatch(input,abstract_input,bm,injected,predictions);
					}
				}
			}
		}

		reductionCS.leave();
	}

	void	AutoFocusController::inject_hlps(const	std::list<P<Code> >	&hlps)	const{	// inject in the primary group; models will be injected in the secondary group automatically.

		std::list<View	*>	views;
		
		uint64	now=Now();

		std::list<P<Code> >::const_iterator	hlp;
		for(hlp=hlps.begin();hlp!=hlps.end();++hlp){

			View	*view=new	View(View::SYNC_ONCE,now,0,-1,output_groups[0],NULL,*hlp,1);	// SYNC_ONCE,sln=0,res=forever,act=1.
			view->references[0]=output_groups[0];
			views.push_back(view);
		}
		
		_Mem::Get()->inject_hlps(views,output_groups[0]);
	}
}