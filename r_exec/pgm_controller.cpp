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

	_PGMController::_PGMController(r_code::View	*ipgm_view):OController(ipgm_view){

		run_once=!ipgm_view->object->code(IPGM_RUN).asBoolean();
	}

	_PGMController::~_PGMController(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	InputLessPGMController::InputLessPGMController(r_code::View	*ipgm_view):_PGMController(ipgm_view){

		overlays.push_back(new	InputLessPGMOverlay(this));
	}

	InputLessPGMController::~InputLessPGMController(){
	}

	void	InputLessPGMController::signal_input_less_pgm(){	// next job will be pushed by the rMem upon processing the current signaling job, i.e. right after exiting this function.

		reductionCS.enter();
		if(overlays.size()){

			Overlay	*o=*overlays.begin();
			((InputLessPGMOverlay	*)o)->inject_productions();
			o->reset();

			if(!run_once){

				if(is_alive()){

					Group	*host=getView()->get_host();
					host->enter();
					if(	host->get_c_act()>host->get_c_act_thr()		&&	// c-active group.
						host->get_c_sln()>host->get_c_sln_thr()){		// c-salient group.

						host->leave();

						TimeJob	*next_job=new	InputLessPGMSignalingJob((r_exec::View*)view,Now()+tsc);
						_Mem::Get()->pushTimeJob(next_job);
					}else
						host->leave();
				}
			}
		}
		reductionCS.leave();

		if(run_once)
			invalidate();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PGMController::PGMController(r_code::View	*ipgm_view):_PGMController(ipgm_view){

		overlays.push_back(new	PGMOverlay(this));
	}

	PGMController::~PGMController(){
	}

	void	PGMController::notify_reduction(){

		if(run_once)
			invalidate();
	}

	void	PGMController::take_input(r_exec::View	*input){

		Controller::__take_input<PGMController>(input);
	}

	void	PGMController::reduce(r_exec::View	*input){

		std::list<P<Overlay> >::const_iterator	o;
		uint32	oid=input->object->get_oid();
		//uint64	t=Now()-Utils::GetTimeReference();
		//std::cout<<Time::ToString_seconds(t)<<" got "<<oid<<" "<<input->get_sync()<<std::endl;
		if(tsc>0){

			reductionCS.enter();
			uint64	now=Now();	// call must be located after the CS.enter() since (*o)->reduce() may update (*o)->birth_time.
			//uint64	t=now-Utils::GetTimeReference();
			for(o=overlays.begin();o!=overlays.end();){

				if((*o)->is_invalidated())
					o=overlays.erase(o);
				else{

					uint64	birth_time=((PGMOverlay	*)*o)->get_birth_time();
					if(birth_time>0	&&	now-birth_time>tsc){
						//std::cout<<Time::ToString_seconds(t)<<" kill "<<std::hex<<(void	*)*o<<std::dec<<" born: "<<Time::ToString_seconds(birth_time-Utils::GetTimeReference())<<" after "<<Time::ToString_seconds(now-birth_time)<<std::endl;
						//std::cout<<std::hex<<(void	*)*o<<std::dec<<" ------------kill "<<input->object->get_oid()<<" ignored "<<std::endl;
						o=overlays.erase(o);
					}else{
						//void	*oo=*o;
						Overlay	*offspring=(*o)->reduce(input);
						if(offspring){
							overlays.push_front(offspring);
							//std::cout<<Time::ToString_seconds(t)<<" "<<std::hex<<oo<<std::dec<<" born: "<<Time::ToString_seconds(((PGMOverlay	*)oo)->get_birth_time()-Utils::GetTimeReference())<<" reduced "<<input->object->get_oid()<<" "<<input->get_sync()<<" offspring: "<<std::hex<<offspring<<std::dec<<std::endl;
							//std::cout<<std::hex<<(void	*)oo<<std::dec<<" --------------- reduced "<<input->object->get_oid()<<" "<<input->get_sync()<<std::endl;
						}
						if(!is_alive())
							break;
						++o;
					}
				}
			}
			reductionCS.leave();
		}else{

			reductionCS.enter();
			for(o=overlays.begin();o!=overlays.end();){

				if((*o)->is_invalidated())
					o=overlays.erase(o);
				else{

					Overlay	*offspring=(*o)->reduce(input);
					if(offspring)
						overlays.push_front(offspring);
					if(!is_alive())
						break;
					++o;
				}
					
			}
			reductionCS.leave();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AntiPGMController::AntiPGMController(r_code::View	*ipgm_view):_PGMController(ipgm_view),successful_match(false){

		overlays.push_back(new	AntiPGMOverlay(this));
	}

	AntiPGMController::~AntiPGMController(){
	}

	void	AntiPGMController::take_input(r_exec::View	*input){

		Controller::__take_input<AntiPGMController>(input);
	}

	void	AntiPGMController::reduce(r_exec::View	*input){

		reductionCS.enter();
		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();){

			if((*o)->is_invalidated())
				o=overlays.erase(o);
			else{

				Overlay	*offspring=(*o)->reduce(input);
				if(successful_match){	//	the controller has been restarted: reset the overlay and kill all others

					Overlay	*overlay=*o;
					overlay->reset();
					overlays.clear();
					overlays.push_back(overlay);
					successful_match=false;
					break;
				}
				++o;
				if(offspring)
					overlays.push_front(offspring);
			}
		}
		reductionCS.leave();
	}

	void	AntiPGMController::signal_anti_pgm(){

		reductionCS.enter();
		if(successful_match)	//	a signaling job has been spawn in restart(): we are here in an old job during which a positive match occurred: do nothing.
			successful_match=false;
		else{	//	no positive match during this job: inject productions and restart.

			Overlay	*overlay=*overlays.begin();
			((AntiPGMOverlay	*)overlay)->inject_productions();
			overlay->reset();	//	reset the first overlay and kill all others.
			if(!run_once	&&	is_alive()){

				overlays.clear();
				overlays.push_back(overlay);
			}
		}
		reductionCS.leave();

		if(run_once)
			invalidate();
	}

	void	AntiPGMController::restart(){	//	one anti overlay matched all its inputs, timings and guards. 

		push_new_signaling_job();
		successful_match=true;
	}

	void	AntiPGMController::push_new_signaling_job(){

		Group	*host=getView()->get_host();
		host->enter();
		if(getView()->get_act()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()	&&	//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){	//	c-salient group.

			host->leave();
			TimeJob	*next_job=new	AntiPGMSignalingJob((r_exec::View*)view,Now()+Utils::GetTimestamp<Code>(getObject(),IPGM_TSC));
			_Mem::Get()->pushTimeJob(next_job);
		}else
			host->leave();
	}
}