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
#include	"mem.h"


namespace	r_exec{

	HLPOverlay::HLPOverlay(Controller	*c,const	BindingMap	*bindings,uint8	reduction_mode):Overlay(c),reduction_mode(reduction_mode){

		this->bindings=(BindingMap	*)bindings;
	}

	HLPOverlay::~HLPOverlay(){
	}

	Code	*HLPOverlay::get_mk_sim(Code	*object)	const{

		if(reduction_mode	&	RDX_MODE_SIMULATION)
			return	factory::Object::MkSim(object,getObject(),1);
		return	NULL;
	}

	Code	*HLPOverlay::get_mk_asmp(Code	*object)	const{

		if(reduction_mode	&	RDX_MODE_ASSUMPTION)
			return	factory::Object::MkAsmp(object,getObject(),1,1);	//	TODO: put the right value (from where?) for the confidence member.
		return	NULL;
	}
}