//	r_group.cpp
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

#include	"r_group.h"
#include	"rgrp_controller.h"
#include	"mem.h"


namespace	r_exec{

	void	RGroup::injectRGroup(View	*view){

		if(!parent	&&	!substitutions){	//	init the head; this assumes that children are injected from left to right.

			substitutionsCS=new	CriticalSection();
			substitutions=new	BindingMap();
		}

		((RGroup	*)view->object)->parent=this;
		((RGroup	*)view->object)->substitutions=substitutions;
		((RGroup	*)view->object)->substitutionsCS=substitutionsCS;

		FwdController	*c=new	FwdController(view);
		view->controller=c;
		((RGroup	*)view->object)->set_controller(c);
	}

	Atom	RGroup::get_numerical_variable(Atom	value,float32	tolerance){	//	tolerance used for finding the least deviation from the value.
																			//	in case of a draw, take the first.
		Atom	var;

		float32	min=value.asFloat()*(1-tolerance);
		float32	max=value.asFloat()*(1+tolerance);

		substitutionsCS->enter();
		UNORDERED_MAP<Atom,Atom>::const_iterator	s;
		for(s=substitutions->atoms.begin();s!=substitutions->atoms.end();++s)
			if(s->second.asFloat()>=min	&&
				s->second.asFloat()<=max){	//	variable already exists for the value.

				var=s->first;
				break;
			}
		
		if(!var){

			var=Atom::NumericalVariable(substitutions->atoms.size(),Atom::GetTolerance(tolerance));
			substitutions->atoms[var]=value;
			numerical_variables.push_back(var);
		}else{

			uint16	i;
			for(i=0;i<numerical_variables.size();++i)
				if(var==numerical_variables[i])
					break;
			if(i==numerical_variables.size())
				numerical_variables.push_back(var);
		}
		substitutionsCS->leave();

		return	var;
	}

	Atom	RGroup::get_structural_variable(Atom	*value,float32	tolerance){	//	structure is a timestamp, a string or any user-defined structure.

		Atom	var;

		substitutionsCS->enter();
		switch(value[0].getDescriptor()){
		case	Atom::TIMESTAMP:{	//	tolerance is used.
			
			uint64	t=Utils::GetTimestamp(value);
			uint64	min;
			uint64	max;
			uint64	now;
			if(t>0){

				now=Now();
				int64	delta_t=now-t;
				if(delta_t<0)
					delta_t=-delta_t;
				min=delta_t*(1-tolerance);
				max=delta_t*(1+tolerance);
			}else
				min=max=0;

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
			for(s=substitutions->structures.begin();s!=substitutions->structures.end();++s){

				if(value[0]==s->second[0]){

					int64	_t=Utils::GetTimestamp(&s->second[0]);
					if(t>0)
						_t-=now;
					if(_t<0)
						_t=-_t;
					if(_t>=min	&&	_t<=max){	//	variable already exists for the value.

						var=s->first;
						break;
					}
				}
			}
			break;
		}case	Atom::STRING:{	//	tolerance is not used.

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
			for(s=substitutions->structures.begin();s!=substitutions->structures.end();++s){

				if(value[0]==s->second[0]){

					uint16	i;
					for(i=1;i<=value[0].getAtomCount();++i)
						if(value[i]!=s->second[i])
							break;
					if(i==value[0].getAtomCount()+1){

						var=s->first;
						break;
					}
				}
			}
			break;
		}case	Atom::OBJECT:{	//	tolerance is used to compare numerical structure members.

			UNORDERED_MAP<Atom,std::vector<Atom> >::const_iterator	s;
			for(s=substitutions->structures.begin();s!=substitutions->structures.end();++s){

				if(value[0]==s->second[0]){

					uint16	i;
					for(i=1;i<=value[0].getAtomCount();++i){

						if(value[i].isFloat()){

							float32	min=value[i].asFloat()*(1-tolerance);
							float32	max=value[i].asFloat()*(1+tolerance);
							if(s->second[i].asFloat()<min	||	s->second[i].asFloat()>max)
								break;
						}else	if(value[i]!=s->second[i])
							break;
					}
					if(i==value[0].getAtomCount()+1){

						var=s->first;
						break;
					}
				}
			}
			break;
		}
		}

		if(!var){

			var=Atom::StructuralVariable(substitutions->structures.size(),Atom::GetTolerance(tolerance));
			std::vector<Atom>	_value;
			for(uint16	i=0;i<=value[0].getAtomCount();++i)
				_value.push_back(value[i]);
			substitutions->structures[var]=_value;
			structural_variables.push_back(var);
		}else{

			uint16	i;
			for(i=0;i<structural_variables.size();++i)
				if(var==structural_variables[i])
					break;
			if(i==structural_variables.size())
				structural_variables.push_back(var);
		}
		substitutionsCS->leave();

		return	var;
	}

