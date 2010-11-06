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

namespace	r_exec{

	_Mem::_Mem():state(NOT_STARTED){
	}

	_Mem::~_Mem(){

		if(state==RUNNING	||	state==SUSPENDED)
			stop();
		root=NULL;
	}

	void	_Mem::init(uint32	base_period,	//	in us; same for upr, spr and res.
						uint32	reduction_core_count,
						uint32	time_core_count,
						uint32	ntf_mk_res){

		this->base_period=base_period;
		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;

		this->ntf_mk_res=ntf_mk_res;
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
++CoreCount;
		stop_sem->acquire(0);
	}

	void	_Mem::shutdown_core(){
--CoreCount;
		stop_sem->release();
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
CoreCount=0;
		suspension_lock=new	Event();
		stop_sem=new	Semaphore(1,1);
		suspend_sem=new	Semaphore(1,1);

		time_job_queue=new	PipeNN<P<TimeJob>,1024>();
		reduction_job_queue=new	PipeNN<P<_ReductionJob>,1024>();

		uint32	i;
		uint64	now=Now();
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

				if(c_salient){

					//	build reduction jobs for each salient view and each active overlay - regardless of the view's sync mode.
					FOR_ALL_VIEWS_BEGIN(g,v)
						
						if(v->second->get_sln()>g->get_sln_thr()){	//	salient view.

							g->newly_salient_views.insert(v->second);
							_inject_reduction_jobs(v->second,g);
						}
					FOR_ALL_VIEWS_END
				}
			}

