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

		std::cout<<"Time Core created.\n";

		while(_this->mem->check_state(false)){	//	enter a wait state when the rMem is suspended.

			TimeJob	j=_this->mem->popTimeJob();
			if(!j.is_alive())
				continue;

			uint64	target=j.target_time;
			if(target==0)	//	0 means ASAP.
				j.job->update(_this->mem);
			else{

				uint64	now=Now();
				int64	deadline=target-now;
				if(deadline>=0)	//	on time: spawn a delegate to wait for the due time; delegate will die when done.
					_this->mem->add_delegate(deadline,j.job);
				else{	//	we are late: do the job and report.

					j.job->update(_this->mem);
					std::cout<<"Time Core report: late on target: "<<-deadline/1000<<" ms behind."<<std::endl;
				}
			}
		}

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TimeCore::TimeCore(_Mem	*m):Thread(),mem(m){
	}

	TimeCore::~TimeCore(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	thread_ret thread_function_call	DelegatedCore::Wait(void	*args){

		DelegatedCore	*_this=((DelegatedCore	*)args);

		uint64	init_time=Now();
		if(_this->mem->check_state(true)){	//	enter a wait state when the rMem is suspended.

			uint64	now=Now();
			_this->deadline-=(now-init_time);
			if(_this->deadline>0){

				_this->timer.start(_this->deadline);
				_this->timer.wait();

				if(!_this->mem->check_state(true)){

					delete	_this;
					thread_ret_val(0);
				}
			}
			
			_this->job->update(_this->mem);
			_this->mem->remove_delegate(_this);
		}else
			delete	_this;

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DelegatedCore::DelegatedCore(_Mem	*m,uint64	deadline,_TimeJob	*j):Thread(),mem(m),deadline(deadline),job(j){
	}

	DelegatedCore::~DelegatedCore(){
	}
}