//	cst_controller.h
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

#ifndef	cst_controller_h
#define	cst_controller_h

#include	"hlp_overlay.h"
#include	"hlp_controller.h"
#include	"factory.h"


namespace	r_exec{

	// All inputs are expected to be synchronized (within the time tolerance).
	// The time value of each fact is therefore the same (within the time tolerance).
	// Therefore, said time value is not listed in the argument list of the icst.
	// It is held by the fact holding said icst.
	// Inputs synchronized on state are treated as if they were produced continuously, i.e. Now().
	// The confidence value for an icst is the lowest value taken from the matched inputs.
	// No cmds or imdls in a cst.
	//
	// Forward chaining:
	//	output a prediction of icst instead of icst if at least one input is a prediction.
	//	output as many predictions of icst as we got simulated predictions for different goals.
	class	CSTOverlay:
	public	HLPOverlay{
	protected:
		uint64	match_deadline;	// before deadline after the last match.
		float32	lowest_cfd;		// among the inputs (forward chaining).

		std::vector<P<_Fact> >	inputs;

		UNORDERED_SET<P<_Fact>,PHash<_Fact> >	predictions;	// f0->pred->f1->obj.
		UNORDERED_SET<P<Sim>,PHash<Sim> >		simulations;

		void	inject_production();
		void	update(BindingMap	*map,_Fact	*input,_Fact	*bound_pattern);
		CSTOverlay	*get_offspring(BindingMap	*map,_Fact	*input,_Fact	*bound_pattern);

		CSTOverlay(const	CSTOverlay	*original);
	public:
		CSTOverlay(Controller	*c,BindingMap	*bindings);
		~CSTOverlay();

		bool	reduce(View	*input,CSTOverlay	*&offspring);

		void	load_patterns();

		bool	can_match(uint64	now)	const;
	};

	// Backward chaining:
	//	if there are requirements, do nothing: these requirements will get the goal and abduce.
	//	else
	//		bind all patterns and look in the cache for positive evidences; for all bound patterns not matched in the cache, output a sub-goal (simulated or not, depending on the super-goal).
	class	CSTController:
	public	HLPController{
	private:
		Group	*secondary_host;

		void	abduce(BindingMap	*bm,Fact	*super_goal);	// super_goal is f->g->f->icst.
		void	inject_goal(BindingMap	*bm,
							Fact		*super_goal,		// f0->g->f1->icst.
							_Fact		*sub_goal_target,	// f1.
							Sim			*sim,
							uint64		now,
							float32		confidence,
							Code		*group)	const;

		void	kill_views();
		void	check_last_match_time(bool	match);	// kill if no match after primary_thz;
	public:
		CSTController(r_code::View	*view);
		~CSTController();

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);

		Fact	*get_f_ihlp(BindingMap	*bindings,bool	wr_enabled)	const;
		Fact	*get_f_icst(BindingMap	*bindings,std::vector<P<_Fact> >	*inputs)	const;

		void	inject_icst(Fact	*production,float32	confidence,uint64	time_to_live)	const;	// here, resilience=time to live, in us.
		bool	inject_prediction(Fact	*prediction,float32	confidence,uint64	time_to_live)	const;	// here, resilience=time to live, in us; returns true if the prediction has actually been injected.

		void	set_secondary_host(Group	*host);
		Group	*get_secondary_host()	const;
	};
}


#endif