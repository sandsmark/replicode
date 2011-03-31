//	mdl_controller.cpp
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

#include	"mdl_controller.h"
#include	"mem.h"


namespace	r_exec{

	MDLOverlay::MDLOverlay(Controller	*c,const	BindingMap	*bindings,uint8	reduction_mode):HLPOverlay(c,bindings,reduction_mode){
	}

	MDLOverlay::~MDLOverlay(){
	}

	void	MDLOverlay::inject_prediction(Code	*input)	const{

		uint64	now=Now();
		Code	*instance=((HLPController	*)controller)->get_instance(bindings,Opcodes::IMDL);
		Code	*instance_fact=factory::Object::Fact(instance,now,1,1);
		
		Code	*rhs=((MDLController	*)controller)->get_rhs();
		Code	*bound_rhs=bindings->bind_object(rhs);	//	fact or |fact.
		Code	*mk_pred=factory::Object::MkPred(bound_rhs,1,1);
		uint64	predicted_time=Utils::GetTimestamp<Code>(bound_rhs,FACT_TIME);
		float32	multiplier=rhs->code(rhs->code(FACT_TIME).asIndex()+1).getMultiplier();
		uint64	time_tolerance=abs((float32)((int64)(predicted_time-now)))*multiplier;
		uint32	pred_resilience=predicted_time+time_tolerance-now;	//	time to live in us.

		Code	*pred_fact=factory::Object::Fact(mk_pred,now,1,1);
		Code	*mk_sim_pred=get_mk_sim(pred_fact);
		Code	*mk_asmp_pred=get_mk_asmp(pred_fact);

		Code	*mk_rdx=factory::Object::MkRdx(instance_fact,input,pred_fact,1);
		Code	*mk_sim_instance=get_mk_sim(instance_fact);
		Code	*mk_asmp_instance=get_mk_asmp(instance_fact);
		
		Group	*origin=getView()->get_host();
		uint16	out_group_count=((HLPController	*)controller)->get_out_group_count();
		for(uint16	i=1;i<=out_group_count;++i){

			Group	*out_group=(Group	*)((HLPController	*)controller)->get_out_group(i);
			uint64	base=_Mem::Get()->get_base_period()*out_group->get_upr();
			uint32	res=Utils::GetResilience(pred_resilience,base);

			View	*view=new	View(true,now,1,1,out_group,origin,instance);	//	SYNC_FRONT,sln=1,res=1.
			_Mem::Get()->inject(view);
			
			view=new	View(true,now,1,1,out_group,origin,instance_fact);	//	SYNC_FRONT,sln=1,res=1.
			_Mem::Get()->inject(view);

			view=new	NotificationView(origin,out_group,mk_rdx);
			_Mem::Get()->inject_notification(view,true);

			if(mk_sim_instance){

				view=new	View(true,now,1,Utils::GetResilience(_Mem::Get()->get_sim_res(),base),out_group,origin,mk_sim_instance);
				_Mem::Get()->inject(view);
			}
			
			if(mk_asmp_instance){

				view=new	View(true,now,1,Utils::GetResilience(_Mem::Get()->get_asmp_res(),base),out_group,origin,mk_asmp_instance);
				_Mem::Get()->inject(view);
			}

			view=new	View(true,now,1,pred_resilience,out_group,origin,bound_rhs);	//	SYNC_FRONT,sln=1,res=pred_resilience.
			_Mem::Get()->inject(view);

			view=new	View(true,now,1,pred_resilience,out_group,origin,mk_pred);	//	SYNC_FRONT,sln=1,res=pred_resilience.
			_Mem::Get()->inject(view);
			
			view=new	View(true,now,1,pred_resilience,out_group,origin,pred_fact);	//	SYNC_FRONT,sln=1,res=pred_resilience.
			_Mem::Get()->inject(view);

			if(mk_sim_pred){

				view=new	View(true,now,1,Utils::GetResilience(_Mem::Get()->get_sim_res(),base),out_group,origin,mk_sim_pred);
				_Mem::Get()->inject(view);
			}
			
			if(mk_asmp_pred){

				view=new	View(true,now,1,Utils::GetResilience(_Mem::Get()->get_asmp_res(),base),out_group,origin,mk_asmp_pred);
				_Mem::Get()->inject(view);
			}
		}
		
		PMonitor	*m=new	PMonitor(	(MDLController	*)controller,
										bindings,
										mk_pred,
										predicted_time+time_tolerance,
										predicted_time-time_tolerance);
		((MDLController	*)controller)->add_monitor(m);
	}

	Overlay	*MDLOverlay::reduce(r_exec::View *input){

		BindingMap	*bm=new	BindingMap(bindings);
		if(bm->match(input->object,((MDLController	*)controller)->get_lhs())){

			BindingMap	*tmp=bindings;
			bindings=bm;
			if(input->object->get_sim()	||	input->object->get_hyp())
				reduction_mode|=RDX_MODE_SIMULATION;
			if(input->object->get_asmp())
				reduction_mode|=RDX_MODE_ASSUMPTION;

			inject_prediction(input->object);
			
			//	reset.
			bindings=tmp;
			reduction_mode=RDX_MODE_REGULAR;
			return	this;
		}else{

			delete	bm;
			return	NULL;
		}
	}

