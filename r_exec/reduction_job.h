//	reduction_job.h
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

#ifndef	reduction_job_h
#define	reduction_job_h

#include	"overlay.h"
#include	"object.h"


namespace	r_exec{

	class	r_exec_dll	_ReductionJob:
	public	_Object{
	protected:
		_ReductionJob();
	public:
		uint64	ijt;	// time of injection of the job in the pipe.
		virtual	bool	update(uint64	now)=0;	//	return false to shutdown the reduction core.
		virtual	void	debug(){}
	};

	template<class	_P>	class	ReductionJob:
	public	_ReductionJob{
	public:
		P<View>	input;
		P<_P>	processor;
		ReductionJob(View	*input,_P	*processor):_ReductionJob(),input(input),processor(processor){}
		bool	update(uint64	now){
			
			_Mem::Get()->register_reduction_job_latency(now-ijt);
			processor->reduce(input);
			return	true;
		}
		void	debug(){

			processor->debug(input);
		}
	};

	template<class	_P,class	T,class	C>	class	BatchReductionJob:
	public	_ReductionJob{
	public:
		P<_P>	processor;	// the controller that will process the job.
		P<T>	trigger;	// the event that triggered the job.
		P<C>	controller;	// the controller that produced the job.
		BatchReductionJob(_P	*processor,T	*trigger,C	*controller):_ReductionJob(),processor(processor),trigger(trigger),controller(controller){}
		bool	update(uint64	now){
			
			_Mem::Get()->register_reduction_job_latency(now-ijt);
			processor->reduce_batch(trigger,controller);
			return	true;
		}
	};

	class	r_exec_dll	ShutdownReductionCore:
	public	_ReductionJob{
	public:
		bool	update(uint64	now);
	};

	class	r_exec_dll	AsyncInjectionJob:
	public	_ReductionJob{
	public:
		P<View>	input;
		AsyncInjectionJob(View	*input):_ReductionJob(),input(input){}
		bool	update(uint64	now);
	};
}


#endif