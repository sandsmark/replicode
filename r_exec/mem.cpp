//	mem.cpp
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

#include	"mem.h"
#include	"mdl_controller.h"


namespace	r_exec{

	_Mem::_Mem():r_code::Mem(),state(NOT_STARTED){
	}

	_Mem::~_Mem(){

		if(state==RUNNING	||	state==SUSPENDED)
			stop();
		_root=NULL;
	}

	void	_Mem::init(uint32	base_period,	//	in us; same for upr, spr and res.
						uint32	reduction_core_count,
						uint32	time_core_count,
						uint32	probe_level,
						uint32	ntf_mk_res,
						uint32	goal_res,
						uint32	asmp_res,
						uint32	sim_res,
						float32	float_tolerance,
						float32	time_tolerance){

		this->base_period=base_period;
		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;
		this->probe_level=probe_level;

		this->ntf_mk_res=ntf_mk_res;
		this->goal_res=goal_res;
		this->asmp_res=asmp_res;
		this->sim_res=sim_res;

		this->float_tolerance=float_tolerance;
		this->time_tolerance=time_tolerance;
	}

	void	_Mem::reset(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		delete	reduction_job_queue;
		delete	time_job_queue;

		delete	suspension_lock;
		delete	stop_sem;
		delete	suspend_sem;
	}

	////////////////////////////////////////////////////////////////

	Code	*_Mem::get_root()	const{

		return	_root;
	}

	Code	*_Mem::get_stdin()	const{

		return	_stdin;
	}

	Code	*_Mem::get_stdout()	const{

		return	_stdout;
	}

