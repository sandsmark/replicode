//	cst_controller.cpp
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

#include	"cst_controller.h"
#include	"cst_g_monitor.h"
#include	"mem.h"


namespace	r_exec{

	CSTOverlay::CSTOverlay(Controller	*c,BindingMap	*bindings,uint8	reduction_mode):HLPOverlay(c,bindings,reduction_mode){

		birth_time=Now();
	}

	CSTOverlay::CSTOverlay(const	CSTOverlay	*original):HLPOverlay(original->controller,original->bindings,original->reduction_mode){

		patterns=original->patterns;
		inputs=original->inputs;
		birth_time=Now();
	}

	CSTOverlay::~CSTOverlay(){
	}

	void	CSTOverlay::load_patterns(){

		uint16	obj_set_index=getObject()->code(CST_OBJS).asIndex();
		uint16	obj_count=getObject()->code(obj_set_index).getAtomCount();
		for(uint16	i=1;i<=obj_count;++i)
			patterns.push_back(getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex()));
	}

	void	CSTOverlay::inject_instance(){

		uint64	now=Now();
		uint32	resilience=0;
		
		Code	*instance=((HLPController	*)controller)->get_instance(bindings,Opcodes::ICST);
		Code	*instance_fact=factory::Object::Fact(instance,now,1,1);
		Code	*mk_rdx=factory::Object::MkRdx(instance_fact,&inputs,1);
		Code	*mk_sim_instance=get_mk_sim(instance_fact);
		Code	*mk_asmp_instance=get_mk_asmp(instance_fact);

		Group	*origin=getView()->get_host();
		uint16	out_group_count=((HLPController	*)controller)->get_out_group_count();
		for(uint16	i=1;i<=out_group_count;++i){

			Group	*out_group=(Group	*)((HLPController	*)controller)->get_out_group(i);
			uint64	base=_Mem::Get()->get_base_period()*out_group->get_upr();
			uint32	res;
			if(resilience<=0)
				res=1;
			else
				res=Utils::GetResilience(resilience,base);

			View	*view=new	View(true,now,1,res,out_group,origin,instance);	//	SYNC_FRONT,sln=1,res=resilience.
			_Mem::Get()->inject(view);
			
			view=new	View(true,now,1,res,out_group,origin,instance_fact);	//	SYNC_FRONT,sln=1,res=resilience.
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
		}
	}

	Overlay	*CSTOverlay::reduce(View	*input){
//std::cout<<std::hex<<this<<std::dec<<" "<<input->object->getOID();
		BindingMap	*bm=new	BindingMap(bindings);
		std::list<Code	*>::const_iterator	p;
		Code	*bound_pattern=NULL;
		for(p=patterns.begin();p!=patterns.end();++p){

			if(bm->match(input->object,*p)){

				bound_pattern=*p;
				break;
			}
		}

		if(bound_pattern){

			CSTOverlay	*offspring=new	CSTOverlay(this);
			bindings=bm;
			patterns.remove(bound_pattern);
			inputs.push_back(input->object);
			if(input->object->get_sim()	||	input->object->get_hyp())
				reduction_mode|=RDX_MODE_SIMULATION;
			if(input->object->get_asmp())
				reduction_mode|=RDX_MODE_ASSUMPTION;
			if(!patterns.size()){
//std::cout<<" full";
				kill();
				inject_instance();
			}
//std::cout<<" match\n";
			return	offspring;
		}else{
//std::cout<<" no match\n";
			delete	bm;
			return	NULL;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CSTController::CSTController(r_code::View	*view):HLPController(view){

		CSTOverlay	*o=new	CSTOverlay(this,bindings,RDX_MODE_REGULAR);	//	master overlay.
		o->load_patterns();
		overlays.push_back(o);
	}

	CSTController::~CSTController(){
	}

	void	CSTController::take_input(r_exec::View	*input){

		HLPController::_take_input<CSTController>(input);
	}

	void	CSTController::reduce(r_exec::View	*input){

		Overlay	*offspring=NULL;
		std::list<P<Overlay> >::const_iterator	o;
		reductionCS.enter();
		if(!input->object->get_goal()){

			for(o=overlays.begin();o!=overlays.end();){

				if(tsc>0	&&	Now()-((CSTOverlay	*)*o)->get_birth_time()>tsc)
					o=overlays.erase(o);
				else	if((*o)->is_alive()){

					offspring=(*o)->reduce(input);
					++o;
					if(offspring)
						overlays.push_front(offspring);
				}else
					o=overlays.erase(o);
			}
		}

		if(!offspring){	//	no match.

			if(!monitor(input->object)){	//	no luck catching (icst getObject() [args]), or there is no such goals.

				Code	*goal=input->object->get_reference(0);
				uint16	opcode=goal->code(0).asOpcode();
				if(opcode==Opcodes::MkGoal){

					Code	*target=goal->get_reference(0)->get_reference(0);
					if(target->code(0).asOpcode()==Opcodes::ICST	&&	target->get_reference(0)==getObject()){

						if(!requirement_count){

							BindingMap	bm(bindings);
							bm.load(target);
							produce_goals(input->object,&bm);
						}
					}else{	//	check if the goal matches one pattern.

						uint16	obj_set_index=getObject()->code(CST_OBJS).asIndex();
						uint16	obj_count=getObject()->code(obj_set_index).getAtomCount();
						for(uint16	i=1;i<=obj_count;++i){

							Code		*pattern=getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex());
							BindingMap	*bm=new	BindingMap(bindings);
							if(bm->match(goal->get_reference(0),pattern)){	//	the fact pointed by a goal matches one pattern.

								if(!requirement_count)
									produce_goals(input->object,bm,pattern);
								else
									produce_sub_goal(bm,input->object,pattern,get_instance(bm,Opcodes::ICST),true);
								break;
							}else
								delete	bm;
						}
					}
				}
			}
		}
		reductionCS.leave();
	}

	void	CSTController::produce_goals(Code	*super_goal,BindingMap	*bm,Code	*excluded_pattern){

		uint16	obj_set_index=getObject()->code(CST_OBJS).asIndex();
		uint16	obj_count=getObject()->code(obj_set_index).getAtomCount();
		for(uint16	i=1;i<=obj_count;++i){

			Code	*pattern=getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex());
			if(pattern==excluded_pattern)
				continue;
			Code	*bound_pattern=bm->bind_object(getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex()));
			produce_sub_goal(bm,super_goal,bound_pattern,NULL,false);
		}
	}

	void	CSTController::produce_goals(Code	*super_goal,BindingMap	*bm){

		uint16	obj_set_index=getObject()->code(CST_OBJS).asIndex();
		uint16	obj_count=getObject()->code(obj_set_index).getAtomCount();
		for(uint16	i=1;i<=obj_count;++i){

			Code	*pattern=getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex());
			Code	*bound_pattern=bm->bind_object(getObject()->get_reference(getObject()->code(obj_set_index+i).asIndex()));
			produce_sub_goal(bm,super_goal,bound_pattern,NULL,false);
		}
	}

	void	CSTController::add_monitor(	BindingMap	*bindings,
										Code		*goal,
										Code		*super_goal,
										Code		*matched_pattern,
										uint64		expected_time_high,
										uint64		expected_time_low){

		HLPController::add_monitor<CSTGMonitor>(new	CSTGMonitor(this,bindings,goal,super_goal,matched_pattern,expected_time_high));
	}

	Code	*CSTController::get_ntf_instance(BindingMap	*bm)	const{

		return	get_instance(bm,Opcodes::ICST);
	}

	uint16	CSTController::get_instance_opcode()	const{

		return	Opcodes::ICST;
	}
}