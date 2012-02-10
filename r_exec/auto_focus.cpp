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


namespace	r_exec{

	AutoFocusController::AutoFocusController(r_code::View	*view):Controller(view){

		// Load arguments: pass_through, list of output groups: 1st must be the primary, 2nd the secondary, then other groups.
		Code	*icpp_pgm=getObject();
		uint16	arg_set_index=icpp_pgm->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=icpp_pgm->code(arg_set_index).getAtomCount();
		pass_through=icpp_pgm->code(arg_set_index+1).asBoolean();
		for(uint16	i=1;i<arg_count;++i)
			output_groups.push_back((Group	*)icpp_pgm->get_reference(i-1));
	}

	AutoFocusController::~AutoFocusController(){
	}

	Code	*AutoFocusController::get_core_object()	const{

		return	getObject();	// icpp_pgm.
	}

	inline	void	AutoFocusController::inject_input(View	*input,uint32	start)	const{

		for(uint16	i=start;i<output_groups.size();++i){

			Group	*output_group=output_groups[i];
			View	*_view=new	View(input,true);
			if(!_view->get_sync()){	// SYNC_STATE: inject with sync_front, res=1, fact::before=next upr.

				//uint64	now=Now();
				//Code	*input_fact=_view->object;
				//Utils::SetTimestamp<Code>(input_fact,FACT_BEFORE,output_group->get_time_at_next_upr(now));
				_view->code(VIEW_SYNC)=Atom::Boolean(true);
			}
			_view->code(VIEW_RES)=Atom::Float(1);
			_view->references[0]=output_group;
			input->object->views.insert(_view);
			output_groups[i]->inject(_view,0);
		}
	}

	inline	void	AutoFocusController::notify(_Fact	*target,View	*input,TPXMap	&map){

		TPXMap::const_iterator	m=map.find(target);
		if(m!=map.end()){	// shall always be the case.

			m->second->signal(input);	// will spawn a ReductionJob holding a P<> on m->second.
			map.erase(m);
		}
	}

	inline	void	AutoFocusController::notify_dispatch(_Fact	*target,View	*input){

		BindingMap	*bm=NULL;
		P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(input->object,bm);

		TPXMap::const_iterator	m;
		for(m=predictions.begin();m!=predictions.end();){

			if(m->first==target){

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

			Input	*_input=new	Input((_Fact	*)input->object,abstract_input,bm);
			if(m->second->take_input(_input)){

				if(!injected){

					inject_input(input,0);
					injected=true;
				}
			}
		}
	}

	inline	void	AutoFocusController::dispatch_no_inject(_Fact	*input,_Fact	*abstract_input,BindingMap	*bm,TPXMap	&map){

		TPXMap::const_iterator	m;
		for(m=map.begin();m!=map.end();++m){

			Input	*_input=new	Input(input,abstract_input,bm);
			m->second->take_input(_input);
		}
	}

	inline	void	AutoFocusController::dispatch(_Fact	*input,_Fact	*abstract_input,BindingMap	*bm,bool	&injected,TPXMap	&map){

		View	*view=new	View(true,Now(),1,1,NULL,NULL,input);	// groups are set in inject_input().
		dispatch(view,abstract_input,bm,injected,map);
	}

	inline	void	AutoFocusController::rate(_Fact	*target,bool	success,TPXMap	&map,RatingMap	&ratings){

		TPXMap::iterator	m=map.find(target);
		if(m!=map.end()){	// shall always be the case.

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

		Code	*input_object=input->object;
		uint16	opcode=input_object->code(0).asOpcode();
		if(opcode==Opcodes::MkRdx){
			
			Code	*production=input_object->get_reference(MK_RDX_MDL_PRODUCTION_REF);		// fact, if an ihlp was the producer.
			_Fact	*f_ihlp=(_Fact	*)input_object->get_reference(MK_RDX_IHLP_REF);
			BindingMap	*bm=((MkRdx	*)input_object)->bindings;
			if(f_ihlp->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl){	// handle new goals/predictions as new targets.

				Code	*mdl=f_ihlp->get_reference(0)->get_reference(0);
				Code	*unpacked_mdl=mdl->get_reference(mdl->references_size()-1);
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

				if(pass_through)
					inject_input(input,0);
				else{

					BindingMap	*bm;
					if(opcode==Opcodes::ICst){	// dispatch but don't inject again (since it comes from inside).

						bm=((ICST	*)payload)->bindings;
						_Fact	*abstract_f_ihlp=bm->abstract_f_ihlp((_Fact	*)input_object);
						dispatch_no_inject((_Fact	*)input_object,abstract_f_ihlp,bm,goals);
						dispatch_no_inject((_Fact	*)input_object,abstract_f_ihlp,bm,predictions);
					}else{

						P<_Fact>		abstract_input=(_Fact	*)BindingMap::Abstract(input_object,bm);
						bool			injected=false;
						dispatch(input,abstract_input,bm,injected,goals);
						dispatch(input,abstract_input,bm,injected,predictions);
					}
				}
			}
		}
	}

	void	AutoFocusController::inject_hlp(Code	*hlp)	const{	// inject in the primary group; models will be injected in the secondary group automatically.

		View	*view=new	View(true,Now(),0,-1,output_groups[0],NULL,hlp,1);	// SYNC_FRONT,sln=0,res=forever,act=1.
		hlp->views.insert(view);
		output_groups[0]->inject(view,0);
	}
}