//	pgm_controller.cpp
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

#include	"pgm_controller.h"
#include	"mem.h"


namespace	r_exec{

	InputLessPGMController::InputLessPGMController(r_code::View	*ipgm_view):_PGMController(ipgm_view){

		overlays.push_back(new	InputLessPGMOverlay(this));
	}

	InputLessPGMController::~InputLessPGMController(){
	}

	void	InputLessPGMController::signal_input_less_pgm(){	//	next job will be pushed by the rMem upon processing the current signaling job, i.e. right after exiting this function.

		overlayCS.enter();
		if(overlays.size()){

			Overlay	*o=*overlays.begin();
			((InputLessPGMOverlay	*)o)->inject_productions(NULL);
			o->reset();

			if(!run_once){

				Group	*host=getView()->get_host();
				host->enter();
				if(getView()->get_act()>host->get_act_thr()		&&	//	active ipgm.
					host->get_c_act()>host->get_c_act_thr()		&&	//	c-active group.
					host->get_c_sln()>host->get_c_sln_thr()){		//	c-salient group.

					TimeJob	*next_job=new	InputLessPGMSignalingJob(this,Now()+tsc);
					_Mem::Get()->pushTimeJob(next_job);
				}
				host->leave();
			}
		}
		overlayCS.leave();

		if(run_once)
			kill();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_PGMController::_PGMController(r_code::View	*ipgm_view):Controller(ipgm_view){

		run_once=!ipgm_view->object->code(IPGM_RUN).asBoolean();
	}

	_PGMController::~_PGMController(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PGMController::PGMController(r_code::View	*ipgm_view):_PGMController(ipgm_view){

		overlays.push_back(new	PGMOverlay(this));

		Code	*pgm=getObject()->get_reference(0);
		uint16	prod_index=pgm->code(PGM_PRODS).asIndex();
		uint16	command_count=pgm->code(prod_index).getAtomCount();
		can_sim=true;
		for(uint16	i=1;i<=command_count;++i)
			if(pgm->code(pgm->code(prod_index+i).asIndex()+1).asOpcode()!=Opcodes::Inject){

				can_sim=false;
				break;
			}
	}

	PGMController::~PGMController(){
	}

	void	PGMController::add(Overlay	*overlay){	//	the first overlay is the master.
													//	the last overlay is the oldest, the one after the master is the youngest.
		overlayCS.enter();
		if(overlays.size()==1)
			overlays.push_back(overlay);
		else{

			std::list<P<_Overlay> >::iterator	master=overlays.begin();
			overlays.insert(++master,overlay);
		}
		overlayCS.leave();
	}

	void	PGMController::notify_reduction(){

		if(run_once)
			kill();
	}

	void	PGMController::take_input(r_exec::View	*input,Controller	*origin){	//	origin unused since there is no recursion here.

		if(input->object->get_pred()	||	input->object->get_goal())
			return;
		if(!can_sim	&&	(input->object->get_hyp()	||	input->object->get_sim()	||	input->object->get_asmp()))
			return;

		overlayCS.enter();

		if(tsc>0){	// the first overlay is the master (no match yet); other overlays are pushed right after the master in order of their matching time.
			
			// start from the last overlay (the oldest), and erase all of them that are older than tsc.
			uint64	now=Now();
			Overlay	*master=overlays.front();

			if(run_once	&&	now-((PGMOverlay	*)master)->birth_time>tsc){

				overlayCS.leave();
				kill();
				return;
			}else{

				Overlay	*current=overlays.back();
				while(current!=master){

					if(now-((PGMOverlay	*)current)->birth_time>tsc){
						
						current->kill();
						overlays.pop_back();
						current=overlays.back();
					}else
						break;
				}
			}
		}

		std::list<P<_Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob<_Overlay>	*j=new	ReductionJob<_Overlay>(new	View(input),*o);
			_Mem::Get()->pushReductionJob(j);
		}

		overlayCS.leave();
	}

	void	PGMController::take_input(r_exec::View	*input,Overlay	*source){	//	called from an r-grp overlay. Perform the reduction immediately (no reduction job pushed).
																				//	no tsc check.
		overlayCS.enter();

		std::list<P<_Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o)
			((PGMOverlay	*)(*o))->reduce(input,source);

		overlayCS.leave();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AntiPGMController::AntiPGMController(r_code::View	*ipgm_view):_PGMController(ipgm_view),successful_match(false){

		overlays.push_back(new	AntiPGMOverlay(this));
	}

	AntiPGMController::~AntiPGMController(){
	}

	void	AntiPGMController::take_input(r_exec::View	*input,Controller	*origin){

		if(input->object->get_pred()	||	input->object->get_goal())
			return;

		if(this!=origin)
			overlayCS.enter();

		std::list<P<_Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob<_Overlay>	*j=new	ReductionJob<_Overlay>(new	View(input),*o);
			_Mem::Get()->pushReductionJob(j);
		}

		if(this!=origin)
			overlayCS.leave();
	}

	void	AntiPGMController::signal_anti_pgm(){

		overlayCS.enter();

		if(successful_match)	//	a signaling job has been spawn in restart(): we are here in an old job during which a positive match occurred: do nothing.
			successful_match=false;
		else{	//	no positive match during this job: inject productions and restart.

			AntiPGMOverlay	*overlay=(AntiPGMOverlay	*)*overlays.begin();
			overlay->reductionCS.enter();
			overlay->inject_productions(this);	//	eventually calls take_input(): origin set to this to avoid a deadlock on overlayCS.
			overlay->reset();
			overlay->reductionCS.leave();
			
			if(!run_once){

				push_new_signaling_job();

				std::list<P<_Overlay> >::const_iterator	first=overlays.begin();
				std::list<P<_Overlay> >::const_iterator	o;
				for(o=++first;o!=overlays.end();){	//	reset the first overlay and kill all others.

					((Overlay	*)(*o))->kill();
					o=overlays.erase(o);
				}
			}
		}

		overlayCS.leave();

		if(run_once)
			kill();
	}

	void	AntiPGMController::restart(AntiPGMOverlay	*overlay){	//	one anti overlay matched all its inputs, timings and guards: push a new signaling job, 
																	//	reset the overlay and kill all others.
		overlayCS.enter();
		
		overlay->reset();
		
		push_new_signaling_job();

		std::list<P<_Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();){

			if(overlay!=*o){

				((AntiPGMOverlay	*)*o)->kill();
				o=overlays.erase(o);
			}else
				++o;
		}

		successful_match=true;

		overlayCS.leave();
	}

	void	AntiPGMController::push_new_signaling_job(){

		Group	*host=getView()->get_host();
		host->enter();
		if(getView()->get_act()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()	&&	//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){	//	c-salient group.

			host->leave();
			TimeJob	*next_job=new	AntiPGMSignalingJob(this,Now()+Utils::GetTimestamp<Code>(getObject(),IPGM_TSC));
			_Mem::Get()->pushTimeJob(next_job);
		}else
			host->leave();
	}
}