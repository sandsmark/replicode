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
		while(run){

			P<TimeJob>	j=_Mem::Get()->popTimeJob();
			if(j==NULL)
				break;
			if(!j->is_alive()){

				j=NULL;
				continue;
			}

			uint64	target=j->target_time;
			uint64	next_target=0;
			if(target==0)	// 0 means ASAP. Control jobs (shutdown) are caught here.
				run=j->update(next_target);
			else{

				int64	time_to_wait=target-Now();
				if(time_to_wait==0)	// right on time: do the job.
					run=j->update(next_target);
				else	if(time_to_wait>0){	// early: spawn a delegate to wait for the due time; delegate will die when done.

					DelegatedCore	*d=new	DelegatedCore(time_to_wait,j);
					d->start(DelegatedCore::Wait);
					_Mem::Get()->register_time_job_latency(time_to_wait);
					next_target=0;
				}else{	// late: do the job and report.

					run=j->update(next_target);
					std::cout<<"1 Time Core report: late on target: "<<-time_to_wait<<" us behind."<<std::endl;
				}
			}

			while(next_target	&&	run){

				if(!j->is_alive())
					break;

				uint64	time_to_wait=next_target-Now();
				next_target=0;
				if(time_to_wait==0)	// right on time: do the job.
					run=j->update(next_target);
				else	if(time_to_wait>0){	// early: spawn a delegate to wait for the due time; delegate will die when done.
											// the delegate will handle the next target when it is known (call to update()).
					DelegatedCore	*d=new	DelegatedCore(time_to_wait,j);
					d->start(DelegatedCore::Wait);
				}else{	// late: do the job and report.

					run=j->update(next_target);
					std::cout<<"2 Time Core report: late on target: "<<-time_to_wait<<" us behind."<<std::endl;
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

	thread_ret thread_function_call	DelegatedCore::Wait(void	*args){

		_Mem::Get()->start_core();
		DelegatedCore	*_this=((DelegatedCore	*)args);

		uint64	time_to_wait=_this->time_to_wait;

wait:	_this->timer.start(time_to_wait);
		_this->timer.wait();

		if(!_this->job->is_alive())
			goto	end;

		uint64	next_target=0;
		if(_Mem::Get()->check_state()==_Mem::RUNNING)	// checks for shutdown that could have happened during the wait on timer.
			_this->job->update(next_target);

redo:	if(next_target){

			if(!_this->job->is_alive())
				goto	end;
			if(_Mem::Get()->check_state()!=_Mem::RUNNING)	// checks for shutdown that could have happened during the last update().
				goto	end;

			next_target=0;
			if(time_to_wait==0){	// right on time: do the job.

				_this->job->update(next_target);
				goto	redo;
			}else	if(time_to_wait<0){

				_this->job->update(next_target);
				std::cout<<"3 Time Core report: late on target: "<<-time_to_wait<<" us behind."<<std::endl;
				goto	redo;
			}else
				goto	wait;
		}

end:	_Mem::Get()->shutdown_core();
		delete	_this;
		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DelegatedCore::DelegatedCore(uint64	time_to_wait,TimeJob	*j):Thread(),time_to_wait(time_to_wait),job(j){
	}

	DelegatedCore::~DelegatedCore(){
	}
}