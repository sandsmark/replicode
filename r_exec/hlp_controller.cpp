//	hlp_controller.cpp
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

#include	"hlp_controller.h"
#include	"hlp_overlay.h"
#include	"mem.h"


namespace	r_exec{

	HLPController::HLPController(r_code::View	*view):OController(view),strong_requirement_count(0),weak_requirement_count(0),requirement_count(0){

		bindings=new	BindingMap();

		Code	*object=get_unpacked_object();
		bindings->init_from_hlp(object);	// init a binding map from the patterns.

		_has_tpl_args=object->code(object->code(HLP_TPL_ARGS).asIndex()).getAtomCount()>0;
		ref_count=0;
		last_match_time=Now();
	}

	HLPController::~HLPController(){
	}

	void	HLPController::invalidate(){

		invalidated=1;
		controllers.clear();
	}

	void	HLPController::add_requirement(bool	strong){

		reductionCS.enter();
		if(strong)
			++strong_requirement_count;
		else
			++weak_requirement_count;
		++requirement_count;
		reductionCS.leave();
	}

	void	HLPController::remove_requirement(bool	strong){

		reductionCS.enter();
		if(strong)
			--strong_requirement_count;
		else
			--weak_requirement_count;
		--requirement_count;
		reductionCS.leave();
	}

	uint32	HLPController::get_requirement_count(uint32	&weak_requirement_count,uint32	&strong_requirement_count){

		uint32	r_c;
		reductionCS.enter();
		r_c=requirement_count;
		weak_requirement_count=this->weak_requirement_count;
		strong_requirement_count=this->strong_requirement_count;
		reductionCS.leave();
		return	r_c;
	}

	uint32	HLPController::get_requirement_count(){

		uint32	r_c;
		reductionCS.enter();
		r_c=requirement_count;
		reductionCS.leave();
		return	r_c;
	}

	uint16	HLPController::get_out_group_count()	const{

		return	getObject()->code(getObject()->code(HLP_OUT_GRPS).asIndex()).getAtomCount();
	}

	Code	*HLPController::get_out_group(uint16	i)	const{

		Code	*hlp=getObject();
		uint16	out_groups_index=hlp->code(HLP_OUT_GRPS).asIndex()+1;	// first output group index.
		return	hlp->get_reference(hlp->code(out_groups_index+i).asIndex());
	}

	bool	HLPController::evaluate_bwd_guards(BindingMap	*bm){

		HLPOverlay	o(this,bm,true);
		return	o.evaluate_bwd_guards();
	}

	inline	Group	*HLPController::get_host()	const{

		return	(Group	*)getView()->get_host();
	}

	void	HLPController::inject_prediction(Fact	*prediction,float32	confidence)	const{	// prediction is simulated: f->pred->f->target.
		
		Code	*primary_host=get_host();
		float32	sln_thr=primary_host->code(GRP_SLN_THR).asFloat();
		if(confidence>sln_thr){	// do not inject if cfd is too low.

			View	*view=new	View(View::SYNC_ONCE,Now(),confidence,1,primary_host,primary_host,prediction);	//	SYNC_ONCE,res=1.
			_Mem::Get()->inject(view);
		}
	}

	MatchResult	HLPController::check_evidences(_Fact *target,_Fact	*&evidence){

		MatchResult	r=MATCH_FAILURE;
		evidences.CS.enter();
		uint64	now=Now();
		std::list<EEntry>::const_iterator	e;
		for(e=evidences.evidences.begin();e!=evidences.evidences.end();){

			if((*e).is_too_old(now))	// garbage collection.	// garbage collection.
				e=evidences.evidences.erase(e);
			else{

				if((r=(*e).evidence->is_evidence(target))!=MATCH_FAILURE){

					evidence=(*e).evidence;
					evidences.CS.leave();
					return	r;
				}
				++e;
			}
		}
		evidence=NULL;
		evidences.CS.leave();
		return	r;
	}

	MatchResult	HLPController::check_predicted_evidences(_Fact *target,_Fact *&evidence){

		MatchResult	r=MATCH_FAILURE;
		predicted_evidences.CS.enter();
		uint64	now=Now();
		std::list<PEEntry>::const_iterator	e;
		for(e=predicted_evidences.evidences.begin();e!=predicted_evidences.evidences.end();){

			if((*e).is_too_old(now))	// garbage collection.	// garbage collection.
				e=predicted_evidences.evidences.erase(e);
			else{

				if((r=(*e).evidence->is_evidence(target))!=MATCH_FAILURE){

					if(target->get_cfd()<(*e).evidence->get_cfd()){

						evidence=(*e).evidence;
						predicted_evidences.CS.leave();
						return	r;
					}else
						r=MATCH_FAILURE;
				}
				++e;
			}
		}
		evidence=NULL;
		predicted_evidences.CS.leave();
		return	r;
	}

	bool	HLPController::become_invalidated(){

		if(is_invalidated())
			return	true;

		for(uint16	i=0;i<controllers.size();++i){

			if(controllers[i]!=NULL	&&	controllers[i]->is_invalidated()){

				kill_views();
				return	true;
			}
		}

		if(has_tpl_args()){

			if(refCount==1	&&	ref_count>1){	// the ctrler was referenced by others, is no longer and has tpl args: it will not be able to predict: kill.
				
				kill_views();
				return	true;
			}else
				ref_count=refCount;
		}

		return	false;
	}

	bool	HLPController::is_orphan(){

		if(_has_tpl_args){
			
			if(get_requirement_count()==0){

				kill_views();
				return	true;
			}
		}
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	HLPController::EEntry::EEntry(_Fact	*evidence):evidence(evidence){

		load_data(evidence);
	}

	HLPController::EEntry::EEntry(_Fact	*evidence,_Fact	*payload):evidence(evidence){

		load_data(payload);
	}

	void	HLPController::EEntry::load_data(_Fact	*evidence){

		after=evidence->get_after();
		before=evidence->get_before();
		confidence=evidence->get_cfd();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	HLPController::PEEntry::PEEntry(_Fact	*evidence):EEntry(evidence,evidence->get_pred()->get_target()){
	}
}