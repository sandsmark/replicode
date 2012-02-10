//	g_monitor.cpp
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

#include	"g_monitor.h"
#include	"mem.h"
#include	"mdl_controller.h"
#include	"factory.h"


namespace	r_exec{

	_GMonitor::_GMonitor(	PMDLController	*controller,
							BindingMap		*bindings,
							uint64			deadline,
							uint64			sim_thz,
							Fact			*goal,
							Fact			*f_imdl):Monitor(	controller,
																bindings,
																goal),
																deadline(deadline),
																sim_thz(sim_thz),
																f_imdl(f_imdl){	// goal is f0->g->f1->object.

		simulating=(sim_thz>0);
		sim_mode=goal->get_goal()->sim->mode;
		goal_target=target->get_goal()->get_target();	// f1.
	}

	void	_GMonitor::store_simulated_outcome(Goal	*affected_goal,Sim	*sim,bool	success){	// outcome is f0 as in f0->pred->f1->success.

		if(success){

			switch(sim->mode){
			case	SIM_MANDATORY:
				sim_successes.mandatory_solutions.push_back(std::pair<P<Goal>,P<Sim> >(affected_goal,sim));
				break;
			case	SIM_OPTIONAL:
				sim_successes.mandatory_solutions.push_back(std::pair<P<Goal>,P<Sim> >(affected_goal,sim));
				break;
			default:
				break;
			}
		}else{

			switch(sim->mode){
			case	SIM_MANDATORY:
				sim_failures.optional_solutions.push_back(std::pair<P<Goal>,P<Sim> >(affected_goal,sim));
				break;
			case	SIM_OPTIONAL:
				sim_failures.optional_solutions.push_back(std::pair<P<Goal>,P<Sim> >(affected_goal,sim));
				break;
			default:
				break;
			}
		}
	}

