//	mem.cpp
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

#include	"mem.h"

namespace	r_exec{

	_Mem::_Mem():state(NOT_STARTED){
	}

	_Mem::~_Mem(){

		if(state==STARTED)
			reset();
		root=NULL;
	}

	void	_Mem::init(uint32	base_period,	//	in us; same for upr, spr and res.
						uint32	reduction_core_count,
						uint32	time_core_count){

		this->base_period=base_period;
		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;
	}

	void	_Mem::reset(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		std::list<DelegatedCore	*>::const_iterator	d;
		for(d=delegates.begin();d!=delegates.end();++d)
			delete	*d;

		delegates.clear();

		delete	object_register_sem;
		delete	objects_sem;
		delete	state_sem;

		delete	reduction_job_queue;
		delete	time_job_queue;
	}

	_Mem::State	_Mem::get_state(){

		state_sem->acquire();
		State	s=state;
		state_sem->release();

		return	s;
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::add_delegate(uint64	dealine,_TimeJob	*j){

		state_sem->acquire();
		if(state==STARTED){

			DelegatedCore	*d=new	DelegatedCore(this,dealine,j);
			d->position=delegates.insert(delegates.end(),d);
			d->start(DelegatedCore::Wait);
		}
		state_sem->release();
	}

	void	_Mem::remove_delegate(DelegatedCore	*core){

		state_sem->acquire();
		delegates.erase(core->position);
		delete	core;
		state_sem->release();
	}

	void	_Mem::stop(){

		state_sem->acquire();
		state=STOPPED;

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			Thread::TerminateAndWait(reduction_cores[i]);
		for(i=0;i<time_core_count;++i)
			Thread::TerminateAndWait(time_cores[i]);

		std::list<DelegatedCore	*>::const_iterator	d;
		for(d=delegates.begin();d!=delegates.end();++d)
			Thread::TerminateAndWait(*d);

		state_sem->release();
		reset();
	}

	////////////////////////////////////////////////////////////////

	ReductionJob	_Mem::popReductionJob(){

		return	reduction_job_queue->pop();
	}

	void	_Mem::pushReductionJob(ReductionJob	j){

		reduction_job_queue->push(j);
	}

	TimeJob	_Mem::popTimeJob(){

		return	time_job_queue->pop();
	}

	void	_Mem::pushTimeJob(TimeJob	j){

		time_job_queue->push(j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::update(UpdateJob	*j){

		update((Group	*)j->group);
	}

	void	_Mem::update(AntiPGMSignalingJob	*j){

		j->controller->signal_anti_pgm(this);
	}

	void	_Mem::update(InputLessPGMSignalingJob	*j){

		j->controller->signal_input_less_pgm(this);
	}

	void	_Mem::update(InjectionJob	*j){

		injectNow(j->view);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(LObject	*object,uint16	nodeID){
	}
}