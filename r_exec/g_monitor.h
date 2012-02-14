//	g_monitor.h
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

#ifndef	g_monitor_h
#define	g_monitor_h

#include	"monitor.h"


namespace	r_exec{

	class	PMDLController;
	class	PrimaryMDLController;

	class	_GMonitor:
	public	Monitor{
	protected:
		uint64	deadline;	// of the goal.
		uint64	sim_thz;
		_Fact	*goal_target;	// convenience; f1->object.
		P<Fact>	f_imdl;
		SimMode	sim_mode;

		uint32	volatile	simulating;	// 32 bits alignment.

		typedef	std::list<std::pair<P<Goal>,P<Sim> > >	SolutionList;

		class	SimOutcomes{
		public:
			SolutionList	mandatory_solutions;
			SolutionList	optional_solutions;
		};

		// Simulated predictions of any goal success resulting from the simulation of the monitored goal.
		SimOutcomes	sim_successes;
		SimOutcomes	sim_failures;

		void	store_simulated_outcome(Goal	*affected_goal,Sim	*sim,bool	success);	// store the outcomes of any goal affected by the simulation of the monitored goal.
		void	invalidate_sim_outcomes();

		_GMonitor(	PMDLController	*controller,
					BindingMap		*bindings,
					uint64			deadline,
					uint64			sim_thz,
					Fact			*goal,
					Fact			*f_imdl);	// goal is f0->g->f1->object.
	public:
		virtual	bool	signal(bool	simulation){	return	false;	}
	};

	// Monitors goals (other than requirements).
	// Use for SIM_ROOT.
	// Is aware of any predicted evidence for the goal target: if at construction time such an evidence is known, the goal is not injected.
	// Reporting a success or failure to the controller invalidates the goal; reporting a predicted success also does.
	// Reporting a predicted failure injects the goal if it has not been already, invalidates it otherwise (a new goal will be injected).
	// The monitor still runs after reporting a predicted success.
	// The monitor does not run anymore if the goal is invalidated (case of a predicted success, followed by a predicted failure).
	// Wait for the time horizon; in the meantime:
	//	actual inputs:
	//		if an input is an evidence for the target, report a success.
	//		if an input is a counter-evidence of the target, report a failure.
	//		if an input is a predicted evidence for the target, report a predicted success.
	//		if an input is a predicted counter-evidence for the target, report a predicted failure.
	//		if there is a predicted evidence for the target that becomes invalidated, report a predicted failure.
	//	simulated predictions: catch only those that are a simulation for the monitored goal.
	//		if an input is an evidence of the goal target, simulate a prediction of the goal success.
	//		if an input is an evidence of the goal target, simulate a prediction of the goal failure.
	//		store any simulated prediction of success/failure for any goal.
	// At the time horizon:
	//	simulation mode:
	//		commit to the appropriate solutions for the goal.
	//		mode become actual.
	//		time horizon becomes the goal deadline.
	//	actual mode
	//		if the goal is not invalidated, report a failure.
	class	GMonitor:
	public	_GMonitor{
	protected:
		_Fact	*volatile	predicted_evidence;	// f0->pred->f1->object; 32 bits alignment.
		bool	injected_goal;
		
		void	commit();
	public:
		GMonitor(	PMDLController	*controller,
					BindingMap		*bindings,
					uint64			deadline,
					uint64			sim_thz,
					Fact			*goal,
					Fact			*f_imdl,
					_Fact			*predicted_evidence);	// goal is f0->g->f1->object.

		virtual	bool	reduce(_Fact	*input);	// returning true will remove the monitor form the controller.
		virtual	void	update(uint64	&next_target);
	};