	void	_GMonitor::invalidate_sim_outcomes(){

		SolutionList::const_iterator	sol;

		for(sol=sim_failures.mandatory_solutions.begin();sol!=sim_failures.mandatory_solutions.end();++sol)
			(*sol).second->invalidate();

		for(sol=sim_failures.optional_solutions.begin();sol!=sim_failures.optional_solutions.end();++sol)
			(*sol).second->invalidate();

		for(sol=sim_successes.mandatory_solutions.begin();sol!=sim_successes.mandatory_solutions.end();++sol)
			(*sol).second->invalidate();

		for(sol=sim_successes.optional_solutions.begin();sol!=sim_successes.optional_solutions.end();++sol)
			(*sol).second->invalidate();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GMonitor::GMonitor(	PMDLController	*controller,
						BindingMap		*bindings,
						uint64			deadline,
						uint64			sim_thz,
						Fact			*goal,
						Fact			*f_imdl,
						_Fact			*predicted_evidence):_GMonitor(	controller,
																		bindings,
																		deadline,
																		sim_thz,
																		goal,
																		f_imdl),
																		predicted_evidence(predicted_evidence){	// goal is f0->g->f1->object.

		injected_goal=(predicted_evidence==NULL);
		MonitoringJob<GMonitor>	*j=new	MonitoringJob<GMonitor>(this,sim_thz);
		_Mem::Get()->pushTimeJob(j);
	}

	void	GMonitor::commit(){	// the purpose is to invalidate damaging simulations; if anything remains, commit to all mandatory simulations and to the best optional one.

		Goal	*monitored_goal=target->get_goal();

		uint64	now=Now();

		SolutionList::const_iterator	sol;

		for(sol=sim_failures.mandatory_solutions.begin();sol!=sim_failures.mandatory_solutions.end();++sol){	// check if any mandatory solution could result in the failure of more important a goal.

			if((*sol).second->is_invalidated())
				continue;
			if((*sol).first->get_strength(now)>monitored_goal->get_strength(now)){	// cave in.

				invalidate_sim_outcomes();	// this stops any further propagation of the goal simulation.
				return;
			}
		}

		for(sol=sim_failures.optional_solutions.begin();sol!=sim_failures.optional_solutions.end();++sol){		// check if any optional solutions could result in the failure of more important a goal; invalidate the culprits.

			if((*sol).second->is_invalidated())
				continue;
			if((*sol).first->get_strength(now)>monitored_goal->get_strength(now))
				(*sol).second->invalidate();
		}

		Sim	*best_sol=NULL;
		for(sol=sim_successes.optional_solutions.begin();sol!=sim_successes.optional_solutions.end();++sol){	// find the best optional solution left.

			if((*sol).second->is_invalidated())
				continue;
			if(!best_sol)
				best_sol=(*sol).second;
			else{
				
				float32	s=(*sol).second->sol_cfd/((*sol).second->sol_before-now);
				float32	_s=best_sol->sol_cfd/(best_sol->sol_before-now);
				if(s>_s)
					best_sol=(*sol).second;
			}
		}

		invalidate_sim_outcomes();	// this stops any further propagation of the goal simulation.

		if(best_sol){

			((PrimaryMDLController	*)(*sol).second->sol)->abduce(bindings,best_sol->super_goal,best_sol->opposite,goal_target->get_cfd());

			for(sol=sim_successes.mandatory_solutions.begin();sol!=sim_successes.mandatory_solutions.end();++sol)	// commit to all mandatory solutions.
				((PrimaryMDLController	*)(*sol).second->sol)->abduce(bindings,(*sol).second->super_goal,(*sol).second->opposite,goal_target->get_cfd());
		}
	}

	bool	GMonitor::reduce(_Fact	*input){	// executed by a reduction core; invalidation check performed in Monitor::is_alive().

		if(!injected_goal){
			
			if(predicted_evidence->is_invalidated()){	// the predicted evidence was wrong.

				((PMDLController	*)controller)->register_predicted_goal_outcome(target,bindings,f_imdl,false,injected_goal);	// report a predicted failure; this will inject the goal.
				predicted_evidence=NULL;
				injected_goal=true;
				return	false;
			}
		}

		Pred	*prediction=input->get_pred();
		if(prediction){	// input is f0->pred->f1->object.

			_Fact	*_input=prediction->get_target();	// _input is f1->obj.
			if(simulating){	// injected_goal==true.

				Sim		*sim=prediction->get_simulation(target);
				if(sim){

					Code	*outcome=_input->get_reference(0);
					if(outcome->code(0).asOpcode()==Opcodes::Success){	// _input is f1->success or |f1->success.

						_Fact	*f_success=(_Fact	*)outcome->get_reference(SUCCESS_OBJ);
						Goal	*affected_goal=f_success->get_goal();
						if(affected_goal){

							store_simulated_outcome(affected_goal,sim,_input->is_fact());
							return	false;
						}
					}else{	// report the simulated outcome: this will inject a simulated prediction of the outcome, to allow any g-monitor deciding on this ground.

						switch(_input->is_evidence(goal_target)){
						case	MATCH_SUCCESS_POSITIVE:
							((PMDLController	*)controller)->register_simulated_goal_outcome(target,true,input);	// report a simulated success.
							return	false;
						case	MATCH_SUCCESS_NEGATIVE:
							((PMDLController	*)controller)->register_simulated_goal_outcome(target,false,input);	// report a simulated failure.
							return	false;
						case	MATCH_FAILURE:
							return	false;
						}
					}
				}else	// during simulation (SIM_ROOT) if the prediction is actual, positive and comes true, we'll eventually catch an actual evidence; otherwise (positive that does not come true or negative), keep simulating: in any case ignore it.
					return	false;
			}else{

				switch(_input->is_evidence(goal_target)){
				case	MATCH_SUCCESS_POSITIVE:
					if(injected_goal)
						((PMDLController	*)controller)->register_predicted_goal_outcome(target,bindings,f_imdl,true,true);	// report a predicted success.
					if(_input->get_cfd()>predicted_evidence->get_pred()->get_target()->get_cfd())	// bias toward cfd instead of age.
						predicted_evidence=input;
					return	false;
				case	MATCH_SUCCESS_NEGATIVE:
					((PMDLController	*)controller)->register_predicted_goal_outcome(target,bindings,f_imdl,false,injected_goal);	// report a predicted failure; this may invalidate the target.
					predicted_evidence=NULL;
					injected_goal=true;
					return	target->is_invalidated();
				case	MATCH_FAILURE:
					return	false;
				}
			}
		}else{	// input is an actual fact.

			Goal	*g=target->get_goal();
			if(g->ground_invalidated(input)){	// invalidate the goal and abduce from the super-goal.

				target->invalidate();
				((PrimaryMDLController	*)controller)->abduce(bindings,g->sim->super_goal,g->sim->opposite,goal_target->get_cfd());
				return	true;
			}

			switch(input->is_evidence(goal_target)){
			case	MATCH_SUCCESS_POSITIVE:
				((PMDLController	*)controller)->register_goal_outcome(target,true,input);	// report a success.
				return	true;
			case	MATCH_SUCCESS_NEGATIVE:
				((PMDLController	*)controller)->register_goal_outcome(target,false,input);	// report a failure.
				return	true;
			case	MATCH_FAILURE:
				return	false;
			}
		}
	}

	void	GMonitor::update(uint64	&next_target){	// executed by a time core.
		
		if(target->is_invalidated()){

			((PMDLController	*)controller)->remove_monitor(this);
			next_target=0;
		}else	if(simulating){

			simulating=0;
			commit();
			next_target=deadline;
		}else{

			((PMDLController	*)controller)->register_goal_outcome(target,false,NULL);
			((PMDLController	*)controller)->remove_monitor(this);
			next_target=0;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_RMonitor::_RMonitor(	PrimaryMDLController	*controller,
							BindingMap				*bindings,
							uint64					deadline,
							uint64					sim_thz,
							Fact					*goal,
							Fact					*f_imdl):Monitor(	controller,
																		bindings,
																		goal),
																		deadline(deadline),
																		sim_thz(sim_thz),
																		f_imdl(f_imdl){	// goal is f0->g->f1->imdl.
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RMonitor::RMonitor(	PrimaryMDLController	*controller,
						BindingMap				*bindings,
						uint64					deadline,
						Fact					*goal,
						Fact					*f_imdl):_RMonitor(	controller,
																	bindings,
																	deadline,
																	sim_thz,
																	goal,
																	f_imdl){	// goal is f0->g->f1->object.

		MonitoringJob<RMonitor>	*j=new	MonitoringJob<RMonitor>(this,deadline);
		_Mem::Get()->pushTimeJob(j);
	}

	bool	RMonitor::signal(){

		if(((PrimaryMDLController	*)controller)->check_imdl(target,bindings))
			return	true;
		return	false;
	}

	bool	RMonitor::reduce(_Fact	*input){

		return	false;
	}

	void	RMonitor::update(uint64	&next_target){

		if(!target->is_invalidated())
			((PrimaryMDLController	*)controller)->register_goal_outcome(target,false,NULL);
		((PrimaryMDLController	*)controller)->remove_monitor(this);
		next_target=0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SGMonitor::SGMonitor(	PrimaryMDLController	*controller,
							BindingMap				*bindings,
							uint64					sim_thz,
							Fact					*goal,
							Fact					*f_imdl):_GMonitor(controller,
																		bindings,
																		0,
																		sim_thz,
																		goal,
																		f_imdl){	// goal is f0->g->f1->object.

		MonitoringJob<SGMonitor>	*j=new	MonitoringJob<SGMonitor>(this,sim_thz);
		_Mem::Get()->pushTimeJob(j);
	}

	void	SGMonitor::commit(){	// the purpose is to invalidate damaging simulations and let the rest flow upward.

		Goal	*monitored_goal=target->get_goal();

		uint64	now=Now();

		SolutionList::const_iterator	sol;

		for(sol=sim_failures.mandatory_solutions.begin();sol!=sim_failures.mandatory_solutions.end();++sol){	// check if any mandatory solution could result in the failure of more important a goal.

			if((*sol).second->is_invalidated())
				continue;
			if((*sol).first->get_strength(now)>monitored_goal->get_strength(now)){	// cave in.

				(*sol).second->invalidate();
				return;
			}
		}

		for(sol=sim_failures.optional_solutions.begin();sol!=sim_failures.optional_solutions.end();++sol){		// check if any optional solutions could result in the failure of more important a goal; invalidate the culprits.

			if((*sol).second->is_invalidated())
				continue;
			if((*sol).first->get_strength(now)>monitored_goal->get_strength(now))
				(*sol).second->invalidate();
		}
	}

	bool	SGMonitor::reduce(_Fact	*input){

		_Fact	*_input;
		Pred	*prediction=input->get_pred();
		if(prediction){	// input is f0->pred->f1->object.

			_input=prediction->get_target();	// _input is f1->obj.

			Sim		*sim=prediction->get_simulation(target);
			if(sim){

				Code	*outcome=_input->get_reference(0);
				if(outcome->code(0).asOpcode()==Opcodes::Success){	// _input is f1->success or |f1->success.

					_Fact	*f_success=(_Fact	*)outcome->get_reference(SUCCESS_OBJ);
					Goal	*affected_goal=f_success->get_goal();
					if(affected_goal){

						store_simulated_outcome(affected_goal,sim,_input->is_fact());
						return	false;
					}
				}
			}
		}else
			_input=input;
		
		// Non-simulated input (can be actual or predicted): report the simulated outcome: this will inject a simulated prediction of the outcome, to allow any g-monitor deciding on this ground.
		switch(_input->is_evidence(goal_target)){
		case	MATCH_SUCCESS_POSITIVE:
			((PMDLController	*)controller)->register_simulated_goal_outcome(target,true,_input);	// report a simulated success.
			return	false;
		case	MATCH_SUCCESS_NEGATIVE:
			((PMDLController	*)controller)->register_simulated_goal_outcome(target,false,_input);	// report a simulated failure.
			return	false;
		case	MATCH_FAILURE:
			return	false;
		}
	}

	void	SGMonitor::update(uint64	&next_target){	// executed by a time core.
		
		if(!target->is_invalidated())
			commit();
		((PMDLController	*)controller)->remove_monitor(this);
		next_target=0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SRMonitor::SRMonitor(	PrimaryMDLController	*controller,
							BindingMap				*bindings,
							uint64					sim_thz,
							Fact					*goal,
							Fact					*f_imdl):_RMonitor(controller,
																		bindings,
																		0,
																		sim_thz,
																		goal,
																		f_imdl){	// goal is f0->g->f1->object.

		MonitoringJob<SRMonitor>	*j=new	MonitoringJob<SRMonitor>(this,sim_thz);
		_Mem::Get()->pushTimeJob(j);
	}

	bool	SRMonitor::signal(bool	simulation){

		if(simulation){

			if(((PrimaryMDLController	*)controller)->check_imdl(target,bindings,target->get_goal()->sim->root))
				return	true;
		}else
			if(((PrimaryMDLController	*)controller)->check_imdl(target,bindings,NULL))
				return	true;
		return	false;
	}

	bool	SRMonitor::reduce(_Fact	*input){

		return	false;
	}

	void	SRMonitor::update(uint64	&next_target){

		if(!target->is_invalidated())
			((PrimaryMDLController	*)controller)->register_simulated_goal_outcome(target,false,NULL);
		((PrimaryMDLController	*)controller)->remove_monitor(this);
		next_target=0;
	}
}