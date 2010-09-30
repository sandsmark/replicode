//	time_job.h
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

#ifndef	time_job_h
#define	time_job_h

#include	"group.h"
#include	"pgm_overlay.h"


namespace	r_exec{
	
	class	r_exec_dll	TimeJob:
	public	_Object{
	protected:
		TimeJob(uint64	ijt);
	public:
		int64			target_time;		//	0 means ASAP.
		virtual	bool	update(_Mem	*m)=0;	//	return false to shutdown the time core.
		virtual	bool	is_alive()	const;
	};

	class	r_exec_dll	UpdateJob:
	public	TimeJob{
	public:
		P<Group>	group;
		UpdateJob(Group	*g,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	SignalingJob:
	public	TimeJob{
	protected:
		SignalingJob(Controller	*o,uint64	ijt);
	public:
		P<Controller>	controller;
		bool	is_alive()	const;
	};

	class	r_exec_dll	AntiPGMSignalingJob:
	public	SignalingJob{
	public:
		AntiPGMSignalingJob(AntiPGMController	*o,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	InputLessPGMSignalingJob:
	public	SignalingJob{
	public:
		InputLessPGMSignalingJob(InputLessPGMController	*o,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	InjectionJob:
	public	TimeJob{
	public:
		P<View>	view;
		InjectionJob(View	*v,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	EInjectionJob:
	public	TimeJob{
	public:
		P<View>	view;
		EInjectionJob(View	*v,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	GInjectionJob:
	public	TimeJob{
	public:
		P<View>		view;
		P<Group>	group;
		P<Group>	host;
		GInjectionJob(View	*v,Group	*g,Group	*h,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	SaliencyPropagationJob:
	public	TimeJob{
	public:
		P<Code>	object;
		float32		sln_change;
		float32		source_sln_thr;
		SaliencyPropagationJob(Code	*o,float32	sln_change,float32	source_sln_thr,uint64	ijt);
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	ShutdownTimeCore:
	public	TimeJob{
	public:
		ShutdownTimeCore();
		bool	update(_Mem	*m);
	};

	class	r_exec_dll	SuspendTimeCore:
	public	TimeJob{
	public:
		SuspendTimeCore();
		bool	update(_Mem	*m);
	};
}


#endif