	// Monitors actual requirements.
	// Use for SIM_ROOT.
	// target==f_imdl; this means we need to fullfill some requirements:
	//	Wait until the deadline of the goal, in the meantime:
	//		each time the monitor is signalled (i.e. a new pred->f_imdl has been produced), check if chaining is allowed:
	//			if no, do nothing.
	//			if yes: assert success and abort: the model will bind its rhs with the bm retrieved from the pred->f_imdl; this will kill the monitor and a new one will be built for the bound rhs sub-goal.
	//	At the deadline, assert failure.
	class	RMonitor:
	public	GMonitor{
	public:
		RMonitor(	PrimaryMDLController	*controller,
					BindingMap				*bindings,
					uint64					deadline,
					uint64					sim_thz,
					Fact					*goal,
					Fact					*f_imdl);

		bool	reduce(_Fact	*input);
		void	update(uint64	&next_target);
		bool	signal(bool	simulation);
	};

	// Monitors simulated goals.
	class	SGMonitor:
	public	_GMonitor{
	protected:
		void	commit();
	public:
		SGMonitor(	PrimaryMDLController	*controller,
					BindingMap				*bindings,
					uint64					sim_thz,
					Fact					*goal,
					Fact					*f_imdl);	// goal is f0->g->f1->object.

		bool	reduce(_Fact	*input);
		void	update(uint64	&next_target);
	};

	// Monitors simulated requirements.
	// Use for SIM_OPTIONAL and SIM_MANDATORY.
	class	SRMonitor:
	public	SGMonitor{
	public:
		SRMonitor(	PrimaryMDLController	*controller,
					BindingMap				*bindings,
					uint64					sim_thz,
					Fact					*goal,
					Fact					*f_imdl);

		bool	reduce(_Fact	*input);
		void	update(uint64	&next_target);
		bool	signal(bool	simulation);
	};

	// Case A: target==actual goal and target!=f_imdl: simulations have been produced for all sub-goals.
	//	Wait until the STHZ, in the meantime:
	//		if input==goal target, assert success and abort: invalidate goal (this will invalidate the related simulations).
	//		if input==|goal target, assert failure and abort: the super-goal will probably fail and so on, until some drives fail, which will trigger their re-injection.
	//		if input==pred goal target, do nothing.
	//		if input==pred |goal target, do nothing.
	//		if input==pred success/failure of any other goal and sim->super-goal==goal, store the simulation for decision at STHZ.
	//		if input==pred goal target and sim->super-goal==goal, store the simulation for decision at STHZ.
	//		if input==pred |goal target and sim->super-goal==goal, store the simulation for decision at STHZ.
	//		if input==pred goal target and sim->super-goal!=goal, predict success for the goal.
	//		if input==pred |goal target and sim->super-goal!=goal, predict failure for the goal.
	//	At STHZ, choose the best simulations if any, and commit to their sub-goals; kill the predictions for the discarded simulations.
	//
	// Case B: target==f_imdl; this means we need to fullfill some requirements: simulations have been produced for all the sub-goals of f_imdl.
	//	Wait until the STHZ, in the meantime:
	//		if input==pred success/failure of any goal and sim->super-goal==goal, store the simulation for decision at STHZ.
	//	At STHZ, choose the best simulations if any, and commit to their sub-goals; kill the predictions for the discarded simulations.
	//
	// Case C: target==simulated goal and target!=f_imdl: simulations have been produced for all sub-goals.
	//	Wait until the STHZ, in the meantime:
	//		if input==goal target, predict success for the goal and abort: invalidate goal.
	//		if input==|goal target, predict failure for the goal and abort: invalidate goal.
	//		if input==pred success/failure of any other goal and sim->super-goal==goal, store the simulation for decision at STHZ.
	//		if input==pred goal target and sim->super-goal==goal, store the simulation for decision at STHZ.
	//		if input==pred |goal target and sim->super-goal==goal, store the simulation for decision at STHZ.
	//		if input==pred goal target and sim->super-goal!=goal, predict success for the goal.
	//		if input==pred |goal target and sim->super-goal!=goal, predict failure for the goal.
	//
	// Case D: target==simulated f_imdl; this means we need to fullfill some requirements: simulations have been produced for all the sub-goals of f_imdl.
	//	Wait until the STHZ, in the meantime:
	//		if input==pred success/failure of any goal and sim->super-goal==goal, store the simulation for decision at STHZ.
	//	At STHZ, choose the best simulations if any, and commit to their sub-goals; kill the predictions for the discarded simulations.
}


#endif