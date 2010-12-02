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
#include	"mem.h"


namespace	r_exec{

	Code	*RGroup::BindObject(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings){

		Code	*bound_object=_Mem::Get()->buildObject(original->code(0));

		uint16	i;
		for(i=0;i<original->code_size();++i)		//	copy the code.
			bound_object->code(i)=original->code(i);
		for(i=0;i<original->references_size();++i)	//	bind references when needed.
			bound_object->set_reference(i,BindReference(original->get_reference(i),bindings));

		return	bound_object;
	}

	Code	*RGroup::BindReference(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings){

		if(original->code(0).asOpcode()==Opcodes::Var){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b=bindings->find(original);
			if(b!=bindings->end())
				return	b->second;
			else
				return	original;
		}

		for(uint16	i=0;i<original->references_size();++i){

			Code	*reference=original->get_reference(i);
			if(NeedsBinding(reference,bindings))
				return	BindObject(original,bindings);
		}

		return	original;
	}

	bool	RGroup::NeedsBinding(Code	*original,UNORDERED_MAP<Code	*,P<Code> >	*bindings){

		if(original->code(0).asOpcode()==Opcodes::Var){

			UNORDERED_MAP<Code	*,P<Code> >::const_iterator	b=bindings->find(original);
			if(b!=bindings->end())
				return	true;
			else
				return	false;
		}

		for(uint16	i=0;i<original->references_size();++i){

			if(NeedsBinding(original->get_reference(i),bindings))
				return	true;
		}

		return	false;
	}

	void	RGroup::injectRGroup(View	*view){

		if(!parent	&&	!substitutions){	//	init the head; this assumes that children are injected from left to right.

			substitutionsCS=new	CriticalSection();
			substitutions=new	UNORDERED_MAP<Code	*,std::pair<Code	*,std::list<RGroup	*> > >();
		}

		((RGroup	*)view->object)->parent=this;
		((RGroup	*)view->object)->substitutions=substitutions;
		((RGroup	*)view->object)->substitutionsCS=substitutionsCS;

		view->controller=new	FwdController(view);
	}

	Code	*RGroup::get_var(Code	*value){

		Code	*var;
		bool	inject=false;

		substitutionsCS->enter();

		// sharp matching on values. TODO: fuzzy matching.
		UNORDERED_MAP<Code	*,std::pair<Code	*,std::list<RGroup	*> > >::iterator	s=substitutions->find(value);
		if(s!=substitutions->end()){	//	variable already exists for the value.

			var=s->second.first;

			std::list<RGroup	*>::const_iterator	g;
			for(g=s->second.second.begin();g!=s->second.second.end();++g)
				if((*g)==this)
					break;

			if(g==s->second.second.end()){	//	variable not injected (yet) in the group.

				s->second.second.push_back(this);
				inject=true;
			}
		}else{	//	no variable exists yet for the value.
			
			var=_Mem::Get()->buildObject(Atom::Object(Opcodes::Var,VAR_ARITY));
			var->code(VAR_ARITY)=Atom::Float(1);	//	psln_thr.

			std::pair<Code	*,std::list<RGroup	*> >	entry;
			entry.first=var;
			entry.second.push_back(this);
			(*substitutions)[value]=entry;

			inject=true;
		}

		substitutionsCS->leave();

		if(inject){	//	inject the variable in the group.

			View	*var_view=new	View(true,Now(),0,-1,this,NULL,var);
			_Mem::Get()->inject(var_view);
		}

		return	var;
	}

