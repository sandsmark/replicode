//	mdl_controller.cpp
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

#include	"mdl_controller.h"
#include	"mem.h"


namespace	r_exec{

	MDLOverlay::MDLOverlay(Controller	*c,const	BindingMap	*bindings):HLPOverlay(c,bindings){
	}

	MDLOverlay::~MDLOverlay(){
	}

	void	MDLOverlay::load_patterns(){

		patterns.push_back(((MDLController	*)controller)->get_lhs());
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PrimaryMDLOverlay::PrimaryMDLOverlay(Controller	*c,const	BindingMap	*bindings):MDLOverlay(c,bindings){
	}

	PrimaryMDLOverlay::~PrimaryMDLOverlay(){
	}

	Overlay	*PrimaryMDLOverlay::reduce(r_exec::View *input){
//std::cout<<std::hex<<this<<std::dec<<" "<<input->object->get_oid();
		_Fact	*input_object;
		Pred	*prediction=((Fact	*)input->object)->get_pred();
		bool	simulation;
		if(prediction){

			input_object=prediction->get_target();
			simulation=prediction->is_simulation();
		}else{

			input_object=(_Fact	*)input->object;
			simulation=false;
		}

		P<BindingMap>	bm=new	BindingMap(bindings);
		bm->reset_fwd_timings(input_object);
		switch(bm->match_lenient(input_object,((MDLController	*)controller)->get_lhs(),MATCH_FORWARD)){
		case	MATCH_SUCCESS_POSITIVE:{

			load_code();
			P<BindingMap>	original_bindings=bindings;
			bindings=bm;
			bool	is_req=((MDLController	*)controller)->is_requirement();
			Overlay				*o;
			Fact				*f_imdl=((MDLController	*)controller)->get_f_ihlp(bm,true);//bm->trace();f_imdl->get_reference(0)->trace();
			RequirementsPair	r_p;
			ChainingStatus		c_s=((MDLController	*)controller)->retrieve_imdl_fwd(bm,f_imdl,r_p);
			bool				c_a=(c_s>=WR_ENABLED);
			switch(c_s){
			case	WR_DISABLED:
			case	SR_DISABLED_NO_WR:	// silent monitoring of a prediction that will not be injected.
				if(simulation){	// if there is simulated imdl for the root of one sim in prediction, allow forward chaining.

					if(check_simulated_chaining(bm,f_imdl,prediction))
						c_a=true;
					else{

						o=NULL;
						break;
					}
				}
			case	NO_R:
				if(f_imdl->get_reference(0)->code(f_imdl->get_reference(0)->code(I_HLP_TPL_ARGS).asIndex()).getAtomCount()>0){	// there are tpl args, abort.

					o=NULL;
					break;
				}else
					f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(false);
			case	SR_DISABLED_WR:		// silent monitoring of a prediction that will not be injected.
				if(simulation){	// if there is simulated imdl for the root of one sim in prediction, allow forward chaining.

					if(check_simulated_chaining(bm,f_imdl,prediction))
						c_a=true;
					else{

						o=NULL;
						break;
					}
				}
			case	WR_ENABLED:
				if(evaluate_fwd_guards()){	// may update bindings.
//std::cout<<" match\n";
					f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.
					//f_imdl->get_reference(0)->trace();
					((PrimaryMDLController	*)controller)->predict(bindings,input_object,f_imdl,c_a,r_p);
					o=this;
				}else{
//std::cout<<" guards failed\n";
					o=NULL;
				}
				break;
			}
			// reset.
			delete[]	code;
			code=NULL;
			bindings=original_bindings;
			if(prediction){

				if(!simulation)
					((PrimaryMDLController	*)controller)->store_predicted_evidence(input->object);
			}else{//std::cout<<"CACHED: "<<Now()<<" : "<<Utils::GetTimestamp<Code>(input->object,FACT_BEFORE)<<std::endl;
				((PrimaryMDLController	*)controller)->store_evidence(input->object);}
			return	o;
		}case	MATCH_SUCCESS_NEGATIVE:
			if(prediction){

				if(!simulation)
					((PrimaryMDLController	*)controller)->store_predicted_evidence(input->object);
			}else
				((PrimaryMDLController	*)controller)->store_evidence(input->object);
		case	MATCH_FAILURE:
//std::cout<<" no match\n";
			return	NULL;
		}
	}

	bool	PrimaryMDLOverlay::check_simulated_chaining(BindingMap	*bm,Fact	*f_imdl,Pred	*prediction){

		for(uint32	i=0;i<prediction->simulations.size();++i){

			switch(((MDLController	*)controller)->retrieve_simulated_imdl_fwd(bm,f_imdl,prediction->simulations[i]->root)){
			case	NO_R:
			case	WR_ENABLED:
				return	true;
			default:
				break;
			}
		}

		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SecondaryMDLOverlay::SecondaryMDLOverlay(Controller	*c,const	BindingMap	*bindings):MDLOverlay(c,bindings){
	}

	SecondaryMDLOverlay::~SecondaryMDLOverlay(){
	}

	Overlay	*SecondaryMDLOverlay::reduce(r_exec::View *input){
//std::cout<<std::hex<<this<<std::dec<<" "<<input->object->get_oid();
		_Fact	*input_object=(_Fact	*)input->object;

		P<BindingMap>	bm=new	BindingMap(bindings);
		bm->reset_fwd_timings(input_object);
		switch(bm->match_lenient(input_object,((MDLController	*)controller)->get_lhs(),MATCH_FORWARD)){
		case	MATCH_SUCCESS_POSITIVE:{

			load_code();
			P<BindingMap>		original_bindings=bindings;
			bindings=bm;
			Overlay				*o;
			Fact				*f_imdl=((MDLController	*)controller)->get_f_ihlp(bm,true);
			RequirementsPair	r_p;
			ChainingStatus		c_s=((MDLController	*)controller)->retrieve_imdl_fwd(bm,f_imdl,r_p);
			bool				c_a=(c_s>=NO_R);
			switch(c_s){
			case	WR_DISABLED:
			case	SR_DISABLED_NO_WR:	// silent monitoring of a prediction that will not be injected.
			case	NO_R:
				if(f_imdl->get_reference(0)->code(f_imdl->get_reference(0)->code(I_HLP_TPL_ARGS).asIndex()).getAtomCount()>0){	// there are tpl args, abort.

					o=NULL;
					break;
				}else
					f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(false);
			case	SR_DISABLED_WR:
			case	WR_ENABLED:
				if(evaluate_fwd_guards()){	// may update bindings.
//std::cout<<" match\n";
					f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.
					((SecondaryMDLController	*)controller)->predict(bindings,input->object,NULL,true,r_p);
					o=this;
				}else{
//std::cout<<" guards failed\n";
					o=NULL;
				}
				break;
			}
			// reset.
			delete[]	code;
			code=NULL;
			bindings=original_bindings;
			return	o;
		}case	MATCH_SUCCESS_NEGATIVE:
		case	MATCH_FAILURE:
//std::cout<<" no match\n";
			return	NULL;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MDLController	*MDLController::New(View	*view,bool	&inject_in_secondary_group){

		Code	*unpacked_mdl=view->object->get_reference(view->object->references_size()-1);
		uint16	obj_set_index=unpacked_mdl->code(MDL_OBJS).asIndex();
		Code	*rhs=unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index+2).asIndex());

		if(rhs->get_reference(0)->code(0).asOpcode()==Opcodes::Ent){	// rhs is a drive.

			inject_in_secondary_group=false;
			return	new	TopLevelMDLController(view);
		}

		inject_in_secondary_group=true;
		return	new	PrimaryMDLController(view);
	}

	MDLController::MDLController(r_code::View	*view):HLPController(view){

		Code	*object=get_unpacked_object();
		uint16	obj_set_index=object->code(MDL_OBJS).asIndex();
		lhs=object->get_reference(object->code(obj_set_index+1).asIndex());
		rhs=object->get_reference(object->code(obj_set_index+2).asIndex());

		_is_requirement=(rhs->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl);
	}

	HLPController	*MDLController::get_rhs_controller(bool	&strong)	const{

		Code	*rhs_ihlp=rhs->get_reference(0);
		uint16	opcode=rhs_ihlp->code(0).asOpcode();
		if(	opcode==Opcodes::ICst	||
			opcode==Opcodes::IMdl){

			Code			*rhs_hlp=rhs_ihlp->get_reference(0);
			Group			*host=getView()->get_host();
			r_exec::View	*rhs_hlp_v=(r_exec::View*)rhs_hlp->find_view(host,true);
			if(rhs_hlp_v){

				strong=rhs->code(0).asOpcode()==Opcodes::AntiFact;
				return	(HLPController	*)rhs_hlp_v->controller;
			}
		}
		return	NULL;
	}

	float32	MDLController::get_cfd()	const{

		return	get_core_object()->code(MDL_SR).asFloat();
	}

	void	MDLController::monitor_predictions(_Fact	*input){	// predictions are admissible inputs (for checking predicted counter-evidences).
		
		std::list<P<PMonitor> >::const_iterator	m;
		p_monitorsCS.enter();
		for(m=p_monitors.begin();m!=p_monitors.end();){

			if((*m)->reduce(input))
				m=p_monitors.erase(m);
			else
				++m;
		}
		p_monitorsCS.leave();
	}

	void	MDLController::add_monitor(PMonitor	*m){

		p_monitorsCS.enter();
		p_monitors.push_front(m);
		p_monitorsCS.leave();
	}

	void	MDLController::remove_monitor(PMonitor	*m){

		p_monitorsCS.enter();
		p_monitors.remove(m);
		p_monitorsCS.leave();
	}

	inline	_Fact	*MDLController::get_lhs()	const{

		return	lhs;
	}

	inline	_Fact	*MDLController::get_rhs()	const{

		return	rhs;
	}

	inline	Fact	*MDLController::get_f_ihlp(const	BindingMap	*bindings,bool	wr_enabled)	const{

		return	bindings->build_f_ihlp(getObject(),Opcodes::IMdl,wr_enabled);
	}

	void	MDLController::_store_requirement(std::list<REntry>	*cache,REntry	&e){

		requirements.CS.enter();
		uint64	now=Now();
		std::list<REntry>::const_iterator	_e;
		for(_e=cache->begin();_e!=cache->end();){

			if((*_e).evidence->is_invalidated()	||	(*_e).before<now)	// garbage collection.
				_e=cache->erase(_e);
			else
				++_e;
		}
		cache->push_front(e);
		requirements.CS.leave();
	}

	ChainingStatus	MDLController::retrieve_simulated_imdl_fwd(BindingMap	*bm,_Fact	*f_imdl,Controller	*root){

		uint32	wr_count;
		uint32	sr_count;
		uint32	r_count=get_requirement_count(wr_count,sr_count);
		if(!r_count)
			return	NO_R;
		ChainingStatus	r;
		if(!sr_count){	// no strong req., some weak req.: true if there is one f->imdl complying with timings and bindings.

			r=WR_DISABLED;
			requirements.CS.enter();
			uint64	now=Now();
			std::list<REntry>::const_iterator	e;
			for(e=simulated_requirements.positive_evidences.begin();e!=simulated_requirements.positive_evidences.end();){

				if((*e).is_too_old(now))	// garbage collection.
					e=simulated_requirements.positive_evidences.erase(e);
				else	if((*e).is_out_of_range(now))
					++e;
				else{

					if((*e).evidence->get_pred()->get_simulation(root)){

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						//_f_imdl->get_reference(0)->trace();
						//f_imdl->get_reference(0)->trace();
						if(bm->match_strict(_f_imdl,f_imdl,MATCH_BACKWARD)){	// tpl args will be valuated in bm, but not in f_imdl yet.

							r=WR_ENABLED;
							break;
						}
					}
					++e;
				}
			}

			requirements.CS.leave();
			return	r;
		}else{

			if(!wr_count){	// some strong req., no weak req.: true if there is no |f->imdl complying with timings and bindings.

				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=simulated_requirements.negative_evidences.begin();e!=simulated_requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=simulated_requirements.negative_evidences.erase(e);
					else	if((*e).is_out_of_range(now))
						++e;
					else{

						if((*e).evidence->get_pred()->get_simulation(root)){

							_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
							if(bm->match_lenient(_f_imdl,f_imdl,MATCH_BACKWARD)==MATCH_SUCCESS_NEGATIVE){	// tpl args will be valuated in bm.

								requirements.CS.leave();
								return	SR_DISABLED_NO_WR;
							}
						}
						++e;
					}
				}

				requirements.CS.leave();
				return	WR_ENABLED;
			}else{	// some strong req. and some weak req.: true if among the entries complying with timings and bindings, the youngest |f->imdl is weaker than the youngest f->imdl.

				r=WR_DISABLED;
				float32	negative_cfd=0;
				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=simulated_requirements.negative_evidences.begin();e!=simulated_requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=simulated_requirements.negative_evidences.erase(e);
					else	if((*e).is_out_of_range(now))
						++e;
					else{

						if((*e).evidence->get_pred()->get_simulation(root)){

							_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
							if(bm->match_lenient(_f_imdl,f_imdl,MATCH_BACKWARD)==MATCH_SUCCESS_NEGATIVE){

								negative_cfd=(*e).confidence;
								r=SR_DISABLED_NO_WR;
								break;
							}
						}
						++e;
					}
				}
				
				for(e=simulated_requirements.positive_evidences.begin();e!=simulated_requirements.positive_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=simulated_requirements.positive_evidences.erase(e);
					else	if((*e).is_out_of_range(now))
						++e;
					else{
	//(*e).f->get_reference(0)->trace();
	//f->get_reference(0)->trace();
						if((*e).evidence->get_pred()->get_simulation(root)){

							_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
							if(bm->match_strict(_f_imdl,f_imdl,MATCH_BACKWARD)){

								if((*e).confidence>=negative_cfd){

									r=WR_ENABLED;
									break;
								}else
									r=SR_DISABLED_WR;
							}
						}
						++e;
					}
				}
				
				requirements.CS.leave();
				return	r;
			}
		}
	}

