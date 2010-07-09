//	pgm_overlay.inline.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

namespace	r_exec{

	inline	void	Overlay::kill(){
		
		alive=false;
	}

	inline	bool	Overlay::is_alive()	const{
		
		return	alive;
	}

	inline	void	Overlay::patch_code(uint16	index,Atom	value){

		pgm_code[index]=value;
		patch_indices.push_back(index);
	}

	inline	r_code::Code	*Overlay::getIPGM()	const{	
		
		return	controller->getIPGM();
	}
	
	inline	r_exec::View	*Overlay::getIPGMView()	const{	
		
		return	controller->getIPGMView();
	}

	////////////////////////////////////////////////////////////////
	
	inline	r_code::Code	*IOverlay::getInputObject(uint16	i)	const{	
		
		return	input_views[i]->object;
	}
	
	inline	r_code::View	*IOverlay::getInputView(uint16	i)	const{	
		
		return	(r_code::View	*)input_views[i];
	}

	////////////////////////////////////////////////////////////////

	inline	bool	IPGMController::is_alive()	const{
		
		return	alive;
	}

	inline	r_exec::View	*IPGMController::getIPGMView()	const{
		
		return	(r_exec::View	*)ipgm_view;
	}
	
	inline	r_code::Code	*IPGMController::getIPGM()	const{
		
		return	ipgm_view->object;
	}
}