	Code	*_Mem::get_self()	const{

		return	_self;
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::wait_for_delegate(){

		suspend_sem->acquire(0);
	}

	void	_Mem::delegate_done(){

		suspend_sem->release();
	}

	_Mem::DState	_Mem::check_state(){

		State	s;
		stateCS.enter();
		s=state;
		stateCS.leave();

		switch(state){
		case	STOPPED:
			return	D_STOPPED;
		case	SUSPENDED:
			suspension_lock->wait();
			stateCS.enter();
			if(state==STOPPED){
					
				stateCS.leave();
				return	D_STOPPED_AFTER_SUSPENSION;
			}
			stateCS.leave();
			return	D_RUNNING_AFTER_SUSPENSION;
		case	RUNNING:
			return	D_RUNNING;
		}
	}

	void	_Mem::start_core(){

		core_countCS.enter();
		if(++core_count==1)
			stop_sem->acquire();
		core_countCS.leave();
	}

	void	_Mem::shutdown_core(){

		core_countCS.enter();
		if(--core_count==0)
			stop_sem->release();
		core_countCS.leave();
	}

	bool	_Mem::suspend_core(){

		suspend_sem->release();
		suspension_lock->wait();
		stateCS.enter();
		if(state==STOPPED){
				
			stateCS.leave();
			return	false;
		}
		stateCS.leave();
		return	true;
	}

	////////////////////////////////////////////////////////////////

	uint32	_Mem::get_oid(){

		return	last_oid++;
	}

	////////////////////////////////////////////////////////////////

	uint64	_Mem::start(){

		if(state!=STOPPED	&&	state!=NOT_STARTED)
			return	0;

		core_count=0;
		suspension_lock=new	Event();
		stop_sem=new	Semaphore(1,1);
		suspend_sem=new	Semaphore(1,1);

		time_job_queue=new	PipeNN<P<TimeJob>,1024>();
		reduction_job_queue=new	PipeNN<P<_ReductionJob>,1024>();

		uint32	i;
		uint64	now=starting_time=Now();
		for(i=0;i<initial_groups.size();++i){

			Group	*g=initial_groups[i];
			bool	c_active=g->get_c_act()>g->get_c_act_thr();
			bool	c_salient=g->get_c_sln()>g->get_c_sln_thr();
			
			FOR_ALL_VIEWS_BEGIN(g,v)
				Utils::SetTimestamp<View>(v->second,VIEW_IJT,now);	//	init injection time for the view.
			FOR_ALL_VIEWS_END

			if(c_active){

				UNORDERED_MAP<uint32,P<View> >::const_iterator	v;

				//	build signaling jobs for active input-less overlays.
				for(v=g->input_less_ipgm_views.begin();v!=g->input_less_ipgm_views.end();++v){

					if(v->second->controller!=NULL){

						P<TimeJob>	j=new	InputLessPGMSignalingJob(v->second->controller,now+Utils::GetTimestamp<Code>(v->second->object,IPGM_TSC));
						time_job_queue->push(j);
					}
				}

				//	build signaling jobs for active anti-pgm overlays.
				for(v=g->anti_ipgm_views.begin();v!=g->anti_ipgm_views.end();++v){

					if(v->second->controller!=NULL){

						P<TimeJob>	j=new	AntiPGMSignalingJob(v->second->controller,now+Utils::GetTimestamp<Code>(v->second->object,IPGM_TSC));
						time_job_queue->push(j);
					}
				}

				//	set the initial dependencies for hlp: active models add requirements to other hlp in the same group.
				if(g->get_c_sln()>g->get_c_sln_thr()	&&	g->get_c_act()>g->get_c_act_thr()){

					UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
					for(v=g->ipgm_views.begin();v!=g->ipgm_views.end();++v){

						if(	v->second->object->code(0).getDescriptor()==Atom::MODEL	&&
							v->second->get_act()>g->get_act_thr())
							((MDLController	*)v->second->controller)->gain_activation();
					}
				}
			}

			if(c_salient){

				//	build reduction jobs for each salient view and each active overlay - regardless of the view's sync mode.
				FOR_ALL_VIEWS_BEGIN(g,v)
					
					if(v->second->get_sln()>g->get_sln_thr()){	//	salient view.

						g->newly_salient_views.insert(v->second);
						inject_reduction_jobs(v->second,g);
					}
				FOR_ALL_VIEWS_END
			}
			

			//	inject the next update job for the group.
			if(g->get_upr()>0){

				P<TimeJob>	j=new	UpdateJob(g,now+g->get_upr()*base_period);
				time_job_queue->push(j);
			}
		}

		initial_groups.clear();

		init_timings(now);

		state=RUNNING;

		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->start(ReductionCore::Run);
		for(i=0;i<time_core_count;++i)
			time_cores[i]->start(TimeCore::Run);

		return	now;
	}

	void	_Mem::stop(){

		stateCS.enter();
		if(state!=RUNNING	&&	state!=SUSPENDED){

			stateCS.leave();
			return;
		}
		if(state==SUSPENDED)
			suspension_lock->fire();

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			pushReductionJob(new	ShutdownReductionCore());
		for(i=0;i<time_core_count;++i)
			pushTimeJob(new	ShutdownTimeCore());

		state=STOPPED;
		stateCS.leave();

		for(i=0;i<time_core_count;++i)
			Thread::Wait(time_cores[i]);

		for(i=0;i<reduction_core_count;++i)
			Thread::Wait(reduction_cores[i]);

		stop_sem->acquire();	//	wait for delegates.

		reset();
	}

	void	_Mem::suspend(){

		stateCS.enter();
		if(state!=RUNNING){

			stateCS.leave();
			return;
		}
		suspension_lock->reset();	//	both cores will lock upon receiving suspension jobs.

		uint32	i;
		for(i=0;i<reduction_core_count;++i){

			suspend_sem->acquire(0);
			pushReductionJob(new	SuspendReductionCore());
		}
		for(i=0;i<time_core_count;++i){

			suspend_sem->acquire(0);
			pushTimeJob(new	SuspendTimeCore());
		}
		suspend_sem->acquire();	//	wait for all cores to be suspended.
		state=SUSPENDED;
		stateCS.leave();
	}

	void	_Mem::resume(){

		stateCS.enter();
		if(state!=SUSPENDED){

			stateCS.leave();
			return;
		}
		suspension_lock->fire();
		state=RUNNING;
		stateCS.leave();
	}

	////////////////////////////////////////////////////////////////

	_ReductionJob	*_Mem::popReductionJob(){

		if(state==STOPPED)
			return	NULL;
		return	reduction_job_queue->pop();
	}

	void	_Mem::pushReductionJob(_ReductionJob	*j){

		if(state==STOPPED)
			return;
		P<_ReductionJob>	_j=j;
		reduction_job_queue->push(_j);
	}

	TimeJob	*_Mem::popTimeJob(){

		if(state==STOPPED)
			return	NULL;
		return	time_job_queue->pop();
	}

	void	_Mem::pushTimeJob(TimeJob	*j){

		if(state==STOPPED)
			return;
		P<TimeJob>	_j=j;
		time_job_queue->push(_j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(Code	*command){
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::inject_copy(View	*view,Group	*destination,uint64	now){

		View	*copied_view=new	View(view,destination);	//	ctrl values are morphed.
		Utils::SetTimestamp<View>(copied_view,VIEW_IJT,now);
		inject_existing_object(copied_view,view->object,destination,true);
	}

	void	_Mem::inject_existing_object(View	*view,Code	*object,Group	*host,bool	lock){

		view->set_object(object);	//	the object already exists (content-wise): have the view point to the existing one.

		if(lock)
			host->enter();

		bool	reduce_view=false;

		object->acq_views();
		View	*existing_view=(View	*)object->find_view(host,false);
		if(!existing_view){	//	no existing view: add the view to the group and to the object's view_map.

			switch(object->code(0).getDescriptor()){
			case	Atom::INSTANTIATED_PROGRAM:
			case	Atom::INSTANTIATED_CPP_PROGRAM:
			case	Atom::COMPOSITE_STATE:
			case	Atom::MODEL:
				host->ipgm_views[view->getOID()]=view;
				break;
			case	Atom::INSTANTIATED_ANTI_PROGRAM:
				host->anti_ipgm_views[view->getOID()]=view;
				break;
			case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
				host->input_less_ipgm_views[view->getOID()]=view;
				break;
			case	Atom::OBJECT:
			case	Atom::MARKER:
				host->other_views[view->getOID()]=view;
				break;
			}

			object->views.insert(view);
			object->rel_views();

			reduce_view=view->get_sln()>host->get_sln_thr();
		}else{	//	call set on the ctrl values of the existing view with the new view's ctrl values, including sync. NB: org left unchanged.

			object->rel_views();

			host->pending_operations.push_back(new	Group::Set(existing_view->getOID(),VIEW_RES,view->get_res()));
			host->pending_operations.push_back(new	Group::Set(existing_view->getOID(),VIEW_SLN,view->get_sln()));
			switch(object->code(0).getDescriptor()){
			case	Atom::INSTANTIATED_PROGRAM:
			case	Atom::INSTANTIATED_ANTI_PROGRAM:
			case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
			case	Atom::INSTANTIATED_CPP_PROGRAM:
			case	Atom::COMPOSITE_STATE:
			case	Atom::MODEL:
				host->pending_operations.push_back(new	Group::Set(existing_view->getOID(),VIEW_ACT,view->get_act()));
				break;
			}

			existing_view->code(VIEW_SYNC)=view->code(VIEW_SYNC);
			bool	wiew_is_salient=view->get_sln()>host->get_sln_thr();
			if(existing_view->synced_on_front()){	// sync on front.

				bool	wiew_was_salient=existing_view->get_sln()>host->get_sln_thr();
			
				reduce_view=(!wiew_was_salient	&&	wiew_is_salient);
			}else	// sync on state.
				reduce_view=wiew_is_salient;
		}

		//	give a chance to ipgms to reduce the new view.
		bool	group_is_c_active=host->update_c_act()>host->get_c_act_thr();
		bool	group_is_c_salient=host->update_c_sln()>host->get_c_sln_thr();
		if(group_is_c_active	&&	group_is_c_salient	&&	reduce_view)
			inject_reduction_jobs(view,host);

		if(lock)
			host->leave();
	}

	void	_Mem::update(Group	*group){		
			
		uint64	now=Now();

		group->enter();

		if(group!=_root	&&	group->views.size()==0){

			group->invalidate();
			group->leave();
			return;
		}

		group->newly_salient_views.clear();

		//	execute pending operations.
		for(uint32	i=0;i<group->pending_operations.size();++i){

			group->pending_operations[i]->execute(group);
			delete	group->pending_operations[i];
		}
		group->pending_operations.clear();

		//	update group's ctrl values.
		group->update_sln_thr();	//	applies decay on sln thr. 
		group->update_act_thr();
		group->update_vis_thr();

		GroupState	group_state(group->get_sln_thr(),group->get_c_act()>group->get_c_act_thr(),group->update_c_act()>group->get_c_act_thr(),group->get_c_sln()>group->get_c_sln_thr(),group->update_c_sln()>group->get_c_sln_thr());

		group->reset_stats();

		FOR_ALL_VIEWS_BEGIN_NO_INC(group,v)

			//	update resilience.
			float32	res=group->update_res(v->second);	//	will decrement res by 1 in addition to the accumulated changes.
			if(res>0){

				_update_saliency(group,&group_state,v->second);	//	(apply decay).

				switch(v->second->object->code(0).getDescriptor()){
				case	Atom::GROUP:
					_update_visibility(group,&group_state,v->second);
					break;
				case	Atom::INSTANTIATED_PROGRAM:
				case	Atom::INSTANTIATED_ANTI_PROGRAM:
				case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
				case	Atom::INSTANTIATED_CPP_PROGRAM:
				case	Atom::COMPOSITE_STATE:
				case	Atom::MODEL:
					_update_activation(group,&group_state,v->second);
					break;
				}
				++v;
			}else{	//	view has no resilience.

				v->second->delete_from_object();

				//	delete the view from the group.
				if(v->second->isNotification())
					v=group->notification_views.erase(v);
				else	switch(v->second->object->code(0).getDescriptor()){
				case	Atom::INSTANTIATED_PROGRAM:
				case	Atom::INSTANTIATED_CPP_PROGRAM:
				case	Atom::COMPOSITE_STATE:
				case	Atom::MODEL:
					v=group->ipgm_views.erase(v);
					break;
				case	Atom::INSTANTIATED_ANTI_PROGRAM:
					v=group->anti_ipgm_views.erase(v);
					break;
				case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
					v=group->input_less_ipgm_views.erase(v);
					break;
				case	Atom::OBJECT:
				case	Atom::MARKER:
					v=group->other_views.erase(v);
					break;
				case	Atom::GROUP:
					v=group->group_views.erase(v);
					break;
				}
			}
		FOR_ALL_VIEWS_END

		if(group_state.is_c_salient)
			group->cov(now);

		//	build reduction jobs.
		std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
		for(v=group->newly_salient_views.begin();v!=group->newly_salient_views.end();++v)
			inject_reduction_jobs(*v,group);

		if(group_state.is_c_active	&&	group_state.is_c_salient){	//	build signaling jobs for new ipgms.

			for(uint32	i=0;i<group->new_controllers.size();++i){

				switch(group->new_controllers[i]->getObject()->code(0).getDescriptor()){
				case	Atom::INSTANTIATED_ANTI_PROGRAM:{	//	inject signaling jobs for |ipgm (tsc).

					P<TimeJob>	j=new	AntiPGMSignalingJob((AntiPGMController	*)group->new_controllers[i],now+Utils::GetTimestamp<Code>(group->new_controllers[i]->getObject(),IPGM_TSC));
					time_job_queue->push(j);
					break;
				}case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:{	//	inject a signaling job for an input-less pgm.

					P<TimeJob>	j=new	InputLessPGMSignalingJob((InputLessPGMController	*)group->new_controllers[i],now+Utils::GetTimestamp<Code>(group->new_controllers[i]->getObject(),IPGM_TSC));
					time_job_queue->push(j);
					break;
				}
				}
			}

			group->new_controllers.clear();
		}

		group->update_stats();	//	triggers notifications.

		//	inject the next update job for the group.
		if(group->get_upr()>0){

			P<TimeJob>	j=new	UpdateJob(group,now+group->get_upr()*base_period);
			time_job_queue->push(j);
		}

		group->leave();
	}

	void	_Mem::_update_saliency(Group	*group,GroupState	*state,View	*view){

		float32	view_old_sln=view->get_sln();
		bool	wiew_was_salient=view_old_sln>state->former_sln_thr;
		float32	view_new_sln=group->update_sln(view);
		bool	wiew_is_salient=view_new_sln>group->get_sln_thr();

		if(state->is_c_salient){
			
			if(wiew_is_salient){

				if(view->synced_on_front()){
					
					if(!wiew_was_salient)	// sync on front: crosses the threshold upward: record as a newly salient view.
						group->newly_salient_views.insert(view);
				}else						// sync on state.
					group->newly_salient_views.insert(view);
			}

			//	inject sln propagation jobs.
			//	the idea is to propagate sln changes when a view "occurs to the mind", i.e. becomes more salient in a group and is eligible for reduction in that group.
			//		- when a view is now salient because the group becomes c-salient, no propagation;
			//		- when a view is now salient because the group's sln_thr gets lower, no propagation;
			//		- propagation can occur only if the group is c_active. For efficiency reasons, no propagation occurs even if some of the group's viewing groups are c-active and c-salient.
			if(state->is_c_active)
				_initiate_sln_propagation(view->object,view_new_sln-view_old_sln,group->get_sln_thr());
		}
	}

	void	_Mem::_update_visibility(Group	*group,GroupState	*state,View	*view){

		bool	view_was_visible=view->get_vis()>group->get_vis_thr();
		bool	view_is_visible=view->update_vis()>group->get_vis_thr();
		bool	cov=view->get_cov();

		//	update viewing groups.
		if(state->was_c_active	&&	state->was_c_salient){

			if(!state->is_c_active	||	!state->is_c_salient)	//	group is not c-active and c-salient anymore: unregister as a viewing group.
				((Group	*)view->object)->viewing_groups.erase(group);
			else{	//	group remains c-active and c-salient.

				if(!view_was_visible){
					
					if(view_is_visible)		//	newly visible view.
						((Group	*)view->object)->viewing_groups[group]=cov;
				}else{
					
					if(!view_is_visible)	//	the view is no longer visible.
						((Group	*)view->object)->viewing_groups.erase(group);
					else					//	the view is still visible, cov might have changed.
						((Group	*)view->object)->viewing_groups[group]=cov;
				}
			}
		}else	if(state->is_c_active	&&	state->is_c_salient){	//	group becomes c-active and c-salient.

			if(view_is_visible)		//	update viewing groups for any visible group.
				((Group	*)view->object)->viewing_groups[group]=cov;
		}
	}

	void	_Mem::_update_activation(Group	*group,GroupState	*state,View	*view){

		bool	view_was_active=view->get_act()>group->get_act_thr();
		bool	view_is_active=group->update_act(view)>group->get_act_thr();

		//	kill newly inactive controllers, register newly active ones.
		if(state->was_c_active	&&	state->was_c_salient){

			if(!state->is_c_active	||	!state->is_c_salient)	//	group is not c-active and c-salient anymore: kill the view's controller.
				view->controller->kill();
			else{	//	group remains c-active and c-salient.

				if(!view_was_active){
			
					if(view_is_active){	//	register the controller for the newly active ipgm view.

						view->controller->gain_activation();
						group->new_controllers.push_back(view->controller);
					}
				}else{
					
					if(!view_is_active){	//	kill the newly inactive ipgm view's overlays.

						view->controller->lose_activation();
						view->controller->kill();
					}
				}
			}
		}else	if(state->is_c_active	&&	state->is_c_salient){	//	group becomes c-active and c-salient.

			if(view_is_active){	//	register the controller for any active ipgm view.

				view->controller->gain_activation();
				group->new_controllers.push_back(view->controller);
			}
		}
	}

	void	_Mem::inject_reduction_jobs(View	*view,Group	*host){	//	host is assumed to be c-salient; host already protected.

		if(host->get_c_act()>host->get_c_act_thr()){	//	host is c-active.

			//	build reduction jobs from host's own inputs and own overlays.
			FOR_ALL_VIEWS_WITH_INPUTS_BEGIN(host,v)

				if(v->second->get_act()>host->get_act_thr())		//	active ipgm/icpp_pgm/rgrp view.
					v->second->controller->take_input(view);	//	view will be copied.

			FOR_ALL_VIEWS_WITH_INPUTS_END
		}

		//	build reduction jobs from host's own inputs and overlays from viewing groups, if no cov and view is not a notification.
		//	NB: visibility is not transitive;
		//	no shadowing: if a view alresady exists in the viewing group, there will be twice the reductions: all of the identicals will be trimmed down at injection time.
		UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
		for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

			if(vg->second	||	view->isNotification())	//	no reduction jobs when cov==true or view is a notification.
				continue;

			FOR_ALL_VIEWS_WITH_INPUTS_BEGIN(vg->first,v)

				if(v->second->get_act()>vg->first->get_act_thr())	//	active ipgm/icpp_pgm/rgrp view.
					v->second->controller->take_input(view);	//	view will be copied.
			
			FOR_ALL_VIEWS_WITH_INPUTS_END
		}
	}

	void	_Mem::propagate_sln(Code	*object,float32	change,float32	source_sln_thr){

		//	apply morphed change to views.
		//	loops are prevented within one call, but not accross several upr:
		//		- feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop spreading accross several upr.
		//		- to avoid this, have the psln_thr set to 1 in o2: this is applicaton-dependent.
		object->acq_views();

		if(object->views.size()==0){

			object->invalidate();
			object->rel_views();
			return;
		}

		UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	it;
		for(it=object->views.begin();it!=object->views.end();++it){

			float32	morphed_sln_change=View::MorphChange(change,source_sln_thr,((r_exec::View*)*it)->get_host()->get_sln_thr());
			if(morphed_sln_change!=0)
				((r_exec::View*)*it)->get_host()->pending_operations.push_back(new	Group::Mod(((r_exec::View*)*it)->getOID(),VIEW_SLN,morphed_sln_change));
		}
		object->rel_views();
	}

	void	_Mem::_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr){
		
		if(fabs(change)>object->get_psln_thr()){

			std::vector<Code	*>	path;
			path.push_back(object);

			if(object->code(0).getDescriptor()==Atom::MARKER){	//	if marker, propagate to references.

				for(uint16	i=0;i<object->references_size();++i)
					_propagate_sln(object->get_reference(i),change,source_sln_thr,path);
			}

			//	propagate to markers
			object->acq_markers();
			std::list<Code	*>::const_iterator	m;
			for(m=object->markers.begin();m!=object->markers.end();++m)
				_propagate_sln(*m,change,source_sln_thr,path);
			object->rel_markers();
		}
	}

	void	_Mem::_initiate_sln_propagation(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path){
		
		if(fabs(change)>object->get_psln_thr()){

			//	prevent loops.
			for(uint32	i=0;i<path.size();++i)
				if(path[i]==object)
					return;
			path.push_back(object);

			if(object->code(0).getDescriptor()==Atom::MARKER)	//	if marker, propagate to references.
				for(uint16	i=0;i<object->references_size();++i)
					_propagate_sln(object->get_reference(i),change,source_sln_thr,path);

			//	propagate to markers
			object->acq_markers();
			std::list<Code	*>::const_iterator	m;
			for(m=object->markers.begin();m!=object->markers.end();++m)
				_propagate_sln(*m,change,source_sln_thr,path);
			object->rel_markers();
		}
	}

	void	_Mem::_propagate_sln(Code	*object,float32	change,float32	source_sln_thr,std::vector<Code	*>	&path){

		if(object==_root)
			return;

		//	prevent loops.
		for(uint32	i=0;i<path.size();++i)
			if(path[i]==object)
				return;
		path.push_back(object);

		P<TimeJob>	j=new	SaliencyPropagationJob(object,change,source_sln_thr,0);
		time_job_queue->push(j);
		
		_initiate_sln_propagation(object,change,source_sln_thr,path);
	}

	////////////////////////////////////////////////////////////////

	Code	*_Mem::clone_object(Code	*object){

		Code	*clone=build_object(object->code(0));
		for(uint16	i=0;i<object->code_size();++i)
			clone->code(i)=object->code(i);
		for(uint16	i=0;i<object->references_size();++i)
			clone->set_reference(i,object->get_reference(i));
		return	clone;
	}

	Code	*_Mem::abstract_object_clone(Code	*object){

		BindingMap	bm;
		return	abstract_object_clone(object,&bm);
	}

	Code	*_Mem::abstract_object_clone(Code	*object,BindingMap	*bm){

		Code	*abstracted_object;

		uint16	opcode=object->code(0).asOpcode();
		switch(object->code(0).getDescriptor()){
		case	Atom::COMPOSITE_STATE:
		case	Atom::MODEL:
			abstracted_object=clone_object(object);
			abstract_object_member_clone(abstracted_object,HLP_OBJS,bm);
			break;
		case	Atom::OBJECT:
			if(	opcode==Opcodes::Fact	||
				opcode==Opcodes::AntiFact){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,FACT_OBJ,bm);
				abstract_object_member_clone(abstracted_object,FACT_TIME,bm);
			}else	if(opcode==Opcodes::Cmd){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,CMD_ARGS,bm);
			}else	if(	opcode==Opcodes::ICST	||
						opcode==Opcodes::IMDL){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,I_HLP_ARGS,bm);
			}else	if(opcode==Opcodes::Ent)
				return	bm->get_variable_object(object);
			else
				return	object;
			break;
		case	Atom::MARKER:
			if(opcode==Opcodes::MkVal){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,MK_VAL_OBJ,bm);
				abstract_object_member_clone(abstracted_object,MK_VAL_VALUE,bm);
			}else	if(opcode==Opcodes::MkGoal){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,MK_GOAL_ACTR,bm);
				abstract_object_member_clone(abstracted_object,MK_GOAL_OBJ,bm);
			}else	if(opcode==Opcodes::MkPred){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,MK_PRED_OBJ,bm);
			}else	if(opcode==Opcodes::MkSuccess){

				abstracted_object=clone_object(object);
				abstract_object_member_clone(abstracted_object,MK_SUCCESS_OBJ,bm);
			}else
				return	object;
			break;
		default:
			return	object;
		}

		return	abstracted_object;
	}

	void	_Mem::abstract_object_member_clone(Code	*object,uint16	index,BindingMap	*bm){

		Atom	a=object->code(index);
		uint16	ai=a.asIndex();
		switch(a.getDescriptor()){
		case	Atom::R_PTR:
			object->set_reference(ai,abstract_object_clone(object->get_reference(ai),bm));
			break;
		case	Atom::I_PTR:
			if(object->code(ai).getDescriptor()==Atom::SET)
				for(uint16	i=1;i<=object->code(ai).getAtomCount();++i)
					abstract_object_member_clone(object,ai+i,bm);
			else
				object->code(ai+1)=bm->get_structural_variable(object,ai);
			break;
		default:
			object->code(index)=bm->get_atomic_variable(object,index);
			break;
		}
	}

	////////////////////////////////////////////////////////////////
	
	void	_Mem::abstract_high_level_pattern(r_code::Code	*object,BindingMap	*bm){

		abstract_object_member(object,HLP_OBJS,bm);
	}

	void	_Mem::abstract_object(r_code::Code	*object,BindingMap	*bm){

		uint16	opcode=object->code(0).asOpcode();
		switch(object->code(0).getDescriptor()){
		case	Atom::OBJECT:
			if(	opcode==Opcodes::Fact	||
				opcode==Opcodes::AntiFact){

				abstract_object_member(object,FACT_OBJ,bm);
				abstract_object_member(object,FACT_TIME,bm);
			}else	if(opcode==Opcodes::Cmd)
				abstract_object_member(object,CMD_ARGS,bm);
			else	if(	opcode==Opcodes::ICST	||
						opcode==Opcodes::IMDL)
				abstract_object_member(object,I_HLP_ARGS,bm);
			else	if(opcode==Opcodes::Ent)
				bm->set_variable_object(object);	//	will mutate the descriptor into a variable object's. Any object referring to this object will thus point to a variable.
			break;
		case	Atom::MARKER:
			if(opcode==Opcodes::MkVal){

				abstract_object_member(object,MK_VAL_OBJ,bm);
				abstract_object_member(object,MK_VAL_VALUE,bm);
			}else	if(opcode==Opcodes::MkGoal){

				abstract_object_member(object,MK_GOAL_ACTR,bm);
				abstract_object_member(object,MK_GOAL_OBJ,bm);
			}else	if(opcode==Opcodes::MkPred)
				abstract_object_member(object,MK_PRED_OBJ,bm);
			else	if(opcode==Opcodes::MkSuccess)
				abstract_object_member(object,MK_SUCCESS_OBJ,bm);
			break;
		}
	}

	void	_Mem::abstract_object_member(r_code::Code	*object,uint16	index,BindingMap	*bm){

		Atom	a=object->code(index);
		uint16	ai=a.asIndex();
		switch(a.getDescriptor()){
		case	Atom::R_PTR:
			abstract_object(object->get_reference(ai),bm);
			break;
		case	Atom::I_PTR:
			if(object->code(ai).getDescriptor()==Atom::SET)
				for(uint16	i=1;i<=object->code(ai).getAtomCount();++i)
					abstract_object_member(object,ai+i,bm);
			else
				object->code(ai+1)=bm->get_structural_variable(object,ai);
			break;
		default:
			object->code(index)=bm->get_atomic_variable(object,index);
			break;
		}
	}

	////////////////////////////////////////////////////////////////

	r_exec_dll r_exec::Mem<r_exec::LObject> *Run(const	char	*user_operator_library_path,
		uint64			(*time_base)(),
		const	char	*seed_path,
		const	char	*source_file_name)
	{
		r_exec::Init(user_operator_library_path,time_base,seed_path );

		srand(r_exec::Now());
		Random::Init();

		std::string	error;
		r_exec::Compile(source_file_name,error);

		r_exec::Mem<r_exec::LObject> *mem = new r_exec::Mem<r_exec::LObject>();

		r_code::vector<r_code::Code	*>	ram_objects;
		r_exec::Seed.getObjects(mem,ram_objects);

		mem->init(	100000,
			3,
			1,
			2,
			1000,
			1000,
			1000,
			1000,
			0.1,
			0.1);

		mem->load( ram_objects.as_std() );

		return mem;
	}
}