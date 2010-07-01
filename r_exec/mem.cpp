//	mem.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

		if(state==STARTED)
			stop();
		root=NULL;
	}

	void	_Mem::init(uint32	base_period,	//	in us; same for upr, spr and res.
						uint32	reduction_core_count,
						uint32	time_core_count){

		this->base_period=base_period;
		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;
	}

	void	_Mem::reset(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		std::list<DelegatedCore	*>::const_iterator	d;
		for(d=delegates.begin();d!=delegates.end();++d)
			delete	*d;

		delegates.clear();

		delete	object_register_sem;
		delete	objects_sem;
		delete	state_sem;

		delete	reduction_job_queue;
		delete	time_job_queue;
	}

	_Mem::State	_Mem::get_state(){

		state_sem->acquire();
		State	s=state;
		state_sem->release();

		return	s;
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::add_delegate(uint64	dealine,_TimeJob	*j){

		state_sem->acquire();
		if(state==STARTED){

			DelegatedCore	*d=new	DelegatedCore(this,dealine,j);
			d->position=delegates.insert(delegates.end(),d);
			d->start(DelegatedCore::Wait);
		}
		state_sem->release();
	}

	void	_Mem::remove_delegate(DelegatedCore	*core){

		state_sem->acquire();
		delegates.erase(core->position);
		delete	core;
		state_sem->release();
	}

	void	_Mem::start(){

		if(state==STARTED)
			return;

		object_register_sem=new	FastSemaphore(1,1);
		objects_sem=new	FastSemaphore(1,1);
		state_sem=new	FastSemaphore(1,1);

		time_job_queue=new	PipeNN<TimeJob,1024>();
		reduction_job_queue=new	PipeNN<ReductionJob,1024>();

		uint32	i;
		uint64	now=Now();
		for(i=0;i<initial_groups.size();++i){

			Group	*g=initial_groups[i];
			bool	c_active=g->get_c_act()>g->get_c_act_thr();
			bool	c_salient=g->get_c_sln()>g->get_c_sln_thr();

			FOR_ALL_VIEWS_BEGIN(g,v)
				r_code::Timestamp::Set<View>(v->second,VIEW_IJT,now);	//	init injection time for the view.
			FOR_ALL_VIEWS_END

			if(c_active){

				UNORDERED_MAP<uint32,P<View> >::const_iterator	v;

				//	build signaling jobs for active input-less overlays.
				for(v=g->input_less_ipgm_views.begin();v!=g->input_less_ipgm_views.end();++v){

					TimeJob	j(new	InputLessPGMSignalingJob(v->second->controller),now+g->get_spr()*base_period);
					time_job_queue->push(j);
				}

				//	build signaling jobs for active anti-pgm overlays.
				for(v=g->anti_ipgm_views.begin();v!=g->anti_ipgm_views.end();++v){

					TimeJob	j(new	AntiPGMSignalingJob(v->second->controller),now+Timestamp::Get<Code>(v->second->controller->getIPGM()->get_reference(0),PGM_TSC));
					time_job_queue->push(j);
				}

				if(c_salient){

					//	build reduction jobs for each salient view and each active overlay.
					FOR_ALL_VIEWS_BEGIN(g,v)
						
						if(v->second->get_sln()>g->get_sln_thr()){	//	salient view.

							g->newly_salient_views.push_back(v->second);
							_inject_reduction_jobs(v->second,g);
						}
					FOR_ALL_VIEWS_END
				}
			}

			//	inject the next update job for the group.
			TimeJob	j(new	UpdateJob(g),now+g->get_upr()*base_period);
			time_job_queue->push(j);
		}

		initial_groups.clear();

		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->start(ReductionCore::Run);
		for(i=0;i<time_core_count;++i)
			time_cores[i]->start(TimeCore::Run);

		state=STARTED;
	}

	void	_Mem::stop(){

		state_sem->acquire();
		if(state!=STARTED){

			state_sem->release();
			return;
		}
		state=STOPPED;

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			Thread::TerminateAndWait(reduction_cores[i]);
		for(i=0;i<time_core_count;++i)
			Thread::TerminateAndWait(time_cores[i]);

		std::list<DelegatedCore	*>::const_iterator	d;
		for(d=delegates.begin();d!=delegates.end();++d)
			Thread::TerminateAndWait(*d);

		state_sem->release();
		reset();
	}

	////////////////////////////////////////////////////////////////

	ReductionJob	_Mem::popReductionJob(){

		return	reduction_job_queue->pop();
	}

	void	_Mem::pushReductionJob(ReductionJob	j){

		reduction_job_queue->push(j);
	}

	TimeJob	_Mem::popTimeJob(){

		return	time_job_queue->pop();
	}

	void	_Mem::pushTimeJob(TimeJob	j){

		time_job_queue->push(j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::update(UpdateJob	*j){

		update((Group	*)j->group);
		j->group=NULL;
	}

	void	_Mem::update(AntiPGMSignalingJob	*j){

		if(j->controller->is_alive())
			j->controller->signal_anti_pgm();
		j->controller=NULL;
	}

	void	_Mem::update(InputLessPGMSignalingJob	*j){

		if(j->controller->is_alive())
			j->controller->signal_input_less_pgm();
		j->controller=NULL;
	}

	void	_Mem::update(InjectionJob	*j){

		injectNow(j->view);
		j->view=NULL;
	}

	void	_Mem::update(SaliencyPropagationJob	*j){
		
		propagate_sln(j->object,j->sln_change,j->source_sln_thr);
		j->object=NULL;
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(Code	*object,uint16	nodeID){
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::injectCopyNow(View	*view,Group	*destination,uint64	now){

		View	*copied_view=new	View(view,destination);	//	ctrl values are morphed.
		r_code::Timestamp::Set<View>(copied_view,VIEW_IJT,now);
		_inject_existing_object_now(copied_view,view->object,destination,true);
	}

	void	_Mem::_inject_existing_object_now(View	*view,Code	*object,Group	*host,bool	lock){

		view->set_object(object);	//	the object already exists (content-wise): have the view point to the existing one.

		if(lock)
			host->acquire();

		object->acq_views();
		View	*existing_view=(View	*)object->find_view(host,false);
		if(!existing_view){	//	no existing view: add the view to the group and to the object's view_map.

			switch(GetType(object)){
			case	ObjectType::IPGM:
				host->ipgm_views[view->getOID()]=view;
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
		}else{	//	call set on the ctrl values of the existing view with the new view's ctrl values. NB: org left unchanged.

			object->rel_views();

			host->pending_operations.push_back(new	Group::Set(existing_view->getOID(),VIEW_RES,view->get_res()));
			host->pending_operations.push_back(new	Group::Set(existing_view->getOID(),VIEW_SLN,view->get_sln()));
			switch(GetType(object)){
			case	ObjectType::IPGM:
			case	ObjectType::ANTI_IPGM:
			case	ObjectType::INPUT_LESS_IPGM:
				host->pending_operations.push_back(new	Group::Set(existing_view->getOID(),IPGM_VIEW_ACT,view->get_act_vis()));
				break;
			}
		}

		if(lock)
			host->release();
	}

	void	_Mem::update(Group	*group){		
			
		uint64	now=Now();

		group->acquire();

		if(group!=root	&&	group->views.size()==0){

			group->invalidate();
			group->release();
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
			v->second->mod_res(-1);
			float32	res=group->update_res(v->second,this);
			if(res>0){

				//	update saliency (apply decay).
				float32	view_old_sln=v->second->get_sln();
				bool	wiew_was_salient=view_old_sln>former_sln_thr;
				float32	view_new_sln=group->update_sln(v->second,this);
				bool	wiew_is_salient=view_new_sln>group->get_sln_thr();

				if(group_is_c_salient){
					
					if(!wiew_was_salient	&&	wiew_is_salient)	//	record as a newly salient view.
						group->newly_salient_views.push_back(v->second);

					//	inject sln propagation jobs.
					//	the idea is to propagate sln changes when a view "occurs to the mind", i.e. becomes more salient in a group and is eligible for reduction in that group.
					//		- when a view is now salient because the group becomes c-salient, no propagation;
					//		- when a view is now salient because the group's sln_thr gets lower, no propagation;
					//		- propagation can occur only if the group is c_active. For efficiency reasons, no propagation occurs even if some of the group's viewing groups are c-active and c-salient.
					if(group_is_c_active)
						_initiate_sln_propagation(v->second->object,view_new_sln-view_old_sln,group->get_sln_thr());
				}

				if(v->second->object->code(0).getDescriptor()==Atom::GROUP){

					//	update visibility.
					bool	view_was_visible=v->second->get_act_vis()>group->get_vis_thr();
					bool	view_is_visible=v->second->update_vis()>group->get_vis_thr();
					bool	cov=v->second->get_cov()==0?false:true;

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
				}else	if(v->second->object->code(0).getDescriptor()==Atom::INSTANTIATED_PROGRAM){

					//	update activation
					bool	view_was_active=v->second->get_act_vis()>group->get_act_thr();
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

				if(v->second->object->code(0).getDescriptor()==Atom::INSTANTIATED_PROGRAM)	//	if ipgm view, kill the overlay.
					v->second->controller->kill();

				v->second->object->acq_views();
				if(v->second->object->views.size()>1)
					v->second->object->views.erase(v->second);	//	delete view from object's views.
				else
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
				case	ObjectType::OBJECT:
				case	ObjectType::MARKER:
					v=group->other_views.erase(v);
					break;
				case	ObjectType::GROUP:
					v=group->group_views.erase(v);
					break;
				}
			}
		FOR_ALL_VIEWS_END

		if(group_is_c_salient){

			//	cov, i.e. injecting now newly salient views in the viewing groups for which group is visible and has cov.
			//	reduction jobs will be added at each of the eligible viewing groups' own update time.
			UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
			for(vg=group->viewing_groups.begin();vg!=group->viewing_groups.end();++vg){

				if(vg->second)	//	cov==true.
					for(uint32	i=0;i<group->newly_salient_views.size();++i)
						if(group->newly_salient_views[i]->object->code(0).getDescriptor()!=Atom::INSTANTIATED_PROGRAM	&&	//	no cov for pgm, groups or notifications.
							group->newly_salient_views[i]->object->code(0).getDescriptor()!=Atom::GROUP					&&
							!group->newly_salient_views[i]->isNotification())
								injectCopyNow(group->newly_salient_views[i],vg->first,now);	//	no need to protect group->newly_salient_views[i] since the support values for the ctrl values are not even read.
			}

			//	build reduction jobs.
			for(uint32	i=0;i<group->newly_salient_views.size();++i)
				_inject_reduction_jobs(group->newly_salient_views[i],group);
		}

		if(group_is_c_active	&&	group_is_c_salient){	//	build signaling jobs for new ipgms.

			for(uint32	i=0;i<group->new_controllers.size();++i){

				switch(GetType(group->new_controllers[i]->getIPGM())){
				case	ObjectType::ANTI_IPGM:{	//	inject signaling jobs for |ipgm (tsc).

					TimeJob	j(new	AntiPGMSignalingJob(group->new_controllers[i]),now+Timestamp::Get<Code>(group->new_controllers[i]->getIPGM()->get_reference(0),PGM_TSC));
					time_job_queue->push(j);
					break;
				}case	ObjectType::INPUT_LESS_IPGM:{	//	inject a signaling job for an input-less pgm (sfr).

					TimeJob	j(new	InputLessPGMSignalingJob(group->new_controllers[i]),now+group->get_spr()*base_period);
					time_job_queue->push(j);
					break;
				}
				}
			}

			group->new_controllers.clear();
		}

		group->update_stats(this);	//	triggers notifications.

		//	inject the next update job for the group.
		TimeJob	j(new	UpdateJob(group),now+group->get_upr()*base_period);
		time_job_queue->push(j);

		group->release();
	}

	void	_Mem::_inject_reduction_jobs(View	*view,Group	*host){	//	host is assumed to be c-salient; host already protected.

		if(host->get_c_act()>host->get_c_act_thr()){	//	host is c-active.

			//	build reduction jobs from host's own inputs and own overlays.
			FOR_ALL_IPGM_VIEWS_WITH_INPUTS_BEGIN(host,v)

				if(v->second->get_act_vis()>host->get_sln_thr())	//	active ipgm view.
					v->second->controller->take_input(v->second);	//	view will be copied.

			FOR_ALL_IPGM_VIEWS_WITH_INPUTS_END
		}

		//	build reduction jobs from host's own inputs and overlays from viewing groups, if no cov and view is not a notification.
		//	NB: visibility is not transitive;
		//	no shadowing: if a view alresady exists in the viewing group, there will be twice the reductions: all of the identicals will be trimmed down at injection time.
		UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
		for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

			if(vg->second	||	view->isNotification())	//	cov==true or notification.
				continue;

			FOR_ALL_IPGM_VIEWS_WITH_INPUTS_BEGIN(vg->first,v)

				if(v->second->get_act_vis()>vg->first->get_sln_thr())	//	active ipgm view.
					v->second->controller->take_input(v->second);			//	view will be copied.
			
			FOR_ALL_IPGM_VIEWS_WITH_INPUTS_END
		}
	}

	void	_Mem::propagate_sln(Code	*object,float32	change,float32	source_sln_thr){

		//	apply morphed change to views.
		//	loops are prevented within one call, but not accross several upr:
		//		- feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop spreding accross several upr.
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

		//	prevent loops.
		for(uint32	i=0;i<path.size();++i)
			if(path[i]==object)
				return;
		path.push_back(object);

		TimeJob	j(new	SaliencyPropagationJob(object,change,source_sln_thr),0);
		time_job_queue->push(j);
		
		_initiate_sln_propagation(object,change,source_sln_thr,path);
	}
}