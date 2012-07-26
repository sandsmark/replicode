//	hlp_overlay.cpp
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

#include	"hlp_overlay.h"
#include	"hlp_controller.h"
#include	"hlp_context.h"
#include	"mem.h"


namespace	r_exec{

	bool	HLPOverlay::EvaluateBWDGuards(Controller	*c,HLPBindingMap	*bindings){

		HLPOverlay	o(c,bindings);
		return	o.evaluate_bwd_guards();
	}

	bool	HLPOverlay::CheckFWDTimings(Controller	*c,HLPBindingMap	*bindings){

		HLPOverlay	o(c,bindings);
		return	o.check_fwd_timings();
	}

	bool	HLPOverlay::ScanBWDGuards(Controller	*c,HLPBindingMap	*bindings){

		HLPOverlay	o(c,bindings);
		return	o.scan_bwd_guards();
	}

	HLPOverlay::HLPOverlay(Controller	*c,HLPBindingMap	*bindings):Overlay(c,true),bindings(bindings){
	}
	
	HLPOverlay::HLPOverlay(Controller	*c,const	HLPBindingMap	*bindings,bool	load_code):Overlay(c,load_code){

		this->bindings=new	HLPBindingMap((HLPBindingMap	*)bindings);
	}

	HLPOverlay::~HLPOverlay(){
	}

	Atom	*HLPOverlay::get_value_code(uint16	id)	const{

		return	bindings->get_value_code(id);
	}

	uint16	HLPOverlay::get_value_code_size(uint16	id)	const{

		return	bindings->get_value_code_size(id);
	}

	inline	bool	HLPOverlay::evaluate_guards(uint16	guard_set_iptr_index){

		uint16	guard_set_index=code[guard_set_iptr_index].asIndex();
		uint16	guard_count=code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i){

			if(!evaluate(guard_set_index+i))
				return	false;
		}
		return	true;
	}

	bool	HLPOverlay::evaluate_fwd_guards(){

		return	evaluate_guards(HLP_FWD_GUARDS);
	}

	bool	HLPOverlay::evaluate_bwd_guards(){

		return	evaluate_guards(HLP_BWD_GUARDS);
	}

	bool	HLPOverlay::evaluate(uint16	index){

		HLPContext	c(code,index,this);
		uint16	result_index;
		return	c.evaluate(result_index);
	}

	bool	HLPOverlay::check_fwd_timings(){

		int16	fwd_after_guard_index=-1;
		int16	fwd_before_guard_index=-1;

		uint16	bm_fwd_after_index=bindings->get_fwd_after_index();
		uint16	bm_fwd_before_index=bindings->get_fwd_before_index();

		uint16	guard_set_index=code[HLP_BWD_GUARDS].asIndex();
		uint16	guard_count=code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i){	// find the relevant guards.

			uint16	index=guard_set_index+i;
			Atom	a=code[index];
			if(a.getDescriptor()==Atom::ASSIGN_PTR){

				uint16	_i=a.asAssignmentIndex();
				if(_i==bm_fwd_after_index)
					fwd_after_guard_index=i;
				if(_i==bm_fwd_before_index)
					fwd_before_guard_index=i;
			}
		}

		if(!evaluate(guard_set_index+fwd_before_guard_index))
			return	false;
		if(bindings->get_fwd_before()<=Now())
			return	false;
		if(!evaluate(guard_set_index+fwd_after_guard_index))
			return	false;

		return	true;
	}

	bool	HLPOverlay::scan_bwd_guards(){

		uint16	guard_set_index=code[HLP_BWD_GUARDS].asIndex();
		uint16	guard_count=code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i){

			uint16	index=guard_set_index+i;
			Atom	a=code[index];
			switch(a.getDescriptor()){
			case	Atom::I_PTR:
				if(!scan_location(a.asIndex()))
					return	false;
				break;
			case	Atom::ASSIGN_PTR:
				if(!scan_location(a.asIndex()))
					return	false;
				break;
			}
		}
		return	true;
	}

	bool	HLPOverlay::scan_location(uint16	index){

		Atom	a=code[index];
		switch(a.getDescriptor()){
		case	Atom::I_PTR:
			return	scan_location(a.asIndex());
		case	Atom::ASSIGN_PTR:
			return	scan_location(a.asIndex());
		case	Atom::VL_PTR:
			if(bindings->scan_variable(a.asIndex()))
				return	true;
			else
				return	scan_variable(a.asIndex());
		case	Atom::OPERATOR:{
			uint16	atom_count=a.getAtomCount();
			for(uint16	j=1;j<=atom_count;++j){

				if(!scan_location(index+j))
					return	false;
			}
			return	true;
		}
		default:
			return	true;
		}
	}

	bool	HLPOverlay::scan_variable(uint16	index){	// check if the variable can be bound.

		uint16	guard_set_index=code[HLP_BWD_GUARDS].asIndex();
		uint16	guard_count=code[guard_set_index].getAtomCount();
		for(uint16	i=1;i<=guard_count;++i){

			uint16	index=guard_set_index+i;
			Atom	a=code[index];
			switch(a.getDescriptor()){
			case	Atom::ASSIGN_PTR:
				if(a.asIndex()==index)
					return	scan_location(a.asAssignmentIndex());
				break;
			}
		}

		return	false;
	}

	Code	*HLPOverlay::get_unpacked_object()	const{
		
		return	((HLPController	*)controller)->get_unpacked_object();
	}

	void	HLPOverlay::store_evidence(_Fact	*evidence,bool	prediction,bool	simulation){

		if(prediction){

			if(!simulation)
				((HLPController	*)controller)->store_predicted_evidence(evidence);
		}else
			((HLPController	*)controller)->store_evidence(evidence);
	}
}