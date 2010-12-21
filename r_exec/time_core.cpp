//	time_core.cpp
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

#include	"time_core.h"
#include	"mem.h"
#include	"init.h"


namespace	r_exec{

	thread_ret thread_function_call	TimeCore::Run(void	*args){

		TimeCore	*_this=((TimeCore	*)args);

		bool	run=true;
		while(run){	//	enter a wait state when the rMem is suspended.

			P<TimeJob>	j=_Mem::Get()->popTimeJob();
			if(!j->is_alive()){

				j=NULL;
				continue;
			}

			uint64	target=j->target_time;
			if(target==0)	//	0 means ASAP. Control jobs (shutdown and suspend) are caught here.
				run=j->update();
			else{

				uint64	now=Now();
				int64	deadline=target-now;
				if(deadline==0)	//	right on time: do the job.
					run=j->update();
				else	if(deadline>0){	//	on time: spawn a delegate to wait for the due time; delegate will die when done.

					DelegatedCore	*d=new	DelegatedCore(deadline,j);
					d->start(DelegatedCore::Wait);
				}else{	//	we are late: do the job and report.

					run=j->update();
					std::cout<<"Time Core report: late on target: "<<-deadline/1000<<" ms behind."<<std::endl;
				}
			}

			j=NULL;
		}

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TimeCore::TimeCore():Thread(){
	}

	TimeCore::~TimeCore(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//	The rMem has to wait for delegates to be actually suspended before doing anything else (ex: getImage()) when they are executing update(_this->mem).
	//	When suspending the rMem, we do not want to have to wait for delegates caught in timer.wait().
	thread_ret thread_function_call	DelegatedCore::Wait(void	*args){

		_Mem::Get()->start_core();
		DelegatedCore	*_this=((DelegatedCore	*)args);

		_this->timer.start(_this->deadline);
		_this->timer.wait();

		_Mem::DState	s=_Mem::Get()->check_state();	//	checks for shutdown or suspension that could have happened during the wait on timer.
		switch(s){
		case	_Mem::D_RUNNING_AFTER_SUSPENSION:
			_this->job->update();
			break;
		case	_Mem::D_RUNNING:	//	suspension might occur now or during update(): the rMem has to wait for completion.
			_Mem::Get()->wait_for_delegate();
			_this->job->update();
			_Mem::Get()->delegate_done();
			break;
		case	_Mem::D_STOPPED:
		case	_Mem::D_STOPPED_AFTER_SUSPENSION:
			break;
		}

		_Mem::Get()->shutdown_core();
		delete	_this;
		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DelegatedCore::DelegatedCore(uint64	deadline,TimeJob	*j):Thread(),deadline(deadline),job(j){
	}

	DelegatedCore::~DelegatedCore(){
	}
}