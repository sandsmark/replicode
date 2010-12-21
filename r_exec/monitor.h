//	monitor.h
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

#ifndef	monitor_h
#define	monitor_h

#include	"model.h"
#include	"rgrp_overlay.h"


namespace	r_exec{

	class	FwdController;

	//	Monitors one prediction.
	//	Discards pred, asmp/hyp and sim inputs.
	//	If what is expected occurs on time, register a success and dismiss the monitor (positive match).
	//	If the opposite of what is expected occurs on time (i.e. fact when expecting a |fact or vice-versa), register a failure,
	//	but keep the monitor alive (negative match).
	//	If at the deadline (expected_time_high), no match has occurred (positive or negative), generate a fact or |fact (depending on what is expected).
	class	PMonitor:
	public	_Object{
	private:
		FwdController	*controller;
		uint64			expected_time_high;
		uint64			expected_time_low;
		bool			success;	//	set to true when a match (positive or negative) for the target was found in due time.
	public:
		P<Code>	target;	//	mk.pred.

		PMonitor(FwdController	*c,Code	*target,uint64	expected_time,uint64	time_tolerance);

		bool	is_alive();
		bool	take_input(r_exec::View	*input);
		void	update();	//	called by monitoring jobs.
	};

	//	Monitors a set of goals, each held by g monitors.
	//	Held by forward controllers.
	//	If what is expected occurs on time, register a success and dismiss the monitor (positive match).
	//	If at the deadline (tsc), some goals have not been matched, register a failure.
	//	For these unmatched goals, generate a fact or |fact (depending on what is expected).
	//	If aéé goals are matched, propagate success to the parent monitor.
	class	GSMonitor:
	public	BindingOverlay{
	private:
		Model			*inv_model;
		P<GSMonitor>	parent;

		std::list<P<Code> >	goals;	//	protected by the inherited reductionCS.

		static	void	KillSubGoals(Code	*mk_goal);
		static	void	KillSuperGoals(Code	*mk_goal);
		static	void	KillRelatedGoals(Code	*mk_goal);
	public:
		GSMonitor(Model	*inv_model,FwdController	*c,GSMonitor	*parent,RGroup	*group,BindingMap	*bindings,uint8	reduction_mode);
		GSMonitor(GSMonitor	*original);
		~GSMonitor();

		RGroup	*get_rgrp()	const;

		bool	take_input(r_exec::View	*input);
		void	update();	//	called by monitoring jobs.

		typedef	enum{
			SUCCESS=0,
			FAILURE=1,
			INVALIDATED=2
		}Outcome;
		void	add_outcome(Outcome	outcome,float32	confidence);

		void	add_goal(Code	*goal);
		void	remove_goal(Code	*goal);

		Code	*get_mk_sim(Code	*object)	const;
		Code	*get_mk_asmp(Code	*object)	const;
	};
}


#include	"monitor.inline.cpp"


#endif