			//	inject the next update job for the group.
			if(g->get_upr()>0){

				P<TimeJob>	j=new	UpdateJob(g,now+g->get_upr()*base_period);
				time_job_queue->push(j);
			}
		}

		initial_groups.clear();

		state=RUNNING;

		for(i=0;i<reduction_core_count;++i){
++CoreCount;
			stop_sem->acquire(0);
			reduction_cores[i]->start(ReductionCore::Run);
		}
		for(i=0;i<time_core_count;++i){
++CoreCount;
			stop_sem->acquire(0);
			time_cores[i]->start(TimeCore::Run);
		}

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
		state=STOPPED;
		
		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			pushReductionJob(new	ShutdownReductionCore());
		for(i=0;i<time_core_count;++i)
			pushTimeJob(new	ShutdownTimeCore());
		stateCS.leave();

		Thread::Sleep(200);
		stop_sem->acquire();	//	wait for the cores and delegates to terminate.
		
		if(CoreCount){	//	HACK: at this point, CoreCount shall be 0. BUG: sometimes it's not: a delegate is still waiting on timer.

			for(uint32	i=0;i<CoreCount;++i)
				stop_sem->acquire(0);
			stop_sem->acquire();
		}

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

		return	reduction_job_queue->pop();
	}

	void	_Mem::pushReductionJob(_ReductionJob	*j){

		P<_ReductionJob>	_j=j;
		reduction_job_queue->push(_j);
	}

	TimeJob	*_Mem::popTimeJob(){

		return	time_job_queue->pop();
	}

	void	_Mem::pushTimeJob(TimeJob	*j){

		P<TimeJob>	_j=j;
		time_job_queue->push(_j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(Code	*object,uint16	nodeID){
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::injectCopyNow(View	*view,Group	*destination,uint64	now){

		View	*copied_view=new	View(view,destination);	//	ctrl values are morphed.
		Utils::SetTimestamp<View>(copied_view,VIEW_IJT,now);
		injectExistingObjectNow(copied_view,view->object,destination,true);
	}

	void	_Mem::injectExistingObjectNow(View	*view,Code	*object,Group	*host,bool	lock){

		view->set_object(object);	//	the object already exists (content-wise): have the view point to the existing one.

		if(lock)
			host->enter();

		bool	reduce_view=false;

		object->acq_views();
		View	*existing_view=(View	*)object->find_view(host,false);
		if(!existing_view){	//	no existing view: add the view to the group and to the object's view_map.

			switch(GetType(object)){
			case	ObjectType::IPGM:
				host->ipgm_views[view->getOID()]=view;
				break;
			case	ObjectType::ICPP_PGM:
				host->icpp_pgm_views[view->getOID()]=view;
				break;
			case	ObjectType::ANTI_IPGM:
				host->anti_ipgm_views[view->getOID()]=view;
				break;
			case	ObjectType::INPUT_LESS_IPGM:
				host->input_less_ipgm_views[view->getOID()]=view;
				break;
			case	ObjectType::OBJECT:
			case	ObjectType::MARKER:
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
			switch(GetType(object)){
			case	ObjectType::IPGM:
			case	ObjectType::ICPP_PGM:
			case	ObjectType::ANTI_IPGM:
			case	ObjectType::INPUT_LESS_IPGM:
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
			_inject_reduction_jobs(view,host);

		if(lock)
			host->leave();
	}

	void	_Mem::update(Group	*group){		
			
		uint64	now=Now();

		group->enter();

		if(group!=root	&&	group->views.size()==0){

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

		float32	former_sln_thr=group->get_sln_thr();

		//	update group's ctrl values.
		group->update_sln_thr();	//	applies decay on sln thr. 
		group->update_act_thr();
		group->update_vis_thr();

		bool	group_was_c_active=group->get_c_act()>group->get_c_act_thr();
		bool	group_is_c_active=group->update_c_act()>group->get_c_act_thr();
		bool	group_was_c_salient=group->get_c_sln()>group->get_c_sln_thr();
		bool	group_is_c_salient=group->update_c_sln()>group->get_c_sln_thr();

		group->reset_stats();

		FOR_ALL_VIEWS_BEGIN_NO_INC(group,v)

			//	update resilience.
			float32	res=group->update_res(v->second,this);	//	will decrement res by 1 in addition to the accumulated changes.
			if(res>0){

				//	update saliency (apply decay).
				float32	view_old_sln=v->second->get_sln();
				bool	wiew_was_salient=view_old_sln>former_sln_thr;
				float32	view_new_sln=group->update_sln(v->second,this);
				bool	wiew_is_salient=view_new_sln>group->get_sln_thr();

				if(group_is_c_salient){
					
					if(wiew_is_salient){

						if(v->second->synced_on_front()){
							
							if(!wiew_was_salient)	// sync on front: crosses the threshold upward: record as a newly salient view.
								group->newly_salient_views.insert(v->second);
						}else													// sync on state.
							group->newly_salient_views.insert(v->second);
					}

					//	inject sln propagation jobs.
					//	the idea is to propagate sln changes when a view "occurs to the mind", i.e. becomes more salient in a group and is eligible for reduction in that group.
					//		- when a view is now salient because the group becomes c-salient, no propagation;
					//		- when a view is now salient because the group's sln_thr gets lower, no propagation;
					//		- propagation can occur only if the group is c_active. For efficiency reasons, no propagation occurs even if some of the group's viewing groups are c-active and c-salient.
					if(group_is_c_active)
						_initiate_sln_propagation(v->second->object,view_new_sln-view_old_sln,group->get_sln_thr());
				}

				if(v->second->object->code(0).getDescriptor()==Atom::GROUP	||	v->second->object->code(0).getDescriptor()==Atom::REDUCTION_GROUP){

					//	update visibility.
					bool	view_was_visible=v->second->get_vis()>group->get_vis_thr();
					bool	view_is_visible=v->second->update_vis()>group->get_vis_thr();
					bool	cov=v->second->get_cov();

					//	update viewing groups.
					if(group_was_c_active	&&	group_was_c_salient){

						if(!group_is_c_active	||	!group_is_c_salient)	//	group is not c-active and c-salient anymore: unregister as a viewing group.
							((Group	*)v->second->object)->viewing_groups.erase(group);
						else{	//	group remains c-active and c-salient.

							if(!view_was_visible){
								
								if(view_is_visible)		//	newly visible view.
									((Group	*)v->second->object)->viewing_groups[group]=cov;
							}else{
								
								if(!view_is_visible)	//	the view is no longer visible.
									((Group	*)v->second->object)->viewing_groups.erase(group);
								else					//	the view is still visible, cov might have changed.
									((Group	*)v->second->object)->viewing_groups[group]=cov;
							}
						}
					}else	if(group_is_c_active	&&	group_is_c_salient){	//	group becomes c-active and c-salient.

						if(view_is_visible)		//	update viewing groups for any visible group.
							((Group	*)v->second->object)->viewing_groups[group]=cov;
					}	
				}else	if(v->second->object->code(0).getDescriptor()==Atom::INSTANTIATED_PROGRAM		||
							v->second->object->code(0).getDescriptor()==Atom::INSTANTIATED_CPP_PROGRAM	||
							v->second->object->code(0).getDescriptor()==Atom::REDUCTION_GROUP){

					//	update activation.
					bool	view_was_active=v->second->get_act()>group->get_act_thr();
					bool	view_is_active=group->update_act(v->second,this)>group->get_act_thr();

					//	kill newly inactive controllers, register newly active ones.
					if(group_was_c_active	&&	group_was_c_salient){

						if(!group_is_c_active	||	!group_is_c_salient)	//	group is not c-active and c-salient anymore: kill the view's controller.
							v->second->controller->kill();
						else{	//	group remains c-active and c-salient.

							if(!view_was_active){
						
								if(view_is_active)	//	register the overlay for the newly active ipgm view.
									group->new_controllers.push_back(v->second->controller);
							}else{
								
								if(!view_is_active)	//	kill the newly inactive ipgm view's overlays.
									v->second->controller->kill();
							}
						}
					}else	if(group_is_c_active	&&	group_is_c_salient){	//	group becomes c-active and c-salient.

						if(view_is_active)	//	register the overlay for any active ipgm view.
							group->new_controllers.push_back(v->second->controller);
					}
				}
				++v;
			}else{	//	view has no resilience.

				v->second->object->acq_views();
				v->second->object->views.erase(v->second);	//	delete view from object's views.
				if(v->second->object->views.size()==0)
					v->second->object->invalidate();
				v->second->object->rel_views();

				//	delete the view.
				if(v->second->isNotification())
					v=group->notification_views.erase(v);
				else	switch(GetType(v->second->object)){
				case	ObjectType::IPGM:
					v=group->ipgm_views.erase(v);
					break;
				case	ObjectType::ANTI_IPGM:
					v=group->anti_ipgm_views.erase(v);
					break;
				case	ObjectType::INPUT_LESS_IPGM:
					v=group->input_less_ipgm_views.erase(v);
					break;
				case	ObjectType::ICPP_PGM:
					v=group->icpp_pgm_views.erase(v);
					break;
				case	ObjectType::OBJECT:
				case	ObjectType::MARKER:
					v=group->other_views.erase(v);
					break;
				case	ObjectType::GROUP:
					v=group->group_views.erase(v);
					break;
				case	ObjectType::RGROUP:
					v=group->rgroup_views.erase(v);
					break;
				}
			}
		FOR_ALL_VIEWS_END

			if(group_is_c_salient	&&	group->code(0).getDescriptor()!=Atom::REDUCTION_GROUP){	//	rgrp don't have a cov member.

			//	cov, i.e. injecting now newly salient views in the viewing groups from which the group is visible and has cov.
			//	reduction jobs will be added at each of the eligible viewing groups' own update time.
			UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
			for(vg=group->viewing_groups.begin();vg!=group->viewing_groups.end();++vg){

				if(vg->second){	//	cov==true.

					std::set<View	*,r_code::View::Less>::const_iterator	v;
					for(v=group->newly_salient_views.begin();v!=group->newly_salient_views.end();++v){	//	no cov for pgm, groups or notifications.

						if((*v)->isNotification())
							continue;
						switch((*v)->object->code(0).getDescriptor()){
						case	Atom::INSTANTIATED_PROGRAM:
						case	Atom::INSTANTIATED_CPP_PROGRAM:
						case	Atom::GROUP:
						case	Atom::REDUCTION_GROUP:
							break;
						default:
							injectCopyNow(*v,vg->first,now);	//	no need to protect group->newly_salient_views[i] since the support values for the ctrl values are not even read.
							break;
						}
					}
				}
			}

			//	build reduction jobs.
			std::set<View	*,r_code::View::Less>::const_iterator	v;
			for(v=group->newly_salient_views.begin();v!=group->newly_salient_views.end();++v)
				_inject_reduction_jobs(*v,group);
		}

		if(group_is_c_active	&&	group_is_c_salient){	//	build signaling jobs for new ipgms.

			for(uint32	i=0;i<group->new_controllers.size();++i){

				switch(GetType(group->new_controllers[i]->getObject())){
				case	ObjectType::ANTI_IPGM:{	//	inject signaling jobs for |ipgm (tsc).

					P<TimeJob>	j=new	AntiPGMSignalingJob((AntiPGMController	*)group->new_controllers[i],now+Utils::GetTimestamp<Code>(group->new_controllers[i]->getObject(),IPGM_TSC));
					time_job_queue->push(j);
					break;
				}case	ObjectType::INPUT_LESS_IPGM:{	//	inject a signaling job for an input-less pgm.

					P<TimeJob>	j=new	InputLessPGMSignalingJob((InputLessPGMController	*)group->new_controllers[i],now+Utils::GetTimestamp<Code>(group->new_controllers[i]->getObject(),IPGM_TSC));
					time_job_queue->push(j);
					break;
				}
				}
			}

			group->new_controllers.clear();
		}

		group->update_stats(this);	//	triggers notifications.

		//	inject the next update job for the group.
		if(group->get_upr()>0){

			P<TimeJob>	j=new	UpdateJob(group,now+group->get_upr()*base_period);
			time_job_queue->push(j);
		}

		group->leave();
	}

	void	_Mem::_inject_reduction_jobs(View	*view,Group	*host,Controller	*origin){	//	host is assumed to be c-salient; host already protected.

		if(host->get_c_act()>host->get_c_act_thr()){	//	host is c-active.

			//	build reduction jobs from host's own inputs and own overlays.
			FOR_ALL_VIEWS_WITH_INPUTS_BEGIN(host,v)

				if(v->second->get_act()>host->get_act_thr())		//	active ipgm/icpp_pgm/rgrp view.
					v->second->controller->take_input(view,origin);	//	view will be copied.

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
					v->second->controller->take_input(view,origin);	//	view will be copied.
			
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

			if(GetType(object)==ObjectType::MARKER){	//	if marker, propagate to references.

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

			if(GetType(object)==ObjectType::MARKER)	//	if marker, propagate to references.
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

		if(object==root)
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
}