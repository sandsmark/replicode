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
#include	"mem.h"


namespace	r_exec{

	RGRPOverlay::RGRPOverlay(RGRPController	*c):Overlay(c){

		birth_time=Now();
	}

	RGRPOverlay::~RGRPOverlay(){
	}

	void	RGRPOverlay::reduce(r_exec::View	*input){

		//	for all remaining binders in this overlay:
		//		binder_view->take_input(input,this); this is needed for (a) dereferencing variables and, (b) calling the overlay back from _subst.
		//		_subst:
		//			if an object of the r-grp becomes fully bound (how to know?), inject it in the out_grps.
		//			callback:
		//				store values for variables for this overlay.
		//				remove the binder from this overlay.
		//	if at least one binder matched:
		//		create an overlay (state prior to the match, i.e. rollback).
		//	if no more unbound values, kill the overlay.

		std::list<P<View> >::const_iterator	it;
		for(it=binders.begin();it!=binders.end();){

			((PGMController	*)(*it)->object)->take_input(input,this);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RGRPController::RGRPController(_Mem	*m,r_code::View	*rgrp_view):Controller(m,rgrp_view){

		RGroup		*rgrp=(RGroup	*)getObject();
		RGRPOverlay	*o=new	RGRPOverlay(this);
		
		//	init binders.
		UNORDERED_MAP<uint32,P<View> >::const_iterator	it;
		for(it=rgrp->ipgm_views.begin();it!=rgrp->ipgm_views.end();++it)
			o->binders.push_back(it->second);

		//	init bindings.
		for(it=rgrp->variable_views.begin();it!=rgrp->variable_views.end();++it)
			o->bindings[(Var	*)it->second->object]=NULL;

		overlays.push_back(o);
	}
	
	RGRPController::~RGRPController(){
	}

	void	RGRPController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		overlayCS.enter();

		//	Pass the inputs to all ipgm views' controllers in the rgrp.

		if(tsc>0){	// 1st overlay is the master (no match yet); other overlays are pushed back in order of their matching time. 
	
			// start from the last overlay, and erase all of them that are older than tsc.
			uint64	now=Now();
			Overlay	*master=*overlays.begin();
			Overlay	*current=overlays.back();
			while(current!=master){

				if(now-((RGRPOverlay	*)current)->birth_time>tsc){
					
					current->kill();
					overlays.pop_back();
					current=overlays.back();
				}else
					break;
			}
		}

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob	*j=new	ReductionJob(new	View(input),*o);
			mem->pushReductionJob(j);
		}

		overlayCS.leave();
	}
}