//	hlp_controller.cpp
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

#include	"hlp_controller.h"
#include	"mem.h"


namespace	r_exec{

	HLPController::HLPController(r_code::View	*view):OController(view),requirement_count(0){

		//	init a binding map from the patterns.
		bindings=new	BindingMap();

		uint16	obj_set_index=getObject()->code(HLP_OBJS).asIndex();
		uint16	obj_count=getObject()->code(obj_set_index).getAtomCount();
		for(uint16	i=1;i<=obj_count;++i){

			Code	*pattern=getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex());
			bindings->init(pattern);
		}
	}

	HLPController::~HLPController(){
	}

	void	HLPController::add_requirement(){

		reductionCS.enter();
		++requirement_count;
		reductionCS.leave();
	}

	void	HLPController::remove_requirement(){

		reductionCS.enter();
		--requirement_count;
		reductionCS.leave();
	}

	Code	*HLPController::get_instance(const	BindingMap	*bm,uint16	opcode)	const{

		Code	*instance=_Mem::Get()->build_object(Atom::Object(opcode,I_HLP_ARITY));
		instance->code(I_HLP_OBJ)=Atom::RPointer(0);
		instance->code(I_HLP_ARGS)=Atom::IPointer(I_HLP_ARITY+1);
		instance->code(I_HLP_ARITY)=Atom::Float(1);	//	psln_thr.
		bm->copy(instance,I_HLP_ARITY+1);
		instance->set_reference(0,getObject());

		Code	*_instance=_Mem::Get()->check_existence(instance);
		if(_instance!=instance){

			delete	instance;
			instance=_instance;
		}

		return	instance;
	}

	Code	*HLPController::get_instance(uint16	opcode)	const{

		return	get_instance(bindings,opcode);
	}

	bool	HLPController::monitor(Code	*input){

		if(!input->get_goal()){	//	discard facts marked as goals; this does not mean that goals are discarded when they are the input.

			bool	match=false;
			std::list<P<GMonitor> >::const_iterator	m;
			g_monitorsCS.enter();
			for(m=g_monitors.begin();m!=g_monitors.end();){

				if((*m)->reduce(input)){

					match=true;
					m=g_monitors.erase(m);
				}else
					++m;
			}
			g_monitorsCS.leave();
			return	match;
		}
		return	false;
	}

	void	HLPController::add_outcome(Code	*target,bool	success,float32	confidence)	const{	//	success==false: executed in the thread of a time core; otherwise, executed in the same thread as for Controller::reduce().

		Code	*mk_success=factory::Object::MkSuccess(target,1);

		uint64	now=Now();

		Code	*fact;
		Code	*opposite=NULL;
		if(success)
			fact=factory::Object::Fact(mk_success,now,confidence,1);
		else{	//	there is no mk.failure: failure=|fact on success.

			fact=factory::Object::AntiFact(mk_success,now,confidence,1);
			opposite=factory::Object::Fact(target->get_reference(0),now,confidence,1);
		}

		for(uint16	i=1;i<=get_out_group_count();++i){	//	inject notification in out groups.

			Group	*origin=getView()->get_host();
			Group	*out_group=(Group	*)get_out_group(i);
			View	*view=new	NotificationView(origin,out_group,mk_success);
			_Mem::Get()->inject_notification(view,true);

			view=new	View(true,now,1,_Mem::Get()->get_ntf_mk_res(),out_group,origin,fact);
			_Mem::Get()->inject(view);

			if(opposite){

				view=new	View(true,now,1,1,out_group,getView()->get_host(),opposite);
				_Mem::Get()->inject(view);
			}
		}

		target->kill();
	}

	void	HLPController::remove_monitor(GMonitor	*m){

		g_monitorsCS.enter();
		g_monitors.remove(m);
		g_monitorsCS.leave();
	}

	uint16	HLPController::get_out_group_count()	const{

		return	getObject()->code(getObject()->code(HLP_OUT_GRPS).asIndex()).getAtomCount();
	}

	Code	*HLPController::get_out_group(uint16	i)	const{

		return	getObject()->get_reference(getObject()->code(getObject()->code(HLP_OUT_GRPS).asIndex()+i).asIndex());
	}

	Code	*HLPController::get_sub_goal(	BindingMap	*bm,
											Code		*super_goal,
											Code		*sub_goal_target,
											Code		*instance,
											uint64		&now,
											uint64		&deadline_high,
											uint64		&deadline_low,
											Code		*&matched_pattern){

		now=Now();
		Code	*super_goal_fact=super_goal->get_reference(0)->get_reference(0);
		if(super_goal_fact->code(super_goal_fact->code(FACT_TIME).asIndex()+1).getDescriptor()==Atom::STRUCTURAL_VARIABLE){

			deadline_high=now+_Mem::Get()->get_goal_res();
			deadline_low=now;
		}else{

			uint64	deadline=Utils::GetTimestamp<Code>(super_goal_fact,FACT_TIME);
			uint32	time_tolerance_us=super_goal_fact->code(super_goal_fact->code(FACT_TIME).asIndex()).getTimeTolerance()<<10;
			deadline_high=deadline+time_tolerance_us;
			deadline_low=deadline-time_tolerance_us;
		}

		Code	*sub_goal;
		Code	*actor=super_goal->get_reference(0)->get_reference(1);
		matched_pattern=NULL;
		if(instance){	//	there exist a requirement.
						
			deadline_low=now;	//	try to get an instance asap.
			deadline_high+=_Mem::Get()->get_time_tolerance()<<10;
			uint64	median_deadline=(uint64)((float64)(deadline_high+deadline_low))/2;
			uint32	time_tolerance_ms=(deadline_high-median_deadline)>>10;

			Code	*ip_f=factory::Object::Fact(instance,median_deadline,1,1);
			ip_f->code(ip_f->code(FACT_TIME).asIndex()).setTimeTolerance(time_tolerance_ms);

			uint16	arg_set_index=instance->code(I_HLP_ARGS).asIndex();
			uint16	arg_count=instance->code(arg_set_index).getAtomCount();
			for(uint16	i=1;i<=arg_count;++i){

				if(instance->code(arg_set_index+i).getDescriptor()==Atom::I_PTR){

					uint16	index=instance->code(arg_set_index+i).asIndex();
					if(instance->code(index).getDescriptor()==Atom::TIMESTAMP)
						instance->code(index).setTimeTolerance(time_tolerance_ms);
				}
			}

			sub_goal=factory::Object::MkGoal(ip_f,actor,1);
			matched_pattern=sub_goal_target;
		}else
			sub_goal=factory::Object::MkGoal(sub_goal_target,actor,1);

		GoalRecords::const_iterator	r;
		for(r=goal_records.begin();r!=goal_records.end();){

			if(instance){
				uint32	u=0;
				}
			uint64	goal_time=Utils::GetTimestamp<Code>(r->first,FACT_TIME);
			if(now-goal_time>_Mem::Get()->get_goal_record_resilience()){

				r=goal_records.erase(r);
				continue;
			}

			BindingMap	bm(r->second);
			Code	*new_target=sub_goal->get_reference(0)->get_reference(0);
			Code	*recorded_target=r->first->get_reference(0)->get_reference(0)->get_reference(0);
			if(new_target->code(0).asOpcode()==Opcodes::MkVal	&&	recorded_target->code(0).asOpcode()==Opcodes::MkVal){
uint32	u=0;
				}
			if(bm.match(recorded_target,new_target))	//	pattern=new_target: we try to catch new goals that are less specified than the recorded goals.				
				return	NULL;
			++r;
		}

		Code	*sub_goal_fact=factory::Object::Fact(sub_goal,now,1,1);
		goal_records.insert(GoalRecords::value_type(sub_goal_fact,bm));

		return	sub_goal_fact;

		/*Code	*_sub_goal=_Mem::Get()->check_existence(sub_goal);
		if(_sub_goal!=sub_goal){	//	a goal already exists for the same target (object or ip_f).
			
			notify_existing_sub_goal(Now(),
									deadline_high,
									_sub_goal,
									super_goal,
									get_instance(get_instance_opcode()));
			delete	sub_goal;
			return;
		}*/
	}

	void	HLPController::inject_sub_goal(	uint64	now,
											uint64	deadline,
											Code	*super_goal,		//	fact.
											Code	*sub_goal,			//	fact.
											Code	*ntf_instance){

		Code	*ntf_instance_fact=factory::Object::Fact(ntf_instance,now,1,1);
		Code	*mk_sim_goal=NULL;
		Code	*mk_asmp_goal=NULL;
		if(super_goal->get_asmp())
			mk_asmp_goal=factory::Object::MkAsmp(sub_goal,getObject(),1,1);
		if(super_goal->get_hyp()	||	super_goal->get_sim())
			mk_sim_goal=factory::Object::MkSim(sub_goal,getObject(),1);

		Code	*mk_rdx=factory::Object::MkRdx(ntf_instance_fact,super_goal,sub_goal,1);

		Group	*origin=getView()->get_host();
		uint16	out_group_count=get_out_group_count();
		for(uint16	i=1;i<=out_group_count;++i){

			Group	*out_group=(Group	*)get_out_group(i);
			uint64	base=_Mem::Get()->get_base_period()*out_group->get_upr();
			int64	delta=abs((float32)((int64)(deadline-now)));
			int32	resilience=Utils::GetResilience(delta,base);

			View	*view=new	View(true,now,1,resilience,out_group,origin,sub_goal->get_reference(0));	//	SYNC_FRONT,sln=1,res=resilience.
			_Mem::Get()->inject(view);

			view=new	View(true,now,1,resilience,out_group,origin,sub_goal);	//	SYNC_FRONT,sln=1,res=resilience.
			_Mem::Get()->inject(view);

			view=new	NotificationView(origin,out_group,mk_rdx);
			_Mem::Get()->inject_notification(view,true);

			if(mk_sim_goal){

				view=new	View(true,now,1,Utils::GetResilience(_Mem::Get()->get_sim_res(),base),out_group,origin,mk_sim_goal);
				_Mem::Get()->inject(view);
			}

			if(mk_asmp_goal){

				view=new	View(true,now,1,Utils::GetResilience(_Mem::Get()->get_asmp_res(),base),out_group,origin,mk_asmp_goal);
				_Mem::Get()->inject(view);
			}
		}
	}

	void	HLPController::notify_existing_sub_goal(uint64	now,
													uint64	deadline,
													Code	*super_goal,	//	fact.
													Code	*sub_goal,		//	mk.goal.
													Code	*ntf_instance){

		Code	*s_mk_sim=sub_goal->get_sim();
		Code	*s_mk_hyp=sub_goal->get_hyp();
		Code	*s_mk_asmp=sub_goal->get_asmp();

		bool	sim=super_goal->get_sim()	||	super_goal->get_hyp();
		bool	asmp=super_goal->get_asmp();
		if(!sim){

			if(s_mk_sim)
				s_mk_sim->kill();
			if(s_mk_hyp)
				s_mk_hyp->kill();
		}
		if(!asmp	&&	s_mk_asmp)
			s_mk_asmp->kill();

		Code	*ntf_instance_fact=factory::Object::Fact(ntf_instance,now,1,1);
		Code	*sub_goal_fact=factory::Object::Fact(sub_goal,now,1,1);

		Code	*mk_rdx=factory::Object::MkRdx(ntf_instance_fact,super_goal,sub_goal_fact,1);

		Group	*origin=getView()->get_host();
		uint16	out_group_count=get_out_group_count();
		for(uint16	i=1;i<=out_group_count;++i){

			Group	*out_group=(Group	*)get_out_group(i);
			uint64	base=_Mem::Get()->get_base_period()*out_group->get_upr();

			int32	resilience=Utils::GetResilience(deadline-now,base);

			View	*view=new	View(true,now,1,resilience,out_group,origin,sub_goal);	//	SYNC_FRONT,sln=1,res=resilience.
			_Mem::Get()->inject(view);

			view=new	View(true,now,1,resilience,out_group,origin,sub_goal_fact);	//	SYNC_FRONT,sln=1,res=resilience.
			_Mem::Get()->inject(view);

			view=new	NotificationView(origin,out_group,mk_rdx);
			_Mem::Get()->inject_notification(view,true);
		}
	}
}