	void	RGroup::instantiate_goals(std::vector<Code	*>				*initial_goals,
									  GSMonitor							*initial_monitor,
									  bool								sim,
									  bool								asmp,
									  Code								*inv_model,
									  UNORDERED_MAP<Code	*,P<Code> >	*bindings){	//	instantiates goals and propagates in one single thread; starts in InvController::take_input().

		uint16	out_group_set_index=inv_model->code(MD_OUT_GRPS).asIndex();
		uint16	out_group_count=inv_model->code(out_group_set_index).getAtomCount();

		uint16	ntf_group_set_index=inv_model->code(MD_NTF_GRPS).asIndex();
		uint16	ntf_group_count=inv_model->code(ntf_group_set_index).getAtomCount();

		uint64	now=Now();

		GSMonitor	*gs_monitor=new	GSMonitor((Model	*)inv_model,controller,initial_monitor,sim,asmp);
		controller->add_monitor(gs_monitor);

		std::vector<Code	*>	new_goals;

		//	Build new objects and their respective new goal markers.
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=other_views.begin();v!=other_views.end();++v){	//	we are only interested in objects and markers.

			Code	*bound_object=RGroup::BindObject(v->second->object,bindings);

			Code	*goal=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkGoal,MK_GOAL_ARITY));
			goal->code(MK_GOAL_OBJ)=Atom::RPointer(0);
			goal->code(MK_GOAL_IMD)=Atom::RPointer(1);
			goal->code(MK_GOAL_ARITY)=Atom::Float(1);	//	psln_thr.
			goal->set_reference(0,bound_object);
			goal->set_reference(1,inv_model);

			new_goals.push_back(goal);

			std::vector<Code	*>	sub_goals;
			for(uint16	i=0;i<initial_goals->size();++i){	//	build sub-goal markers for the new goal and inject only this sub-goal marker (in the notification groups of the inverse model).
			
				Code	*mk_sub_goal=_Mem::Get()->buildObject(Atom::Object(Opcodes::MkSubGoal,MK_SUB_GOAL_ARITY));
				mk_sub_goal->code(MK_SUB_GOAL_PARENT)=Atom::RPointer(0);
				mk_sub_goal->code(MK_SUB_GOAL_CHILD)=Atom::RPointer(1);
				mk_sub_goal->code(MK_SUB_GOAL_ARITY)=Atom::Float(1);	//	psln_thr.
				mk_sub_goal->set_reference(0,goal);
				mk_sub_goal->set_reference(1,(*initial_goals)[i]);

				sub_goals.push_back(mk_sub_goal);
			}

			Code	*mk_sim=NULL;
			Code	*mk_asmp=NULL;
			if(!parent){	//	inject the bound object in the model's output groups; monitor the goal.

				//	Add asmp/sim markers to the bound object depending on the initial goal (sim/asmp: similar arrangement as for predictions).
				mk_sim=gs_monitor->get_mk_sim(bound_object);
				mk_asmp=gs_monitor->get_mk_asmp(bound_object);

				for(uint16	i=1;i<=out_group_count;++i){	//	inject the bound object in the model's output groups.

					Code	*out_group=inv_model->get_reference(inv_model->code(out_group_set_index+i).asIndex());
					View	*view=new	View(true,now,1,1,out_group,this,bound_object);
					_Mem::Get()->inject(view);
				}

				GMonitor	*m=new	GMonitor(gs_monitor,goal);
				uint64		expected_time=Utils::GetTimestamp<Code>(bound_object->get_reference(1),VAL_VAL);

				gs_monitor->add_monitor(m);
				_Mem::Get()->pushTimeJob(new	MonitoringJob(m,expected_time));
			}

			for(uint16	i=1;i<=ntf_group_count;++i){	//	inject the sub-goal markers in the model's notification groups.

				Code	*ntf_group=inv_model->get_reference(inv_model->code(ntf_group_set_index+i).asIndex());
				View	*view;

				for(uint16	i=0;i<sub_goals.size();++i){

					view=new	View(true,now,1,_Mem::Get()->get_goal_res(),ntf_group,this,sub_goals[i]);
					_Mem::Get()->inject(view);
				}

				if(!parent){	//	inject the goal/sim/asmp markers in the model's notification groups.

					view=new	View(true,now,1,_Mem::Get()->get_goal_res(),ntf_group,this,goal);
					_Mem::Get()->inject(view);

					if(mk_sim){

						view=new	View(true,now,1,_Mem::Get()->get_sim_res(),ntf_group,this,mk_sim);
						_Mem::Get()->inject(view);
					}

					if(mk_asmp){

						view=new	View(true,now,1,_Mem::Get()->get_asmp_res(),ntf_group,this,mk_asmp);
						_Mem::Get()->inject(view);
					}
				}
			}
		}

		if(parent)	//	propagate.
			parent->instantiate_goals(&new_goals,gs_monitor,sim,asmp,inv_model,bindings);
	}
}