	void	MDLOverlay::load_patterns(){

		patterns.push_back(((MDLController	*)controller)->get_lhs());
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MDLController::MDLController(r_code::View	*view):HLPController(view){

		MDLOverlay	*o=new	MDLOverlay(this,bindings,RDX_MODE_REGULAR);	//	master overlay.
		o->load_patterns();
		overlays.push_back(o);
	}

	MDLController::~MDLController(){
	}

	void	MDLController::take_input(r_exec::View	*input){

		HLPController::_take_input<MDLController>(input);
	}

	void	MDLController::reduce(r_exec::View	*input){

		bool	match=false;
		std::list<P<Overlay> >::const_iterator	o=overlays.begin();
		reductionCS.enter();
		for(o=overlays.begin();o!=overlays.end();++o){

			if((*o)->reduce(input))
				match=true;
		}

		if(!match){
			
			Code	*mk_goal=input->object->get_reference(0);	//	input->object is  fact or |fact.
			if(mk_goal->code(0).asOpcode()==Opcodes::MkGoal){

				BindingMap	*bm=new	BindingMap(bindings);
				Code		*goal_target=mk_goal->get_reference(0);
				Code		*rhs_target=get_rhs()->get_reference(0);
				if(bm->match(goal_target->get_reference(0),rhs_target->get_reference(0))){	//	first, check the objects pointed to by the facts.

					if(bm->match(goal_target,get_rhs())){	//	the fact pointed by a goal matches the rhs.

						Code	*bound_lhs=bm->bind_object(get_lhs());	//	fact or |fact: no need to check for existence (timings always different).
						if(goal_target->code(0).asOpcode()==Opcodes::Fact){

							if(get_rhs()->code(0).asOpcode()==Opcodes::AntiFact){

								if(bound_lhs->code(0).asOpcode()==Opcodes::Fact)	//	case A -> |B
									bound_lhs->code(0)=Atom::Object(Opcodes::AntiFact,FACT_ARITY);
								else												//	case |A -> |B
									bound_lhs->code(0)=Atom::Object(Opcodes::Fact,FACT_ARITY);
							}
						}else{	//	goal target is |fact.

							if(get_rhs()->code(0).asOpcode()==Opcodes::Fact){

								if(bound_lhs->code(0).asOpcode()==Opcodes::Fact)	//	case A -> B
									bound_lhs->code(0)=Atom::Object(Opcodes::AntiFact,FACT_ARITY);
								else												//	case |A -> B
									bound_lhs->code(0)=Atom::Object(Opcodes::Fact,FACT_ARITY);
							}
						}
			
						if(requirement_count)
							produce_sub_goal(bm,input->object,bound_lhs,get_instance(bm,Opcodes::IMDL),true);
						else
							produce_sub_goal(bm,input->object,bound_lhs,NULL,true);
					}else
						delete	bm;
				}
			}
		}
		reductionCS.leave();

		monitor(input->object);
	}

	void	MDLController::monitor(Code	*input){

		if(	!input->get_sim()	&&
			!input->get_asmp()	&&
			!input->get_pred()){	//	we are only interested in actual facts: discard pred, asmp and hyp/sim.

			std::list<P<PMonitor> >::const_iterator	m;
			p_monitorsCS.enter();
			for(m=p_monitors.begin();m!=p_monitors.end();){

				if((*m)->reduce(input))
					m=p_monitors.erase(m);
				else
					++m;
			}
			p_monitorsCS.leave();
		}

		HLPController::monitor(input);
	}

	void	MDLController::add_monitor(PMonitor	*m){

		p_monitorsCS.enter();
		p_monitors.push_front(m);
		p_monitorsCS.leave();
	}

	void	MDLController::remove_monitor(PMonitor	*m){

		p_monitorsCS.enter();
		p_monitors.remove(m);
		p_monitorsCS.leave();
	}

	Code	*MDLController::get_lhs()	const{

		uint16	obj_set_index=getObject()->code(MDL_OBJS).asIndex();
		return	getObject()->get_reference(getObject()->code(obj_set_index+1).asIndex());
	}

	Code	*MDLController::get_rhs()	const{

		uint16	obj_set_index=getObject()->code(MDL_OBJS).asIndex();
		return	getObject()->get_reference(getObject()->code(obj_set_index+2).asIndex());
	}

	HLPController	*MDLController::get_rhs_controller()	const{

		Code	*rhs=get_rhs();	//	fact or |fact.
		Code	*rhs_ihlp=rhs->get_reference(0);
		uint16	opcode=rhs_ihlp->code(0).asOpcode();
		if(	opcode==Opcodes::ICST	||
			opcode==Opcodes::IMDL){

			Code			*rhs_hlp=rhs_ihlp->get_reference(0);
			Group			*host=getView()->get_host();
			r_exec::View	*rhs_hlp_v=(r_exec::View*)rhs_hlp->find_view(host,true);
			if(rhs_hlp_v)
				return	(HLPController	*)rhs_hlp_v->controller;
		}
		return	NULL;
	}

	void	MDLController::gain_activation()	const{

		HLPController	*c=get_rhs_controller();
		if(c)
			c->add_requirement();
		
	}

	void	MDLController::lose_activation()	const{

		HLPController	*c=get_rhs_controller();
		if(c)
			c->remove_requirement();
	}

	Code	*MDLController::get_ntf_instance(BindingMap	*bm)	const{

		return	get_instance(bm,Opcodes::IMDL);
	}
	uint16	MDLController::get_instance_opcode()	const{

		return	Opcodes::IMDL;
	}
}