	Code	*RGroup::get_variable_object(Code	*value,float32	tolerance){

		Code	*var=NULL;
		bool	exists=false;

		substitutionsCS->enter();
		UNORDERED_MAP<Code	*,P<Code> >::const_iterator		s;
		for(s=substitutions->objects.begin();s!=substitutions->objects.end();++s)
			if(s->second==value){	//	variable already exists for the value. TODO: apply tolerance on a distance (TBD) between an existing value and the requested value.

				var=s->first;
				break;
			}
		
		if(!var){

			var=factory::Object::Var(tolerance,1);

			P<Code>	p=value;
			substitutions->objects.insert(std::pair<Code	*,P<Code> >(var,p));
		}
		substitutionsCS->leave();

		View	*var_view=new	View(true,Now(),0,-1,this,NULL,var);	//	inject in r-grp: overlays will be initialized with the variables they need.
		_Mem::Get()->inject(var_view);

		return	var;
	}

	void	RGroup::instantiate_goals(std::vector<Code	*>	*initial_goals,GSMonitor	*initial_monitor,uint8	reduction_mode,Code	*inv_model,BindingMap	*bindings){	//	instantiates goals and propagates in one single thread; starts in InvController::take_input().

		uint16	ntf_group_set_index=inv_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=inv_model->code(ntf_group_set_index).getAtomCount();

		uint64	now=Now();

		GSMonitor	*gs_monitor;

		if(initial_monitor){

			std::vector<Code	*>	new_goals;

			//	Build new objects and their respective new goal markers.
			UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
			for(v=other_views.begin();v!=other_views.end();++v){	//	we are only interested in objects and markers.

				Code	*original=v->second->object;
				Code	*bound_object=bindings->bind_object(original);
				Code	*goal=factory::Object::MkGoal(bound_object,inv_model,1);
				new_goals.push_back(goal);

				std::vector<Code	*>	sub_goals;
				for(uint16	i=0;i<initial_goals->size();++i){	//	build sub-goal markers for the new goal.
				
					Code	*mk_sub_goal=factory::Object::MkSubGoal((*initial_goals)[i],goal,1);
					sub_goals.push_back(mk_sub_goal);
				}

				for(uint16	i=1;i<=ntf_group_count;++i){	//	inject the sub-goal markers in the model's notification groups.

					Code	*ntf_group=inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
					View	*view;

					for(uint16	i=0;i<sub_goals.size();++i){

						view=new	View(true,now,1,_Mem::Get()->get_goal_res(),ntf_group,this,sub_goals[i]);
						_Mem::Get()->inject(view);
					}
				}
			}

			gs_monitor=new	GSMonitor((Model	*)inv_model,controller,initial_monitor,this,bindings,reduction_mode);	//	bindings are according to the new goals.
			gs_monitor->set_goals(new_goals);

			if(parent)	//	propagate.
				parent->instantiate_goals(&new_goals,gs_monitor,reduction_mode,inv_model,bindings);
		}else{	
		
			gs_monitor=new	GSMonitor((Model	*)inv_model,controller,initial_monitor,this,bindings,reduction_mode);	//	original bindings.
			gs_monitor->set_goals(*initial_goals);

			if(parent)	//	propagate.
				parent->instantiate_goals(initial_goals,gs_monitor,reduction_mode,inv_model,bindings);
		}

		controller->add_monitor(gs_monitor);
		
		if(!parent)
			gs_monitor->instantiate();
	}
}