	ChainingStatus	MDLController::retrieve_simulated_imdl_bwd(BindingMap	*bm,_Fact	*f_imdl,Controller	*root){

		uint32	wr_count;
		uint32	sr_count;
		uint32	r_count=get_requirement_count(wr_count,sr_count);
		if(!r_count)
			return	NO_R;
		ChainingStatus	r;
		if(!sr_count){	// no strong req., some weak req.: true if there is one f->imdl complying with timings and bindings.

			r=WR_DISABLED;
			requirements.CS.enter();
			uint64	now=Now();
			std::list<REntry>::const_iterator	e;
			for(e=simulated_requirements.positive_evidences.begin();e!=simulated_requirements.positive_evidences.end();){

				if((*e).is_too_old(now))	// garbage collection.
					e=simulated_requirements.positive_evidences.erase(e);
				else{

					if((*e).evidence->get_pred()->get_simulation(root)){

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						//_f_imdl->get_reference(0)->trace();
						//f_imdl->get_reference(0)->trace();
						if(bm->match_strict(_f_imdl,f_imdl,MATCH_BACKWARD)){	// tpl args will be valuated in bm, but not in f_imdl yet.

							r=WR_ENABLED;
							break;
						}
					}
					++e;
				}
			}

			requirements.CS.leave();
			return	r;
		}else{

			if(!wr_count){	// some strong req., no weak req.: true if there is no |f->imdl complying with timings and bindings.

				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=simulated_requirements.negative_evidences.begin();e!=simulated_requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=simulated_requirements.negative_evidences.erase(e);
					else{

						if((*e).evidence->get_pred()->get_simulation(root)){

							_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
							if(bm->match_lenient(_f_imdl,f_imdl,MATCH_BACKWARD)==MATCH_SUCCESS_NEGATIVE){	// tpl args will be valuated in bm.

								requirements.CS.leave();
								return	SR_DISABLED_NO_WR;
							}
						}
						++e;
					}
				}

				requirements.CS.leave();
				return	WR_ENABLED;
			}else{	// some strong req. and some weak req.: true if among the entries complying with timings and bindings, the youngest |f->imdl is weaker than the youngest f->imdl.

				r=WR_DISABLED;
				float32	negative_cfd=0;
				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=simulated_requirements.negative_evidences.begin();e!=simulated_requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=simulated_requirements.negative_evidences.erase(e);
					else{

						if((*e).evidence->get_pred()->get_simulation(root)){

							_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
							if(bm->match_lenient(_f_imdl,f_imdl,MATCH_BACKWARD)==MATCH_SUCCESS_NEGATIVE){

								negative_cfd=(*e).confidence;
								r=SR_DISABLED_NO_WR;
								break;
							}
						}
						++e;
					}
				}
				
				for(e=simulated_requirements.positive_evidences.begin();e!=simulated_requirements.positive_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=simulated_requirements.positive_evidences.erase(e);
					else{
	//(*e).f->get_reference(0)->trace();
	//f->get_reference(0)->trace();
						if((*e).evidence->get_pred()->get_simulation(root)){

							_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
							if(bm->match_strict(_f_imdl,f_imdl,MATCH_BACKWARD)){

								if((*e).confidence>=negative_cfd){

									r=WR_ENABLED;
									break;
								}else
									r=SR_DISABLED_WR;
							}
						}
						++e;
					}
				}
				
				requirements.CS.leave();
				return	r;
			}
		}
	}

	ChainingStatus	MDLController::retrieve_imdl_fwd(BindingMap	*bm,_Fact	*f_imdl,RequirementsPair	&r_p){

		uint32	wr_count;
		uint32	sr_count;
		uint32	r_count=get_requirement_count(wr_count,sr_count);
		if(!r_count)
			return	NO_R;
		ChainingStatus	r;
		BindingMap	original(bm);
		if(!sr_count){	// no strong req., some weak req.: true if there is one f->imdl complying with timings and bindings.

			r=WR_DISABLED;
			requirements.CS.enter();
			uint64	now=Now();
			std::list<REntry>::const_iterator	e;
			for(e=requirements.positive_evidences.begin();e!=requirements.positive_evidences.end();){

				if((*e).is_too_old(now))	// garbage collection.
					e=requirements.positive_evidences.erase(e);
				else	if((*e).is_out_of_range(now))
					++e;
				else{

					_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
					//_f_imdl->get_reference(0)->trace();
					//f_imdl->get_reference(0)->trace();
					BindingMap	_original=original;	// matching updates the bm; always start afresh.
					if(_original.match_strict(_f_imdl,f_imdl,MATCH_FORWARD)){	// tpl args will be valuated in bm, but not in f_imdl yet.

						if(r==WR_DISABLED	&&	(*e).chaining_was_allowed){	// first match.

							r=WR_ENABLED;
							bm->load(&_original);
						}
						
						r_p.first.controllers.push_back((*e).controller);
						r_p.first.f_imdl=_f_imdl;
						r_p.first.chaining_was_allowed=(*e).chaining_was_allowed;
					}
					++e;
				}
			}

			requirements.CS.leave();
			return	r;
		}else{

			if(!wr_count){	// some strong req., no weak req.: true if there is no |f->imdl complying with timings and bindings.

				r=WR_ENABLED;
				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=requirements.negative_evidences.begin();e!=requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=requirements.positive_evidences.erase(e);
					else	if((*e).is_out_of_range(now))
						++e;
					else{

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						BindingMap	_original=original;	// matching updates the bm; always start afresh.
						if(_original.match_lenient(_f_imdl,f_imdl,MATCH_FORWARD)==MATCH_SUCCESS_NEGATIVE){	// tpl args will be valuated in bm.

							if(r==WR_ENABLED	&&	(*e).chaining_was_allowed)	// first match.
								r=SR_DISABLED_NO_WR;

							r_p.second.controllers.push_back((*e).controller);
							r_p.second.f_imdl=_f_imdl;
							r_p.second.chaining_was_allowed=(*e).chaining_was_allowed;
						}
						++e;
					}
				}

				requirements.CS.leave();
				return	r;
			}else{	// some strong req. and some weak req.: true if among the entries complying with timings and bindings, the youngest |f->imdl is weaker than the youngest f->imdl.

				r=NO_R;
				requirements.CS.enter();
				float32	negative_cfd=0;
				uint64	now=Now();

				std::list<REntry>::const_iterator	e;
				for(e=requirements.negative_evidences.begin();e!=requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=requirements.negative_evidences.erase(e);
					else	if((*e).is_out_of_range(now))
						++e;
					else{

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						BindingMap	_original=original;	// matching updates the bm; always start afresh.
						if(_original.match_lenient(_f_imdl,f_imdl,MATCH_FORWARD)==MATCH_SUCCESS_NEGATIVE){

							if(r==NO_R	&&	(*e).chaining_was_allowed){	// first match.

								negative_cfd=(*e).confidence;
								r=SR_DISABLED_NO_WR;
							}

							r_p.second.controllers.push_back((*e).controller);
							r_p.second.f_imdl=_f_imdl;
							r_p.second.chaining_was_allowed=(*e).chaining_was_allowed;
						}
						++e;
					}
				}
				
				for(e=requirements.positive_evidences.begin();e!=requirements.positive_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=requirements.positive_evidences.erase(e);
					else	if((*e).is_out_of_range(now))
						++e;
					else{

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						BindingMap	_original=original;	// matching updates the bm; always start afresh.
						if(_original.match_strict(_f_imdl,f_imdl,MATCH_FORWARD)){

							if(r!=WR_ENABLED	&&	(*e).chaining_was_allowed){	// first siginificant match.
								
								if((*e).confidence>=negative_cfd)
									r=WR_ENABLED;
								else
									r=SR_DISABLED_WR;
								bm->load(&_original);
							}

							r_p.first.controllers.push_back((*e).controller);
							r_p.first.f_imdl=_f_imdl;
							r_p.first.chaining_was_allowed=(*e).chaining_was_allowed;
						}
						++e;
					}
				}
				requirements.CS.leave();
				return	r;
			}
		}
	}

	ChainingStatus	MDLController::retrieve_imdl_bwd(BindingMap	*bm,_Fact	*f_imdl,Fact	*&ground){

		uint32	wr_count;
		uint32	sr_count;
		uint32	r_count=get_requirement_count(wr_count,sr_count);
		if(!r_count){

			ground=NULL;
			return	NO_R;
		}

		ChainingStatus	r;
		if(!sr_count){	// no strong req., some weak req.: true if there is one f->imdl complying with timings and bindings.

			r=WR_DISABLED;
			requirements.CS.enter();
			uint64	now=Now();
			std::list<REntry>::const_iterator	e;
			for(e=requirements.positive_evidences.begin();e!=requirements.positive_evidences.end();){

				if((*e).is_too_old(now))	// garbage collection.
					e=requirements.positive_evidences.erase(e);
				else{

					_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
					//_f_imdl->get_reference(0)->trace();
					//f_imdl->get_reference(0)->trace();
					if(bm->match_strict(_f_imdl,f_imdl,MATCH_BACKWARD)){	// tpl args will be valuated in bm, but not in f_imdl yet.

						r=WR_ENABLED;
						ground=(*e).evidence;
						break;
					}
					++e;
				}
			}

			requirements.CS.leave();
			return	r;
		}else{

			if(!wr_count){	// some strong req., no weak req.: true if there is no |f->imdl complying with timings and bindings.

				ground=NULL;

				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=requirements.negative_evidences.begin();e!=requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=requirements.negative_evidences.erase(e);
					else{

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						if(bm->match_lenient(_f_imdl,f_imdl,MATCH_BACKWARD)==MATCH_SUCCESS_NEGATIVE){	// tpl args will be valuated in bm.

							requirements.CS.leave();
							return	SR_DISABLED_NO_WR;
						}
						++e;
					}
				}

				requirements.CS.leave();
				return	WR_ENABLED;
			}else{	// some strong req. and some weak req.: true if among the entries complying with timings and bindings, the youngest |f->imdl is weaker than the youngest f->imdl.

				r=WR_DISABLED;
				float32	negative_cfd=0;
				requirements.CS.enter();
				uint64	now=Now();
				std::list<REntry>::const_iterator	e;
				for(e=requirements.negative_evidences.begin();e!=requirements.negative_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=requirements.negative_evidences.erase(e);
					else{

						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						if(bm->match_lenient(_f_imdl,f_imdl,MATCH_BACKWARD)==MATCH_SUCCESS_NEGATIVE){

							negative_cfd=(*e).confidence;
							r=SR_DISABLED_NO_WR;
							break;
						}
						++e;
					}
				}
				
				for(e=requirements.positive_evidences.begin();e!=requirements.positive_evidences.end();){

					if((*e).is_too_old(now))	// garbage collection.
						e=requirements.positive_evidences.erase(e);
					else{
	//(*e).f->get_reference(0)->trace();
	//f->get_reference(0)->trace();
						_Fact	*_f_imdl=(*e).evidence->get_pred()->get_target();
						if(bm->match_strict(_f_imdl,f_imdl,MATCH_BACKWARD)){

							if((*e).confidence>=negative_cfd){

								r=WR_ENABLED;
								ground=(*e).evidence;
								break;
							}else
								r=SR_DISABLED_WR;
						}
						++e;
					}
				}
				
				requirements.CS.leave();
				return	r;
			}
		}
	}

	void	MDLController::register_requirement(_Fact	*f_pred,RequirementsPair	&r_p){

		if(r_p.first.controllers.size()>0	||	r_p.second.controllers.size()>0)
			active_requirements.insert(std::pair<P<Code>,RequirementsPair>(f_pred,r_p));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MDLController::REntry::REntry(_Fact	*f_p_f_imdl,MDLController	*c,bool	chaining_was_allowed):PEEntry(f_p_f_imdl),controller(c),chaining_was_allowed(chaining_was_allowed){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PMDLController::PMDLController(r_code::View	*view):MDLController(view){

		PrimaryMDLOverlay	*o=new	PrimaryMDLOverlay(this,bindings);	// master overlay.
		o->load_patterns();
		overlays.push_back(o);
	}

	void	PMDLController::add_monitor(_GMonitor	*m){

		g_monitorsCS.enter();
		g_monitors.push_front(m);
		g_monitorsCS.leave();
	}

	void	PMDLController::remove_monitor(_GMonitor	*m){

		g_monitorsCS.enter();
		g_monitors.remove(m);
		g_monitorsCS.leave();
	}

	void	PMDLController::add_monitor(RMonitor	*m){

		g_monitorsCS.enter();
		r_monitors.push_front(m);
		g_monitorsCS.leave();
	}

	void	PMDLController::remove_monitor(RMonitor	*m){

		g_monitorsCS.enter();
		r_monitors.remove(m);
		g_monitorsCS.leave();
	}

	void	PMDLController::add_monitor(SRMonitor	*m){

		g_monitorsCS.enter();
		sr_monitors.push_front(m);
		g_monitorsCS.leave();
	}

	void	PMDLController::remove_monitor(SRMonitor	*m){

		g_monitorsCS.enter();
		sr_monitors.remove(m);
		g_monitorsCS.leave();
	}

	void	PMDLController::inject_goal(BindingMap	*bm,Fact	*goal,Fact	*f_imdl)	const{

		Group	*primary_grp=get_host();
		uint64	before=goal->get_before();
		uint64	now=Now();
		int32	resilience=_Mem::Get()->get_goal_pred_success_res(primary_grp,before-now);
		
		View	*view=new	View(true,now,1,resilience,primary_grp,primary_grp,goal);	// SYNC_FRONT,res=resilience.
		_Mem::Get()->inject(view);

		MkRdx	*mk_rdx=new	MkRdx(f_imdl,goal->get_goal()->get_super_goal(),goal,1,bm);

		uint16	out_group_count=get_out_group_count();
		for(uint16	i=0;i<out_group_count;++i){

			Group	*out_group=(Group	*)get_out_group(i);
			View	*view=new	NotificationView(primary_grp,out_group,mk_rdx);
			_Mem::Get()->inject_notification(view,true);
		}
	}

	void	PMDLController::inject_simulation(Fact	*goal_pred)	const{	// f->pred->f->obj or f->goal->f->obj.

		Group	*primary_grp=get_host();
		uint64	before=((_Fact	*)goal_pred->get_reference(0)->get_reference(0))->get_before();
		uint64	now=Now();
		int32	resilience=_Mem::Get()->get_goal_pred_success_res(primary_grp,before-now);
		
		View	*view=new	View(true,now,1,resilience,primary_grp,primary_grp,goal_pred);	// SYNC_FRONT,res=resilience.
		_Mem::Get()->inject(view);
	}

	void	PMDLController::monitor_goals(_Fact	*input){

		std::list<P<_GMonitor> >::const_iterator	m;
		g_monitorsCS.enter();
		for(m=g_monitors.begin();m!=g_monitors.end();){

			if((*m)->reduce(input))
				m=g_monitors.erase(m);
			else
				++m;
		}
		g_monitorsCS.leave();
	}

	void	PMDLController::register_predicted_goal_outcome(Fact	*goal,BindingMap	*bm,Fact	*f_imdl,bool	success,bool	injected_goal){	// called only for SIM_COMMITTED mode.

		if(success)
			goal->invalidate();	// monitor still running to detect failures (actual or predicted).
		else{

			if(!injected_goal)	// the goal has not been injected; monitor still running.
				inject_goal(bm,goal,f_imdl);
			else{

				if(goal->is_invalidated()){	// the only case when the goal can be invalidated here is when a predicted failure follows a predicted success.
				
					Fact	*new_goal=new	Fact(goal);
					Goal	*g=new_goal->get_goal();

					uint64	deadline=g->get_target()->get_before();
					uint64	sim_thz=get_sim_thz(Now(),deadline);

					Sim		*new_sim=new	Sim(SIM_ROOT,sim_thz,g->sim->super_goal,false,this);

					g->sim=new_sim;

					add_monitor(new	GMonitor(this,bm,deadline,sim_thz,new_goal,f_imdl,NULL));

					inject_goal(bm,new_goal,f_imdl);
				}
			}
		}
	}

	inline	uint64	PMDLController::get_sim_thz(uint64	now,uint64 deadline)	const{

		uint32	min_sim_thz=_Mem::Get()->get_min_sim_time_horizon()>>1;	// time allowance for the simulated predictions to flow upward.
		uint64	sim_thz=_Mem::Get()->get_sim_time_horizon(deadline-now)>>1;
		if(sim_thz>min_sim_thz)
			return	sim_thz-min_sim_thz;
		else	// no time to simulate.
			return	0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TopLevelMDLController::TopLevelMDLController(r_code::View	*view):PMDLController(view){
	}

	void	TopLevelMDLController::store_requirement(_Fact	*f_imdl,bool	chaining_was_allowed,bool	simulation){
	}

	void	TopLevelMDLController::take_input(r_exec::View	*input){

		if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
			input->object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
			return;
		Controller::__take_input<TopLevelMDLController>(input);
	}

	void	TopLevelMDLController::reduce(r_exec::View	*input){

		_Fact	*input_object=(_Fact	*)input->object;	// input_object is f->obj.
		if(input_object->is_invalidated())
			return;

		Goal	*goal=input_object->get_goal();
		if(goal	&&	goal->is_drive()){

			_Fact	*goal_target=goal->get_target();				// goal_target is f->object.
			float32	confidence=get_cfd()*goal_target->get_cfd();	// reading SR is atomic.
			if(confidence<=get_host()->code(GRP_SLN_THR).asFloat())	// cfd is too low for any sub-goal to be injected.
				return;
			
			P<BindingMap>	bm=new	BindingMap(bindings);
			bm->reset_bwd_timings(goal_target);
			if(bm->match_strict(goal_target,rhs,MATCH_BACKWARD))	// the rhs of a top-level model is never a |fact, hence strict matching instead of lenient.
				abduce(bm,(Fact	*)input_object,confidence);
			else	if(!goal->is_requirement()){	// goal_target may be f->imdl and not a requirement: case of a reuse of the model, i.e. the goal target is for the model to make a prediction: this translates into making a sub-goal from the lhs.

				Code	*imdl=goal_target->get_reference(0);
				if(imdl->code(0).asOpcode()==Opcodes::IMdl	&&	imdl->get_reference(0)==getObject()){	// in that case, get the bm from the imdl, ignore the bwd guards, bind the rhs and inject.

					bm=new	BindingMap(bindings);
					bm->reset_bwd_timings(goal_target);
					bm->init_from_f_ihlp(goal_target);
					abduce(bm,(Fact	*)input_object,confidence);
				}
			}
		}else{

			reductionCS.enter();
			((MDLOverlay	*)*overlays.begin())->reduce(input);	// matching is used to fill up the cache.
			monitor_goals(input_object);
			reductionCS.leave();
		}
	}

	void	TopLevelMDLController::abduce(BindingMap	*bm,Fact	*super_goal,float32	confidence){	// super_goal is a drive.

		if(evaluate_bwd_guards(bm)){	// bm may be updated.

			P<_Fact>	bound_lhs=(_Fact	*)bm->bind_pattern(get_lhs());
			_Fact		*evidence;
			Fact		*f_imdl;

			switch(check_evidences(bound_lhs,evidence)){
			case	MATCH_SUCCESS_POSITIVE:	// goal target is already known: report drive success.
				register_goal_outcome(super_goal,true,evidence);
				break;
			case	MATCH_SUCCESS_NEGATIVE:	// a counter evidence is already known: report drive failure.
				register_goal_outcome(super_goal,false,evidence);
				break;
			case	MATCH_FAILURE:
				f_imdl=get_f_ihlp(bm,false);
				f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.
				bound_lhs->set_cfd(confidence);
				switch(check_predicted_evidences(bound_lhs,evidence)){
				case	MATCH_SUCCESS_POSITIVE:
					break;
				case	MATCH_SUCCESS_NEGATIVE:
				case	MATCH_FAILURE:
					evidence=NULL;
					break;
				}
				abduce_lhs(bm,super_goal,bound_lhs,f_imdl,evidence);
				break;
			}
		}
	}

	void	TopLevelMDLController::abduce_lhs(	BindingMap	*bm,
												Fact		*super_goal,		// f->g->f->obj; actual goal.
												_Fact		*sub_goal_target,	// f->obj, i.e. bound lhs.
												Fact		*f_imdl,
												_Fact		*evidence){

		Goal	*sub_goal=new	Goal(sub_goal_target,super_goal->get_goal()->get_reference(GOAL_ACTR_REF),1);
		uint64	now=Now();
		Fact	*f_sub_goal=new	Fact(sub_goal,now,now,1,1);

		uint64	deadline=sub_goal_target->get_before();
		uint64	sim_thz=get_sim_thz(now,deadline);

		Sim	*sub_sim=new	Sim(SIM_ROOT,sim_thz,super_goal,false,this);

		sub_goal->sim=sub_sim;

		add_monitor(new	GMonitor(this,bm,deadline,sim_thz,f_sub_goal,f_imdl,evidence));

		if(!evidence)
			inject_goal(bm,f_sub_goal,f_imdl);
	}

	void	TopLevelMDLController::predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p){	// no prediction here.
	}

	void	TopLevelMDLController::register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures){
	}

	void	TopLevelMDLController::register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const{

		goal->invalidate();

		uint64	now=Now();
		Code	*f_success_object;
		_Fact	*absentee;
		if(success){

			Code	*success_object=new	Success(goal,evidence,1);
			f_success_object=new	Fact(success_object,now,now,1,1);
		}else{

			Code	*success_object;
			if(!evidence){	// assert absence of the goal target.

				absentee=get_absentee(goal);
				success_object=new	Success(goal,absentee,1);
			}else
				success_object=new	Success(goal,evidence,1);
			f_success_object=new	AntiFact(success_object,now,now,1,1);
		}
		
		Group	*primary_host=get_host();
		uint16	out_group_count=get_out_group_count();
		Group	*drives_host=(Group	*)get_out_group(out_group_count-1);	// the drives group is the last of the output groups.
		int32	resilience=_Mem::Get()->get_goal_pred_success_res(drives_host,0);
		View	*view=new	View(true,now,1,resilience,drives_host,primary_host,f_success_object);
		_Mem::Get()->inject(view);	// inject in the drives group (will be caught by the drive injectors).

		resilience=_Mem::Get()->get_goal_pred_success_res(primary_host,0);
		view=new	View(true,now,1,resilience,primary_host,primary_host,f_success_object);

		for(uint16	i=0;i<out_group_count;++i){	// inject notification in out groups.

			Group	*out_group=(Group	*)get_out_group(i);
			resilience=_Mem::Get()->get_goal_pred_success_res(out_group,0);
			view=new	View(true,now,1,resilience,out_group,primary_host,f_success_object);
			_Mem::Get()->inject(view);

			if(absentee){

				view=new	View(true,now,1,1,out_group,primary_host,absentee);
				_Mem::Get()->inject(view);
			}
		}
	}

	void	TopLevelMDLController::register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const{	// evidence is a simulated prediction.

		Code	*success_object=new	Success(goal,evidence,1);
		Pred	*evidence_pred=evidence->get_pred();
		float32	confidence=evidence_pred->get_target()->get_cfd();
		uint64	now=Now();
		_Fact	*f_success_object;
		if(success)
			f_success_object=new	Fact(success_object,now,now,confidence,1);
		else
			f_success_object=new	AntiFact(success_object,now,now,confidence,1);

		Pred	*pred=new	Pred(f_success_object,1);
		for(uint16	i=0;i<evidence_pred->simulations.size();++i)
			pred->simulations.push_back(evidence_pred->simulations[i]);

		Fact	*f_pred=new	Fact(pred,now,now,1,1);

		Group	*primary_host=get_host();
		uint16	out_group_count=get_out_group_count();
		int32	resilience=_Mem::Get()->get_goal_pred_success_res(primary_host,0);
		View	*view=new	View(true,now,1,resilience,primary_host,primary_host,f_pred);
		_Mem::Get()->inject(view);	// inject in the primary group.
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PrimaryMDLController::PrimaryMDLController(r_code::View	*view):PMDLController(view){
	}

	void	PrimaryMDLController::set_secondary(SecondaryMDLController	*secondary){

		this->secondary=secondary;
	}

	void	PrimaryMDLController::gain_activation(){

		bool	strong;
		HLPController	*c=get_rhs_controller(strong);
		if(c)
			c->add_requirement(strong);
		HLPController::gain_activation();
	}

	void	PrimaryMDLController::lose_activation(){

		HLPController::lose_activation();
		secondary->getView()->set_act(getView()->get_act());	// activate the secondary controller in its own group g: will be performed at the next g->upr.
		bool	strong;											// will trigger secondary->gain_activation() at the next g->upr.
		HLPController	*c=get_rhs_controller(strong);
		if(c)
			c->remove_requirement(strong);
	}

	void	PrimaryMDLController::store_requirement(_Fact	*f_p_f_imdl,bool	chaining_was_allowed,bool	simulation){

		_Fact	*f_imdl=f_p_f_imdl->get_pred()->get_target();
		Code	*mdl=f_imdl->get_reference(0);
		REntry	e(f_p_f_imdl,this,chaining_was_allowed);
		if(f_imdl->is_fact()){	// in case of a positive requirement tell monitors they can check for chaining again.

			std::list<P<SRMonitor> >::const_iterator	m;
			g_monitorsCS.enter();
			for(m=sr_monitors.begin();m!=sr_monitors.end();){	// signal sr-monitors.

				if((*m)->is_alive())
					m=sr_monitors.erase(m);
				else{

					if((*m)->signal(simulation))
						m=sr_monitors.erase(m);
					else
						++m;
				}
			}
			g_monitorsCS.leave();
			_store_requirement(&simulated_requirements.positive_evidences,e);

			if(!simulation){	// r-monitors are never signaled for simulated f_imdl.

				std::list<P<RMonitor> >::const_iterator	m;
				g_monitorsCS.enter();
				for(m=r_monitors.begin();m!=r_monitors.end();){	// signal r-monitors.

					if((*m)->is_alive())
						m=r_monitors.erase(m);
					else{

						if((*m)->signal())
							m=r_monitors.erase(m);
						else
							++m;
					}
				}
				g_monitorsCS.leave();
				_store_requirement(&requirements.positive_evidences,e);
			}
		}else	if(!simulation)
			_store_requirement(&requirements.negative_evidences,e);
		if(!simulation)
			secondary->store_requirement(f_p_f_imdl,chaining_was_allowed,false);
	}

	void	PrimaryMDLController::take_input(r_exec::View	*input){

		if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
			input->object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
			return;
		Controller::__take_input<PrimaryMDLController>(input);
	}

	void	PrimaryMDLController::predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p){

		//rhs->get_reference(0)->trace();//bindings->trace();
		_Fact	*bound_rhs=(_Fact	*)bm->bind_pattern(rhs);	// fact or |fact.
		
		bool	simulation;
		float32	confidence;
		Pred	*prediction=input->get_pred();
		if(prediction){	// the input was a prediction.

			simulation=prediction->is_simulation();
			if(chaining_was_allowed)
				confidence=prediction->get_target()->get_cfd()*get_cfd();
			else
				return;
		}else{

			simulation=false;
			if(chaining_was_allowed)
				confidence=input->get_cfd()*get_cfd();
			else
				confidence=0;
		}
		
		bound_rhs->set_cfd(confidence);

		Pred	*pred=new	Pred(bound_rhs,1);
		uint64	now=Now();
		Fact	*production=new	Fact(pred,now,now,1,1);

		if(prediction	&&	!simulation)	// store the antecedent.
			pred->grounds.push_back(input);

		if(_is_requirement){

			bool	strong;
			PrimaryMDLController	*c=((PrimaryMDLController	*)get_rhs_controller(strong));	// rhs controller: in the same view.
			c->store_requirement(production,chaining_was_allowed,simulation);					// if not simulation, stores also in the secondary controller.
			return;
		}

		if(!simulation){	// rdx and monitor only for predictions built from actual inputs.

			register_requirement(production,r_p);

			if(!chaining_was_allowed){	// reaching this point in the code means that the input was not a prediction.

				PMonitor	*m=new	PMonitor(this,bm,production,false);	// the model will not be rated in case of a failure; the requirements will be rated in both cases (if their own chaining was allowed, else only in case of success and recurse).
				MDLController::add_monitor(m);
			}else{	// try to inject the prediction: if cfd too low, the prediction is not injected.

				uint64	before=bound_rhs->get_before();
				if(prediction){	// no rdx nor monitoring if the input was a prediction; case of a reuse: f_imdl becomes f->p->f_imdl.

					Fact	*pred_f_imdl=new	Fact(new	Pred(f_imdl,1),now,now,1,1);
					inject_prediction(production,pred_f_imdl,confidence,before-now,NULL);
				}else{

					Code		*mk_rdx=new	MkRdx(f_imdl,(Code	*)input,production,1,bindings);
					bool		rate_failures=inject_prediction(production,f_imdl,confidence,before-now,mk_rdx);
					PMonitor	*m=new	PMonitor(this,bm,production,rate_failures);	// not-injected predictions are monitored for rating the model that produced them (successes only).
					MDLController::add_monitor(m);
					
					Group	*secondary_host=secondary->getView()->get_host();	// inject f_imdl in secondary group.
					View	*view=new	View(true,now,confidence,1,getView()->get_host(),secondary_host,f_imdl);	// SYNC_FRONT,res=resilience.
					_Mem::Get()->inject(view);
				}
			}
		}else{	// no monitoring for simulated predictions.

			for(uint16	i=0;i<prediction->simulations.size();++i)
				pred->simulations.push_back(prediction->simulations[i]);
			inject_prediction(production,confidence);	// inject a simulated prediction in the primary group.
		}
	}

	void	PrimaryMDLController::reduce(r_exec::View	*input){

		_Fact	*input_object=(_Fact	*)input->object;	// input_object is f->obj.
		if(input_object->is_invalidated())
			return;

		Goal	*goal=input_object->get_goal();
		if(goal	&&	goal->is_self_goal()	&&	!goal->is_drive()){

			_Fact	*goal_target=goal->get_target();				// goal_target is f->object.
			float32	confidence=get_cfd()*goal_target->get_cfd();	// reading SR is atomic.
			Code	*host=get_host();
			if(confidence<=host->code(GRP_SLN_THR).asFloat())	// cfd is too low for any sub-goal to be injected.
				return;

			P<BindingMap>	bm=new	BindingMap(bindings);
			bm->reset_bwd_timings(goal_target);
			bool	opposite=false;
			MatchResult	match_result=bm->match_lenient(goal_target,rhs,MATCH_BACKWARD);
			switch(match_result){
			case	MATCH_SUCCESS_NEGATIVE:
				opposite=true;
			case	MATCH_SUCCESS_POSITIVE:
				abduce(bm,(Fact	*)input_object,opposite,confidence);
				break;
			default:	// no match; however, goal_target may be f->imdl, i.e. case of a reuse of the model, i.e. the goal is for the model to make a prediction: this translates into making a sub-goal from the lhs.
				if(!goal->is_requirement()){	// models like imdl -> |rhs or |imdl -> rhs are not allowed.

					Code	*imdl=goal_target->get_reference(0);
					if(imdl->code(0).asOpcode()==Opcodes::IMdl	&&	imdl->get_reference(0)==getObject()){	// in that case, get the bm from the imdl, ignore the bwd guards, bind the rhs and inject.

						bm=new	BindingMap(bindings);
						bm->reset_bwd_timings(goal_target);
						bm->init_from_f_ihlp(goal_target);
						abduce(bm,(Fact	*)input_object,opposite,confidence);
					}
				}
				break;
			}
		}else{

			reductionCS.enter();
			((MDLOverlay	*)*overlays.begin())->reduce(input);
			monitor_predictions(input_object);
			monitor_goals(input_object);
			reductionCS.leave();
		}
	}

	void	PrimaryMDLController::abduce(BindingMap	*bm,Fact	*super_goal,bool	opposite,float32	confidence){	// goal is f->g->f->object or f->g->|f->object; called concurrently by redcue() and _GMonitor::update().

		P<Fact>	f_imdl=get_f_ihlp(bm,false);
		Sim			*sim=super_goal->get_goal()->sim;
		uint64		sim_thz=sim->thz>>1;	// 0 if super-goal had not time for simulation.
		uint32		min_sim_thz=_Mem::Get()->get_min_sim_time_horizon()>>1;	// time allowance for the simulated predictions to flow upward.

		Sim	*sub_sim;
		if(sim_thz>min_sim_thz){

			sim_thz-=min_sim_thz;

			f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.

			uint64	now=Now();
			switch(sim->mode){
			case	SIM_ROOT:
				sub_sim=new	Sim(opposite?SIM_MANDATORY:SIM_OPTIONAL,now+sim_thz,super_goal,opposite,sim->root,this,confidence,0);
				break;
			case	SIM_OPTIONAL:
			case	SIM_MANDATORY:
				sub_sim=new	Sim(sim->mode,now+sim_thz,sim->super_goal,opposite,sim->root,sim->sol,sim->sol_cfd,sim->sol_before);
				break;
			}

			Fact	*ground;
			switch(retrieve_imdl_bwd(bm,f_imdl,ground)){
			case	WR_ENABLED:
				f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(true);
			case	NO_R:
				if(sub_sim->mode==SIM_ROOT)
					abduce_lhs(bm,super_goal,f_imdl,opposite,confidence,sub_sim,ground,true);
				else
					abduce_simulated_lhs(bm,super_goal,f_imdl,opposite,confidence,sub_sim);
				break;
			default:	// WR_DISABLED, SR_DISABLED_NO_WR or SR_DISABLED_WR.
				switch(retrieve_simulated_imdl_bwd(bm,f_imdl,sim->root)){
				case	WR_ENABLED:
					f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(true);
				case	NO_R:
					if(sub_sim->mode==SIM_ROOT)
						abduce_lhs(bm,super_goal,f_imdl,opposite,confidence,sub_sim,NULL,true);
					else
						abduce_simulated_lhs(bm,super_goal,f_imdl,opposite,confidence,sub_sim);
					break;
				default:	// WR_DISABLED, SR_DISABLED_NO_WR or SR_DISABLED_WR.
					sub_sim->is_requirement=true;
					if(sub_sim->mode==SIM_ROOT)
						abduce_imdl(bm,super_goal,f_imdl,opposite,confidence,sub_sim);
					else
						abduce_simulated_imdl(bm,super_goal,f_imdl,opposite,confidence,sub_sim);
					break;
				}
				break;
			}
		}else{	// no time to simulate.

			Fact	*ground;
			switch(sim->mode){
			case	SIM_ROOT:
				f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.
				switch(retrieve_imdl_bwd(bm,f_imdl,ground)){
				case	WR_ENABLED:
					f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(true);
				case	NO_R:
					sub_sim=new	Sim(SIM_ROOT,0,super_goal,opposite,this);
					abduce_lhs(bm,super_goal,f_imdl,opposite,confidence,sub_sim,ground,false);
					break;
				default:	// WR_DISABLED, SR_DISABLED_NO_WR or SR_DISABLED_WR.
					sub_sim=new	Sim(SIM_ROOT,0,super_goal,opposite,this);
					sub_sim->is_requirement=true;
					abduce_imdl(bm,super_goal,f_imdl,opposite,confidence,sub_sim);
					break;
				}
				break;
			case	SIM_OPTIONAL:
			case	SIM_MANDATORY:	// stop the simulation branch.
				predict_simulated_lhs(bm,opposite,confidence,sim);
				break;
			}
		}
	}

	void	PrimaryMDLController::abduce_lhs(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim,Fact	*ground,bool	set_before){	// goal is f->g->f->object or f->g->|f->object; called concurrently by reduce() and _GMonitor::update().

		if(evaluate_bwd_guards(bm)){	// bm may be updated.

			P<_Fact>	bound_lhs=(_Fact	*)bm->bind_pattern(get_lhs());
			if(opposite)
				set_opposite(bound_lhs);
			bound_lhs->set_cfd(confidence);

			_Fact	*evidence;
			switch(check_evidences(bound_lhs,evidence)){
			case	MATCH_SUCCESS_POSITIVE:	// goal target is already known: abort.
				break;
			case	MATCH_SUCCESS_NEGATIVE:	// a counter evidence is already known: abort.
				break;
			case	MATCH_FAILURE:{

				f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.

				switch(check_predicted_evidences(bound_lhs,evidence)){
				case	MATCH_SUCCESS_POSITIVE:
					break;
				case	MATCH_SUCCESS_NEGATIVE:
				case	MATCH_FAILURE:
					evidence=NULL;
					break;
				}

				Goal	*sub_goal=new	Goal(bound_lhs,super_goal->get_goal()->get_reference(GOAL_ACTR_REF),1);
				sub_goal->ground=ground;
				sub_goal->sim=sim;
				if(set_before)
					sim->sol_before=bound_lhs->get_before();
				
				uint64	now=Now();
				Fact	*f_sub_goal=new	Fact(sub_goal,now,now,1,1);

				add_monitor(new	GMonitor(this,bm,bound_lhs->get_before(),0,f_sub_goal,f_imdl,evidence));
				
				if(!evidence)
					inject_goal(bm,f_sub_goal,f_imdl);
				break;
			}
			}
		}
	}

	void	PrimaryMDLController::abduce_imdl(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim){	// goal is f->g->f->object or f->g->|f->object; called concurrently by redcue() and _GMonitor::update().

		f_imdl->set_cfd(confidence);

		Goal	*sub_goal=new	Goal(f_imdl,super_goal->get_goal()->get_reference(GOAL_ACTR_REF),1);
		sub_goal->sim=sim;
		
		uint64	now=Now();
		Fact	*f_sub_goal=new	Fact(sub_goal,now,now,1,1);
		add_monitor(new	RMonitor(this,bm,super_goal->get_goal()->get_target()->get_before(),f_sub_goal,f_imdl));	// the monitor will wait until the deadline of the super-goal.
		inject_goal(bm,f_sub_goal,f_imdl);
	}

	void	PrimaryMDLController::abduce_simulated_lhs(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim){	// goal is f->g->f->object or f->g->|f->object; called concurrently by redcue() and _GMonitor::update().

		if(evaluate_bwd_guards(bm)){	// bm may be updated.

			P<_Fact>	bound_lhs=(_Fact	*)bm->bind_pattern(get_lhs());
			if(opposite)
				set_opposite(bound_lhs);
			bound_lhs->set_cfd(confidence);

			_Fact	*evidence;
			switch(check_evidences(bound_lhs,evidence)){
			case	MATCH_SUCCESS_POSITIVE:	// an evidence is already known: stop the simulation.
				register_simulated_goal_outcome(super_goal,true,evidence);
				break;
			case	MATCH_SUCCESS_NEGATIVE:	// a counter evidence is already known: stop the simulation.
				register_simulated_goal_outcome(super_goal,false,evidence);
				break;
			case	MATCH_FAILURE:
				switch(check_predicted_evidences(bound_lhs,evidence)){
				case	MATCH_SUCCESS_POSITIVE:	// a predicted evidence is already known: stop the simulation.
					register_simulated_goal_outcome(super_goal,true,evidence);
					break;
				case	MATCH_SUCCESS_NEGATIVE:
					register_simulated_goal_outcome(super_goal,false,evidence);
					break;
				case	MATCH_FAILURE:{

					f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.

					Goal	*sub_goal=new	Goal(bound_lhs,super_goal->get_goal()->get_reference(GOAL_ACTR_REF),1);
					
					sub_goal->sim=sim;
					
					uint64	now=Now();
					Fact	*f_sub_goal=new	Fact(sub_goal,now,now,1,1);

					add_monitor(new	SGMonitor(this,bm,sim->thz,f_sub_goal,f_imdl));
					inject_simulation(f_sub_goal);
					break;
				}
				}
				break;
			}
		}
	}

	void	PrimaryMDLController::abduce_simulated_imdl(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim){	// goal is f->g->f->object or f->g->|f->object; called concurrently by redcue() and _GMonitor::update().

		f_imdl->set_cfd(confidence);

		Goal	*sub_goal=new	Goal(f_imdl,super_goal->get_goal()->get_reference(GOAL_ACTR_REF),1);
		sub_goal->sim=sim;
		
		uint64	now=Now();
		Fact	*f_sub_goal=new	Fact(sub_goal,now,now,1,1);
		add_monitor(new	SRMonitor(this,bm,sim->thz,f_sub_goal,f_imdl));
		inject_simulation(f_sub_goal);
	}

	bool	PrimaryMDLController::check_imdl(Fact	*goal,BindingMap	*bm){	// goal is f->g->f->imdl; called by r-monitors.

		Goal	*g=goal->get_goal();
		Fact	*f_imdl=(Fact	*)g->get_target();
		
		Sim	*sim=g->sim;
		Fact	*ground;
		switch(retrieve_imdl_bwd(bm,f_imdl,ground)){
		case	WR_ENABLED:
			f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(true);
		case	NO_R:
			if(evaluate_bwd_guards(bm)){	// bm may be updated.

				f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.
				abduce_lhs(bm,sim->super_goal,f_imdl,sim->opposite,f_imdl->get_cfd(),new	Sim(SIM_ROOT,0,sim->super_goal,sim->opposite,this),ground,false);
			}
			return	true;
		default:	// WR_DISABLED, SR_DISABLED_NO_WR or SR_DISABLED_WR.
			return	false;
		}
	}

	bool	PrimaryMDLController::check_imdl(Fact	*goal,BindingMap	*bm,Controller	*root){	// goal is f->g->f->imdl; called by sr-monitors.

		Goal	*g=goal->get_goal();
		Fact	*f_imdl=(Fact	*)g->get_target();
		ChainingStatus	c_s;
		if(root)
			c_s=retrieve_simulated_imdl_bwd(bm,f_imdl,root);
		else{

			Fact	*ground;
			c_s=retrieve_imdl_bwd(bm,f_imdl,ground);
		}

		Sim	*sim=g->sim;
		switch(c_s){
		case	WR_ENABLED:
			f_imdl->get_reference(0)->code(I_HLP_WR_E)=Atom::Boolean(true);
		case	NO_R:
			if(evaluate_bwd_guards(bm)){	// bm may be updated.

				f_imdl->set_reference(0,bm->bind_pattern(f_imdl->get_reference(0)));	// valuate f_imdl from updated bm.
				abduce_simulated_lhs(bm,sim->super_goal,f_imdl,sim->opposite,f_imdl->get_cfd(),new	Sim(sim));
			}
			return	true;
		default:	// WR_DISABLED, SR_DISABLED_NO_WR or SR_DISABLED_WR.
			return	false;
		}
	}

	inline	void	PrimaryMDLController::predict_simulated_lhs(BindingMap	*bm,bool	opposite,float32	confidence,Sim	*sim){

		_Fact	*bound_lhs=(_Fact	*)bm->bind_pattern(get_lhs());
		if(opposite)
			set_opposite(bound_lhs);
		bound_lhs->set_cfd(confidence);

		predict_simulated_evidence(bound_lhs,sim);
	}

	inline	void	PrimaryMDLController::predict_simulated_evidence(_Fact	*evidence,Sim	*sim){

		Pred	*pred=new	Pred(evidence,1);
		pred->simulations.push_back(sim);

		uint64	now=Now();
		inject_simulation(new	Fact(pred,now,now,1,1));
	}

	void	PrimaryMDLController::register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures){

		f_pred->invalidate();

		register_req_outcome(f_pred->get_pred()->get_target(),success,rate_failures);

		if(_is_requirement)
			return;

		_Fact	*f_evidence=evidence;
		if(!f_evidence)	// failure: assert absence of the pred target.
			f_evidence=get_absentee(f_pred);

		Success	*success_object=new	Success(f_pred,f_evidence,1);
		Code	*f_success_object;
		uint64	now=Now();
		if(success)
			f_success_object=new	Fact(success_object,now,now,confidence,1);
		else
			f_success_object=new	AntiFact(success_object,now,now,confidence,1);

		Group	*primary_host=get_host();
		uint16	out_group_count=get_out_group_count();
		for(uint16	i=0;i<out_group_count;++i){	// inject notification in out groups.

			Group	*out_group=(Group	*)get_out_group(i);
			int32	resilience=_Mem::Get()->get_goal_pred_success_res(out_group,0);
			View	*view=new	View(true,now,1,resilience,out_group,primary_host,f_success_object);
			_Mem::Get()->inject(view);

			view=new	View(true,now,1,1,out_group,primary_host,f_evidence);
			_Mem::Get()->inject(view);
		}
	}

	void	PrimaryMDLController::register_req_outcome(_Fact	*f_imdl,bool	success,bool	rate_failures){

		if(success)
			rate_model(true);
		else	if(rate_failures)
			rate_model(false);

		active_requirementsCS.enter();
		UNORDERED_MAP<P<_Fact>,RequirementsPair,PHash<_Fact>	>::const_iterator	r=active_requirements.find(f_imdl);
		if(r!=active_requirements.end()){	// some requirements were controlling the prediction: give feedback.

			for(uint32	i=0;i<r->second.first.controllers.size();++i)
				r->second.first.controllers[i]->register_req_outcome(r->second.first.f_imdl,success,r->second.first.chaining_was_allowed);
			for(uint32	i=0;i<r->second.second.controllers.size();++i)
				r->second.second.controllers[i]->register_req_outcome(r->second.second.f_imdl,!success,r->second.second.chaining_was_allowed);
		}
		active_requirementsCS.leave();
	}

	void	PrimaryMDLController::register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const{

		goal->invalidate();

		uint64	now=Now();
		_Fact	*f_success_object;
		_Fact	*absentee;

		if(success){

			Code	*success_object=new	Success(goal,evidence,1);
			f_success_object=new	Fact(success_object,now,now,1,1);
		}else{

			Code	*success_object;
			if(!evidence){	// assert absence of the goal target.

				absentee=get_absentee(goal);
				success_object=new	Success(goal,absentee,1);
			}else
				success_object=new	Success(goal,evidence,1);
			f_success_object=new	AntiFact(success_object,now,now,1,1);
		}

		Group	*primary_host=get_host();
		int32	resilience=_Mem::Get()->get_goal_pred_success_res(primary_host,0);
		View	*view=new	View(true,now,1,resilience,primary_host,primary_host,f_success_object);

		uint16	out_group_count=get_out_group_count();
		for(uint16	i=0;i<out_group_count;++i){	// inject notification in out groups.

			Group	*out_group=(Group	*)get_out_group(i);
			resilience=_Mem::Get()->get_goal_pred_success_res(out_group,0);
			view=new	View(true,now,1,resilience,out_group,primary_host,f_success_object);
			_Mem::Get()->inject(view);

			if(absentee){

				view=new	View(true,now,1,1,out_group,primary_host,absentee);
				_Mem::Get()->inject(view);
			}
		}
	}

	void	PrimaryMDLController::register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const{

		Code	*success_object=new	Success(goal,evidence,1);
		_Fact	*f_success;

		uint64	now=Now();
		if(success)
			f_success=new	Fact(success_object,now,now,1,1);
		else
			f_success=new	AntiFact(success_object,now,now,1,1);

		Pred	*pred=new	Pred(f_success,1);
		Fact	*f_pred=new	Fact(pred,now,now,1,1);

		Group	*primary_host=get_host();
		int32	resilience=_Mem::Get()->get_goal_pred_success_res(primary_host,0);
		View	*view=new	View(true,now,1,resilience,primary_host,primary_host,f_pred);
	}

	void	PrimaryMDLController::rate_model(bool	success){

		Code	*model=get_core_object();

		reductionCS.enter();	// protects the model's data.

		float32	strength=model->code(MDL_STRENGTH).asFloat();
		uint32	instance_count=model->code(MDL_CNT).asFloat();
		uint32	success_count=model->code(MDL_SR).asFloat()*instance_count;

		model->code(MDL_DSR)=model->code(MDL_SR);
		model->code(MDL_CNT)=Atom::Float(++instance_count);

		float32	success_rate;
		if(success){

			++success_count;
			success_rate=success_count/instance_count;
			uint32	instance_count_base=_Mem::Get()->get_mdl_inertia_cnt_thr();
			if(success_rate>=_Mem::Get()->get_mdl_inertia_sr_thr()	&&	instance_count>=instance_count_base){	// trim the instance count to reduce the rating's inertia.

				instance_count-=instance_count_base;
				if(instance_count==0)
					instance_count=1;
				success_count=instance_count;
				success_rate=1;
				model->code(MDL_STRENGTH)=Atom::Float(1);
			}
		}else{

			++instance_count;
			success_rate=success_count/instance_count;
		}

		model->code(MDL_SR)=Atom::Float(success_rate);

		getView()->set_act(success_rate);

		reductionCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SecondaryMDLController::SecondaryMDLController(r_code::View	*view):MDLController(view){

		SecondaryMDLOverlay	*o=new	SecondaryMDLOverlay(this,bindings);	//	master overlay.
		o->load_patterns();
		overlays.push_back(o);
	}

	void	SecondaryMDLController::set_primary(PrimaryMDLController	*primary){

		this->primary=primary;
	}

	void	SecondaryMDLController::take_input(r_exec::View	*input){

		if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
			input->object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
			return;
		Controller::__take_input<SecondaryMDLController>(input);
	}

	void	SecondaryMDLController::reduce(r_exec::View	*input){

		if(input->object->is_invalidated())
			return;

		bool	match;
		reductionCS.enter();
		match=(((MDLOverlay	*)*overlays.begin())->reduce(input)!=NULL);	// forward chaining.
		reductionCS.leave();

		if(!match)
			monitor_predictions(input->object);
	}

	void	SecondaryMDLController::predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p){	// predicitons are not injected: they are silently produced for rating purposes.

		//rhs->trace();rhs->get_reference(0)->trace();bindings->trace();
		_Fact	*bound_rhs=(_Fact	*)bm->bind_pattern(rhs);	// fact or |fact.
		Pred	*_prediction=new	Pred(bound_rhs,1);
		uint64	now=Now();
		Fact	*production=new	Fact(_prediction,now,now,1,1);
		
		register_requirement(production,r_p);
		
		if(_is_requirement){	// store in the rhs controller, even if primary (to allow rating in any case).

			bool	strong;
			((MDLController	*)get_rhs_controller(strong))->store_requirement(bound_rhs,chaining_was_allowed,false);
			return;
		}

		PMonitor	*m=new	PMonitor(this,bm,production,false);	// predictions are monitored for rating (successes only); no injection.
		add_monitor(m);
	}

	void	SecondaryMDLController::store_requirement(_Fact	*f_imdl,bool	chaining_was_allowed,bool	simulation){

		Code	*mdl=f_imdl->get_reference(0);
		REntry	e(f_imdl,this,chaining_was_allowed);
		if(f_imdl->is_fact())
			_store_requirement(&requirements.positive_evidences,e);
		else
			_store_requirement(&requirements.negative_evidences,e);
	}

	void	SecondaryMDLController::rate_model()	const{	// acknowledge successes only.

		Code	*model=get_core_object();
		uint32	instance_count=model->code(MDL_CNT).asFloat();
		uint32	success_count=model->code(MDL_SR).asFloat()*instance_count;

		model->code(MDL_DSR)=Atom::Float(model->code(MDL_SR).asFloat());
		model->code(MDL_CNT)=Atom::Float(++instance_count);

		++success_count;
		float32		success_rate=success_count/instance_count;	// no trimming.
		model->code(MDL_SR)=Atom::Float(success_rate);

		if(success_rate>primary->getView()->get_host()->get_act_thr())
			primary->getView()->set_act(success_rate);	// activate the primary controller in its own group g: will be performmed at the nex g->upr.
		else{											// will trigger primary->gain_activation at the next g->upr.
			
			if(success_rate>getView()->get_host()->get_act_thr())
				getView()->set_act(success_rate);
			else	// delete the model.
				getView()->set_res(0);
		}
	}

	void	SecondaryMDLController::register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures){	// success==false means executed in the thread of a time core; otherwise, executed in the same thread as for Controller::reduce().

		register_req_outcome(f_pred->get_pred()->get_target(),success,rate_failures);
	}

	void	SecondaryMDLController::register_req_outcome(_Fact	*f_imdl,bool	success,bool	rate_failures){

		if(success){

			rate_model();

			active_requirementsCS.enter();
			UNORDERED_MAP<P<_Fact>,RequirementsPair,PHash<_Fact>	>::const_iterator	r=active_requirements.find(f_imdl);
			if(r!=active_requirements.end()){	// some requirements were controlling the prediction: give feedback.

				for(uint32	i=0;i<r->second.first.controllers.size();++i)
					r->second.first.controllers[i]->register_req_outcome(r->second.first.f_imdl,success,r->second.first.chaining_was_allowed);
			}
			active_requirementsCS.leave();
		}
	}
}