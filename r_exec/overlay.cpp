//	overlay.cpp
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

#include	"overlay.h"
#include	"mem.h"


namespace	r_exec{

	Overlay::Overlay():alive(true){
	}

	Overlay::~Overlay(){
	}

	Overlay::Overlay(Controller	*c):controller(c),alive(true){
	}

	_Mem	*Overlay::get_mem()	const{

		return	controller->get_mem();
	}

	r_code::Code	*Overlay::buildObject(Atom	head)	const{
		
		return	controller->get_mem()->buildObject(head);
	}

	void	Overlay::reset(){
	}

	void	Overlay::reduce(r_exec::View	*input){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Controller::Controller(_Mem	*m,r_code::View	*view):mem(m),view(view),alive(true){

		switch(getObject()->code(0).getDescriptor()){
		case	Atom::INSTANTIATED_PROGRAM:
			tsc=Utils::GetTimestamp<Code>(getObject(),IPGM_TSC);
			break;
		case	Atom::INSTANTIATED_CPP_PROGRAM:
			tsc=Utils::GetTimestamp<Code>(getObject(),ICPP_PGM_TSC);
			break;
		case	Atom::REDUCTION_GROUP:
			tsc=Utils::GetTimestamp<Code>(getObject(),RGRP_TSC);
			break;
		}
	}

	Controller::~Controller(){
	}

	void	Controller::kill(){
		
		aliveCS.enter();
		if(!alive){

			aliveCS.leave();
			return;
		}
		alive=false;
		aliveCS.leave();

		std::list<P<Overlay> >::const_iterator	o;
		overlayCS.enter();
		for(o=overlays.begin();o!=overlays.end();++o)
			(*o)->kill();
		overlays.clear();
		overlayCS.leave();
	}

	inline	void	Controller::add(Overlay	*overlay){	//	overlay has just matched an input; builds a copy of overlay.

		overlayCS.enter();
		overlays.push_back(overlay);
		overlayCS.leave();
	}

	inline	void	Controller::remove(Overlay	*overlay){

		overlayCS.enter();
		if(overlays.size()>1){

			overlay->kill();
			overlays.remove(overlay);
		}else
			overlay->reset();
		overlayCS.leave();
	}

	void	Controller::take_input(r_exec::View	*input,Controller	*origin){
	}
}