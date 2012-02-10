//	time_job.cpp
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

#include	"time_job.h"
#include	"pgm_controller.h"
#include	"mem.h"


namespace	r_exec{

	TimeJob::TimeJob(uint64	ijt):_Object(),target_time(ijt){
	}

	bool	TimeJob::is_alive()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////

	UpdateJob::UpdateJob(Group	*g,uint64	ijt):TimeJob(ijt){

		group=g;
	}

	bool	UpdateJob::update(uint64	&next_target){

		_Mem::Get()->update(group);
		return	true;
	}

	////////////////////////////////////////////////////////////

	SignalingJob::SignalingJob(View	*v,uint64	ijt):TimeJob(ijt){

		view=v;
	}

	bool	SignalingJob::is_alive()	const{

		return	view->controller->is_alive();
	}

	////////////////////////////////////////////////////////////

	AntiPGMSignalingJob::AntiPGMSignalingJob(View	*v,uint64	ijt):SignalingJob(v,ijt){
	}

	bool	AntiPGMSignalingJob::update(uint64	&next_target){

		if(is_alive())
			((AntiPGMController	*)view->controller)->signal_anti_pgm();
		return	true;
	}

	////////////////////////////////////////////////////////////

	InputLessPGMSignalingJob::InputLessPGMSignalingJob(View	*v,uint64	ijt):SignalingJob(v,ijt){
	}

	bool	InputLessPGMSignalingJob::update(uint64	&next_target){

		if(is_alive())
			((InputLessPGMController	*)view->controller)->signal_input_less_pgm();
		return	true;
	}

	////////////////////////////////////////////////////////////

	InjectionJob::InjectionJob(View	*v,uint64	ijt):TimeJob(ijt){

		view=v;
	}

	bool	InjectionJob::update(uint64	&next_target){

		_Mem::Get()->inject(view);
		return	true;
	}

	////////////////////////////////////////////////////////////

	EInjectionJob::EInjectionJob(View	*v,uint64	ijt):TimeJob(ijt){

		view=v;
	}

	bool	EInjectionJob::update(uint64	&next_target){

		_Mem::Get()->inject_existing_object(view,view->object,view->get_host(),true);
		return	true;
	}

	////////////////////////////////////////////////////////////

	SaliencyPropagationJob::SaliencyPropagationJob(Code	*o,float32	sln_change,float32	source_sln_thr,uint64	ijt):TimeJob(ijt),sln_change(sln_change),source_sln_thr(source_sln_thr){

		object=o;
	}
	
	bool	SaliencyPropagationJob::update(uint64	&next_target){

		if(!object->is_invalidated())
			_Mem::Get()->propagate_sln(object,sln_change,source_sln_thr);
		return	true;
	}

	////////////////////////////////////////////////////////////

	ShutdownTimeCore::ShutdownTimeCore():TimeJob(0){
	}

	bool	ShutdownTimeCore::update(uint64	&next_target){

		return	false;
	}

	////////////////////////////////////////////////////////////

	PerfSamplingJob::PerfSamplingJob(uint32	period):TimeJob(0),period(period){
	}

	bool	PerfSamplingJob::update(uint64	&next_target){

		_Mem::Get()->inject_perf_stats();
		next_target=Now()+period;
		return	true;
	}

	bool	PerfSamplingJob::is_alive()	const{

		return	true;
	}
}