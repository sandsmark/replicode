//	rgrp_overlay.cpp
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

#include	"rgrp_overlay.h"


namespace	r_exec{

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RGRPController::RGRPController(_Mem	*m,r_code::View	*rgrp_view):Controller(m,rgrp_view){

		//overlays.push_back(new	RGRPOverlay(this));
	}
	
	RGRPController::~RGRPController(){
	}

	void	RGRPController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		overlayCS.enter();

		//	Pass the inputs to all ipgm views' controllers in the rgrp.

		/*if(tsc>0){	// 1st overlay is the master (no match yet); other overlays are pushed back in order of their matching time. 
			
			// start from the last overlay, and erase all of them that are older than tsc.
			uint64	now=Now();
			std::list<P<Overlay> >::iterator	master=overlays.begin();
			std::list<P<Overlay> >::iterator	o;
			std::list<P<Overlay> >::iterator	previous;
			for(o=overlays.end();o!=master;){

				if(now-((RGRPOverlay	*)(*o))->birth_time>tsc){
					
					previous=--o;
					(*o)->kill();
					overlays.erase(o);
					o=previous;
				}else
					break;
			}
		}

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob	*j=new	ReductionJob(new	View(input),*o);
			mem->pushReductionJob(j);
		}
*/
		overlayCS.leave();
	}
}