//	mdl_controller.h
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

#ifndef	mdl_controller_h
#define	mdl_controller_h

#include	"hlp_overlay.h"
#include	"hlp_controller.h"
#include	"p_monitor.h"
#include	"factory.h"


namespace	r_exec{

	class	MDLOverlay:
	public	HLPOverlay{
	protected:
		MDLOverlay(Controller	*c,const	HLPBindingMap	*bindngs);
	public:
		~MDLOverlay();

		void	load_patterns();

		virtual	Overlay	*reduce(_Fact	*input,Fact	*f_p_f_imdl,MDLController	*req_controller)=0;
	};

	class	PrimaryMDLOverlay:
	public	MDLOverlay{
	protected:
		bool	check_simulated_chaining(HLPBindingMap	*bm,Fact	*f_imdl,Pred	*prediction);
	public:
		PrimaryMDLOverlay(Controller	*c,const	HLPBindingMap	*bindngs);
		~PrimaryMDLOverlay();

		Overlay	*reduce(_Fact	*input,Fact	*f_p_f_imdl,MDLController	*req_controller);
	};

	class	SecondaryMDLOverlay:
	public	MDLOverlay{
	public:
		SecondaryMDLOverlay(Controller	*c,const	HLPBindingMap	*bindngs);
		~SecondaryMDLOverlay();

		Overlay	*reduce(_Fact	*input,Fact	*f_p_f_imdl,MDLController	*req_controller);
	};

	class	MDLController;
	class	Requirements{
	public:
		std::vector<P<MDLController>	>	controllers;
		P<_Fact>							f_imdl;	// f1 as in f0->pred->f1->imdl.
		bool								chaining_was_allowed;
	};
	typedef	std::pair<Requirements,Requirements>	RequirementsPair;	// first: wr, second: sr.

	// Requirements don't monitor their predictions: they don't inject any; instead, they store a f->imdl in the controlled model controllers (both primary and secondary), thus, no success injected for the productions of requirements.
	// Models controlled by requirements maintain for each prediction they make, a list of all the controllers of the requirements having allowed/inhibited said prediction.
	// P-monitors (associated to non-requirement models) propagate the outcome to the controllers associated with the prediction they monitor.
	//
	// Predictions and goals are injected in the primary group only.
	// Simulations are injected in the primary group only; no mk.rdx.
	//
	// Each time a prediction is made by a non-req model, a f->imdl is injected in both primary and secondary groups. If the input was a prediction, f->pred->f->imdl is injected instead.
	// f->imdl can also be tagged as simulations.
	//
	// Successes and failures are injected only in output groups.
	//
	// If forward chaining is inhibited (by strong reqs with better cfd than weak reqs), predictions are still produced, but not injected (no mk.rdx): this is to allow the rating of the requirements.
	class	MDLController:
	public	HLPController{
	protected:
		class	REntry:	// use for requirements.
		public	PEEntry{
		public:
			P<MDLController>	controller;	// of the requirement.
			bool				chaining_was_allowed;

			REntry();
			REntry(_Fact	*f_p_f_imdl,MDLController	*c,bool	chaining_was_allowed);	// f_imdl is f0 as in f0->pred->f1->imdl.

			bool	is_out_of_range(uint64	now)	const{	return	(before<now	||	after>now);	}
		};

		class	RCache{
		public:
			CriticalSection	CS;
			r_code::list<REntry>	positive_evidences;
			r_code::list<REntry>	negative_evidences;
		};

		RCache	requirements;
		RCache	simulated_requirements;

		void	_store_requirement(r_code::list<REntry>	*cache,REntry	&e);

		CriticalSection				p_monitorsCS;
		r_code::list<P<PMonitor> >	p_monitors;

		P<Code>	lhs;
		P<Code>	rhs;

		static	const	uint32	LHSController=0;
		static	const	uint32	RHSController=1;

		typedef	enum{
			NaR=0,
			WR=1,
			SR=2
		}RType;

		RType	_is_requirement;
		bool	_is_reuse;
		bool	_is_cmd;

		float32	get_cfd()	const;

		CriticalSection	active_requirementsCS;
		UNORDERED_MAP<P<_Fact>,RequirementsPair,PHash<_Fact> >	active_requirements;	// P<_Fact>: f1 as in f0->pred->f1->imdl; requirements having allowed the production of prediction; first: wr, second: sr.		

		template<class	C>	void	reduce_cache(Fact	*f_p_f_imdl,MDLController	*controller){	// fwd; controller is the controller of the requirement which produced f_p_f_imdl.

			BatchReductionJob<C,Fact,MDLController>	*j=new	BatchReductionJob<C,Fact,MDLController>((C	*)this,f_p_f_imdl,controller);
			_Mem::Get()->pushReductionJob(j);
		}

		template<class	E>	void	reduce_cache(Cache<E>	*cache,Fact	*f_p_f_imdl,MDLController	*controller){

			cache->CS.enter();
			uint64	now=Now();
			r_code::list<E>::const_iterator	_e;
			for(_e=cache->evidences.begin();_e!=cache->evidences.end();){

				if((*_e).is_too_old(now))	// garbage collection.
					_e=cache->evidences.erase(_e);
				else{
					
					P<MDLOverlay>	o=new	PrimaryMDLOverlay(this,bindings);
					o->reduce((*_e).evidence,f_p_f_imdl,controller);
					++_e;
				}
			}
			cache->CS.leave();
		}

		bool	monitor_predictions(_Fact	*input);

		MDLController(r_code::View	*view);
	public:
		static	MDLController	*New(View	*view,bool	&inject_in_secondary_group);

		void	add_monitor(PMonitor	*m);
		void	remove_monitor(PMonitor	*m);

		_Fact	*get_lhs()	const;
		_Fact	*get_rhs()	const;
		Fact	*get_f_ihlp(HLPBindingMap	*bindings,bool	wr_enabled)	const;

		virtual	void	store_requirement(_Fact	*f_p_f_imdl,MDLController	*controller,bool	chaining_was_allowed,bool	simulation)=0;
		ChainingStatus	retrieve_imdl_fwd(HLPBindingMap	*bm,Fact	*f_imdl,RequirementsPair	&r_p,Fact	*&ground,MDLController	*req_controller,bool	&wr_enabled);	// checks the requirement instances during fwd; r_p: all wrs in first, all srs in second.
		ChainingStatus	retrieve_imdl_bwd(HLPBindingMap	*bm,Fact	*f_imdl,Fact	*&ground);	// checks the requirement instances during bwd; ground is set to the best weak requirement if chaining allowed, NULL otherwise.
		ChainingStatus	retrieve_simulated_imdl_fwd(HLPBindingMap	*bm,Fact	*f_imdl,Controller	*root);
		ChainingStatus	retrieve_simulated_imdl_bwd(HLPBindingMap	*bm,Fact	*f_imdl,Controller	*root);

		virtual	void	predict(HLPBindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground)=0;
		virtual	void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures)=0;
		virtual	void	register_req_outcome(Fact	*f_pred,bool	success,bool	rate_failures){}

