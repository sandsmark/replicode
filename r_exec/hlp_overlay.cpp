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

	HLPOverlay::HLPOverlay(Controller	*c,const	BindingMap	*bindings,bool	load_code):Overlay(c,load_code){

		this->bindings=new	BindingMap((BindingMap	*)bindings);
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

	Code	*HLPOverlay::get_unpacked_object()	const{
		
		return	((HLPController	*)controller)->get_unpacked_object();
	}
}