		void	add_requirement_to_rhs();
		void	remove_requirement_from_rhs();

		RType	is_requirement()	const{	return	_is_requirement;	}
		bool	is_reuse()			const{	return	_is_reuse;	}
		bool	is_cmd()			const{	return	_is_cmd;	}

		void	register_requirement(_Fact	*f_pred,RequirementsPair	&r_p);
	};

	class	PMDLController:
	public	MDLController{
	protected:
		CriticalSection				g_monitorsCS;
		r_code::list<P<_GMonitor> >	g_monitors;
		r_code::list<P<_GMonitor> >	r_monitors;

		virtual	uint32	get_rdx_out_group_count()	const{	return	get_out_group_count();	}
		void	inject_goal(HLPBindingMap	*bm,Fact	*goal,Fact	*f_imdl)	const;
		void	inject_simulation(Fact	*simulation)	const;

		bool	monitor_goals(_Fact	*input);

		uint64	get_sim_thz(uint64	now,uint64 deadline)	const;

		PMDLController(r_code::View	*view);
	public:
		void	add_g_monitor(_GMonitor	*m);
		void	remove_g_monitor(_GMonitor	*m);
		void	add_r_monitor(_GMonitor	*m);
		void	remove_r_monitor(_GMonitor	*m);

		virtual	void	register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const=0;
				void	register_predicted_goal_outcome(Fact	*goal,HLPBindingMap	*bm,Fact	*f_imdl,bool	success,bool	injected_goal);
		virtual	void	register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const=0;
	};

	// See g_monitor.h: controllers and monitors work closely together.
	//
	// Min sthz is the time allowed for simulated predictions to flow upward.
	// Max sthz sets the responsiveness of the model, i.e. limits the time waiting for simulation results, i.e. limits the latency of decision making.
	// Simulation is synchronous, i.e. is performed within the enveloppe of sthz, recursively.

	// Drives are not monitored (since they are not produced by models): they are injected periodically by user-defined pgms.
	// Drives are not observable: they cannot be predicted to succeed or fail.
	// Rhs is a drive; the model is an axiom: no rating and lives in the primary group only.
	// The model does not predict.
	// There is exactly one top-level model for each drive: hence no simulation during backward chaining.
	// Top-level models cannot have requirements.
	//
	// Backward chaining: inputs are drives.
	//	if lhs is in the fact cache, stop.
	//	if lhs is in the prediction cache, spawn a g-monitor (will be ready to catch a counter-prediction, invalidate the goal and trigger the re-issuing of a new goal).
	//	else commit to the sub-goal; this will trigger the simulation of sub-sub-goals; N.B.: commands are not simulated, commands with unbound values are not injected.
	class	TopLevelMDLController:
	public	PMDLController{
	private:
		uint32	get_rdx_out_group_count()	const{	return	get_out_group_count()-1;	}	// so that rdx are not injected in the drives host.

		void	abduce(HLPBindingMap	*bm,Fact	*super_goal,float32	confidence);
		void	abduce_lhs(HLPBindingMap	*bm,Fact	*super_goal,_Fact	*sub_goal_target,Fact	*f_imdl,_Fact	*evidence);

		void	register_drive_outcome(Fact	*goal,bool	success)	const;

		void	check_last_match_time(bool	match){}
	public:
		TopLevelMDLController(r_code::View	*view);

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);

		void	store_requirement(_Fact	*f_imdl,MDLController	*controller,bool	chaining_was_allowed,bool	simulation);	// never called.

		void	predict(HLPBindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground);
		void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures);
		void	register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;
		void	register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;
	};

	class	SecondaryMDLController;

	// Backward chaining: inputs are goals, actual or simulated.
	// Actual goals:
	//	if lhs is in the fact cache, stop.
	//	if lhs is in the prediction cache, spawn a g-monitor (will be ready to catch a counter-prediction, invalidate the goal and re-issue a new goal).
	//	else
	//		if (before-now)*percentage<min sthz, commit sub-goal on lhs.
	//		else
	//			if chaining is allowed, simulate the lhs and spawn a g-monitor with sthz=min((before-now)*percentage,max sthz)-min sthz.
	//			else, simulate f->imdl and spawn a g-monitor with sthz=min((before-now)*percentage,max sthz)/2-min sthz.
	// Simulated goals:
	//	if lhs is in the fact cache, .
	//	if lhs is in the prediction cache,
	//	else:
	//		if sthz/2>min thz, simulate the lhs and spawn a g-monitor with sthz/2-min sthz.
	//		else predict rhs (cfd=1) and stop.
	//	Commands with unbound values are not injected.
	class	PrimaryMDLController:
	public	PMDLController{
	private:
		SecondaryMDLController	*secondary;

		CriticalSection		codeCS;
		CriticalSection		last_match_timeCS;

		CriticalSection		assumptionsCS;
		r_code::list<P<Code> >	assumptions;	// produced by the model; garbage collection at reduce(9 time..

		void	rate_model(bool	success);
		void	kill_views();	// force res in both primary/secondary to 0.
		void	check_last_match_time(bool	match);	// activate secondary controller if no match after primary_thz;

		void	abduce_lhs(HLPBindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim,Fact	*ground,bool	set_before);
		void	abduce_imdl(HLPBindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim);
		void	abduce_simulated_lhs(HLPBindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim);
		void	abduce_simulated_imdl(HLPBindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim);
		void	predict_simulated_lhs(HLPBindingMap	*bm,bool	opposite,float32	confidence,Sim	*sim);
		void	predict_simulated_evidence(_Fact	*evidence,Sim	*sim);
		void	assume(_Fact	*input);
		void	assume_lhs(HLPBindingMap	*bm,bool	opposite,_Fact	*input,float32	confidence);
	public:
		PrimaryMDLController(r_code::View	*view);

		void	set_secondary(SecondaryMDLController	*secondary);

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);
		void	reduce_batch(Fact	*f_p_f_imdl,MDLController	*controller);

		void	store_requirement(_Fact	*f_imdl,MDLController	*controller,bool	chaining_was_allowed,bool	simulation);

		void	predict(HLPBindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground);
		bool	inject_prediction(Fact	*prediction,Fact	*f_imdl,float32	confidence,uint64	time_to_live,Code	*mk_rdx)	const;	// here, resilience=time to live, in us; returns true if the prediction has actually been injected.

		void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures);
		void	register_req_outcome(_Fact	*f_imdl,bool	success,bool	rate_failures);

		void	register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;
		void	register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;

		bool	check_imdl(Fact	*goal,HLPBindingMap	*bm);
		bool	check_simulated_imdl(Fact	*goal,HLPBindingMap	*bm,Controller	*root);
		void	abduce(HLPBindingMap	*bm,Fact	*super_goal,bool	opposite,float32	confidence);
	};

	// No backward chaining.
	// Rating happens only upon the success of predictions.
	// Requirements are stroed whetwehr they come from a primary or a secondary controller.
	// Positive requirements are stored into the rhs controller, both kinds (secondary or primary: the latter case is necessary for rating the model).
	class	SecondaryMDLController:
	public	MDLController{
	private:
		PrimaryMDLController	*primary;

		CriticalSection		codeCS;
		CriticalSection		last_match_timeCS;

		void	rate_model();	// record successes only.
		void	kill_views();	// force res in both primary/secondary to 0.
		void	check_last_match_time(bool	match);	// kill if no match after secondary_thz;
	public:
		SecondaryMDLController(r_code::View	*view);

		void	set_primary(PrimaryMDLController	*primary);

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);
		void	reduce_batch(Fact	*f_p_f_imdl,MDLController	*controller);

		void	store_requirement(_Fact	*f_imdl,MDLController	*controller,bool	chaining_was_allowed,bool	simulation);

		void	predict(HLPBindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground);
		void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures);
		void	register_req_outcome(_Fact	*f_pred,bool	success,bool	rate_failures);
	};
}


#endif