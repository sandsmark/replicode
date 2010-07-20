//	mem.tpl.cpp
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

#include	"init.h"
#include	"replicode_defs.h"
#include	"operator.h"
#include	"group.h"
#include	"factory.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	template<class	O>	Mem<O>::Mem(){
	}

	template<class	O>	Mem<O>::~Mem(){
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	r_code::Code	*Mem<O>::buildObject(r_code::SysObject	*source){

		switch(source->code[0].getDescriptor()){
		case	Atom::GROUP:
			return	new	Group(source,this);
		default:
			return	new	O(source,this);
		}
	}

	template<class	O>	r_code::Code	*Mem<O>::buildObject(Atom	head){

		switch(head.getDescriptor()){
		case	Atom::GROUP:
			return	new	Group();
		default:
			if(O::RequiresPacking())
				return	new	r_code::LObject();	//	temporary sand box for assembling code; will be packed into an O at injection time.
			else
				return	new	O();
		}
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	void	Mem<O>::deleteObject(Code	*object){

		if(!IsNotification(object)	&&	GetType(object)!=GROUP){

			object_registerCS.enter();
			object_register.erase(((O	*)object)->position_in_object_register);
			object_registerCS.leave();
		}

		objectsCS.enter();
		objects.erase(object->position_in_objects);
		objectsCS.leave();
	}

	template<class	O>	Group	*Mem<O>::get_stdin()	const{

		return	_stdin;
	}
	
	template<class	O>	Group	*Mem<O>::get_stdout()	const{

		return	_stdout;
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	void	Mem<O>::load(std::vector<r_code::Code	*>	*objects){	//	NB: no cov at init time.

		uint32	i;
		reduction_cores=new	ReductionCore	*[reduction_core_count];
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]=new	ReductionCore(this);
		time_cores=new	TimeCore	*[time_core_count];
		for(i=0;i<time_core_count;++i)
			time_cores[i]=new	TimeCore(this);

		//	load root (always comes first).
		root=(Group	*)(*objects)[0];
		this->objects.push_back(root);
		initial_groups.push_back(root);

		for(uint32	i=1;i<objects->size();++i){	//	skip root as it has no initial views.

			Code	*object=(*objects)[i];
			switch(object->get_axiom()){
			case	SysObject::STDIN_GRP:
				_stdin=(Group	*)(*objects)[i];
				break;
			case	SysObject::STDOUT_GRP:
				_stdout=(Group	*)(*objects)[i];
				break;
			case	SysObject::SELF_ENT:
				_self=(O	*)(*objects)[i];
				break;
			}

			UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	it;
			for(it=object->views.begin();it!=object->views.end();++it){

				//	init hosts' member_set.
				View	*view=(r_exec::View	*)*it;
				view->set_object(object);
				Group	*host=view->get_host();

				switch(GetType(object)){
				case	ObjectType::GROUP:{
					
					host->group_views[view->getOID()]=view;

					//	init viewing_group.
					bool	viewing_c_active=host->get_c_act()>host->get_c_act_thr();
					bool	viewing_c_salient=host->get_c_sln()>host->get_c_sln_thr();
					bool	viewed_visible=view->get_act_vis()>host->get_vis_thr();
					if(viewing_c_active	&&	viewing_c_salient	&&	viewed_visible)	//	visible group in a c-salient, c-active group.
						((Group	*)object)->viewing_groups[host]=view->get_cov()==0?false:true;	//	init the group's viewing groups.
					break;
				}case	ObjectType::IPGM:
					host->ipgm_views[view->getOID()]=view;
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						PGMController	*o=new	PGMController(this,view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
					break;
				case	ObjectType::INPUT_LESS_IPGM:
					host->input_less_ipgm_views[view->getOID()]=view;
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						InputLessPGMController	*o=new	InputLessPGMController(this,view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
					break;
				case	ObjectType::ANTI_IPGM:
					host->anti_ipgm_views[view->getOID()]=view;
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						AntiPGMController	*o=new	AntiPGMController(this,view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
					break;
				case	ObjectType::MARKER:	//	populate the marker set of the referenced objects.
					for(uint32	i=0;i<object->references_size();++i)
						object->get_reference(i)->markers.push_back(object);
				case	ObjectType::OBJECT:
					host->other_views[view->getOID()]=view;
					break;
				}
			}

			object->position_in_objects=this->objects.insert(this->objects.end(),object);
			if(GetType(object)!=ObjectType::GROUP)	//	load non-group object in regsister.
				((O	*)object)->position_in_object_register=object_register.insert((O	*)object).first;
			else
				initial_groups.push_back((Group	*)object);	//	convenience to create initial update jobs - see start().
		}
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	r_comp::Image	*Mem<O>::getImage(){

		r_comp::Image	*image=new	r_comp::Image();
		std::list<Code	*>::const_iterator	i;
		for(i=objects.begin();i!=objects.end();++i)
			image->operator	<<(*i);
		return	image;
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	void	Mem<O>::inject(O	*object,View	*view){

		view->set_object(object);
		injectNow(view);
	}

	template<class	O>	Code	*Mem<O>::inject(View	*view){

		Group	*host=view->get_host();

		host->enter();
		if(host->is_invalidated()){

			host->leave();
			return	NULL;
		}
		host->leave();

		uint64	now=Now();
		uint64	ijt=view->get_ijt();

		if(view->object->code(0).getDescriptor()!=Atom::GROUP){

			O	*object;
			if(O::RequiresPacking())						//	false if LObject, true for network-aware objects.
				view->object=O::Pack(view->object,this);	//	non compact form will be deleted (P<> in view) if not an instance of O; compact forms are left unchanged.
			object=(O	*)view->object;

			object_registerCS.enter();
			UNORDERED_SET<O	*,typename	O::Hash,typename	O::Equal>::const_iterator	it=object_register.find(object);
			if(it!=object_register.end()){

				object_registerCS.leave();
				if(ijt<=now)
					injectExistingObjectNow(view,*it,host,true);
				else{
					
					TimeJob	j(new	EInjectionJob(view),ijt);
					time_job_queue->push(j);
				}
				return	*it;
			}
			object_registerCS.leave();
			if(ijt<=now)
				injectNow(view);
			else{
				
				TimeJob	j(new	InjectionJob(view),ijt);
				time_job_queue->push(j);
			}
			return	NULL;
		}else{

			if(ijt<=now)
				injectGroupNow(view,(Group	*)view->object,host);
			else{
				
				TimeJob	j(new	GInjectionJob(view,(Group	*)view->object,host),ijt);
				time_job_queue->push(j);
			}

			return	NULL;
		}
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	void	Mem<O>::injectNow(View	*view){

		Group	*host=view->get_host();

		O	*object=(O	*)view->object;	//	has been packed if necessary in inject(view).
		object->views.insert(view);	//	no need to protect object since it's new.
		uint64	now=Now();
		r_code::Timestamp::Set<View>(view,VIEW_IJT,now);

		object_registerCS.enter();
		object->position_in_object_register=object_register.insert(object).first;
		object_registerCS.leave();

		object->bind(this);
		objectsCS.enter();
		object->position_in_objects=objects.insert(objects.end(),object);
		objectsCS.leave();

		host->enter();

		switch(GetType(object)){
		case	ObjectType::IPGM:{

			host->ipgm_views[view->getOID()]=view;
			PGMController	*o=new	PGMController(this,view);
			view->controller=o;
			if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				for(uint32	i=0;i<host->newly_salient_views.size();++i)
					o->take_input(host->newly_salient_views[i]);	//	view will be copied.
			}
			break;
		}case	ObjectType::ANTI_IPGM:{
			host->anti_ipgm_views[view->getOID()]=view;
			AntiPGMController	*o=new	AntiPGMController(this,view);
			view->controller=o;
			if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				for(uint32	i=0;i<host->newly_salient_views.size();++i)
					o->take_input(host->newly_salient_views[i]);	//	view will be copied.

				TimeJob	j(new	AntiPGMSignalingJob(o),now+Timestamp::Get<Code>(o->getIPGM()->get_reference(0),PGM_TSC));
				time_job_queue->push(j);

			}
			break;
		}case	ObjectType::INPUT_LESS_IPGM:{
			host->input_less_ipgm_views[view->getOID()]=view;
			InputLessPGMController	*o=new	InputLessPGMController(this,view);
			view->controller=o;
			if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				TimeJob	j(new	InputLessPGMSignalingJob(o),now+Timestamp::Get<Code>(view->object->get_reference(0),PGM_TSC));
				time_job_queue->push(j);
			}
			break;
		}case	ObjectType::MARKER:	//	the marker does not exist yet: add it to the mks of its references.
			for(uint32	i=0;i<object->references_size();++i){

				object->get_reference(i)->acq_markers();
				object->get_reference(i)->markers.push_back(object);
				object->get_reference(i)->rel_markers();
			}
		case	ObjectType::OBJECT:{
			host->other_views[view->getOID()]=view;
			//	cov, i.e. injecting now newly salient views in the viewing groups for which group is visible and has cov.
			UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
			for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

				if(vg->second)	//	cov==true, vieiwing group c-salient and c-active (otherwise it wouldn't be a viewing group).
					injectCopyNow(view,vg->first,now);
			}
			break;
		}
		}

		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr())	//	host is c-salient and view is salient.
			_inject_reduction_jobs(view,host);

		if(host->get_ntf_new()==1)	//	the view cannot be a ntf view (would use injectNotificationNow instead).
			injectNotificationNow(new	NotificationView(host,host->get_ntf_grp(),new	factory::MkNew(this,object)),host->get_ntf_grp()!=host);	//	the object appears for the first time in the group: notify.

		host->leave();
	}

	template<class	O>	void	Mem<O>::injectGroupNow(View	*view,Group	*object,Group	*host){	//	groups are always new; no cov for groups; no need to protect object.

		object->bind(this);
		objectsCS.enter();
		object->position_in_objects=objects.insert(objects.end(),object);
		objectsCS.leave();

		host->enter();

		host->group_views[view->getOID()]=view;

		uint64	now=Now();
		r_code::Timestamp::Set<View>(view,VIEW_IJT,now);
		object->views.insert(view);

		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr()){	//	host is c-salient and view is salient.

			if(view->get_act_vis()>host->get_vis_thr())	//	new visible group in a c-active and c-salient host.
				host->viewing_groups[object]=view->get_cov()==0?false:true;

			_inject_reduction_jobs(view,host);
		}

		//	inject the next update job for the group.
		TimeJob	j(new	UpdateJob((Group	*)object),now+((Group	*)object)->get_upr()*base_period);
		time_job_queue->push(j);

		if(host->get_ntf_new()==1)
			injectNotificationNow(new	NotificationView(host,host->get_ntf_grp(),new	factory::MkNew(this,object)),false);	//	the group appears for the first time in the group: notify.

		host->leave();
	}

	template<class	O>	void	Mem<O>::injectNotificationNow(View	*view,bool	lock,_PGMController	*origin){	//	no notification for notifications; no registration either (object_register and object_io_map) and no cov.
																												//	notifications are ephemeral: they are not held by the marker sets of the object they refer to; this implies no propagation of saliency changes trough notifications.
		view->code(VIEW_RES)=Atom::Float(ntf_mk_res);

		Group	*host=view->get_host();
		LObject	*object=(LObject	*)view->object;

		object->bind(this);
		objectsCS.enter();
		object->position_in_objects=objects.insert(objects.end(),object);
		objectsCS.leave();
		
		if(lock)
			host->enter();

		r_code::Timestamp::Set<View>(view,VIEW_IJT,Now());
		host->notification_views[view->getOID()]=view;

		object->views.insert(view);
		
		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr())	//	host is c-salient and view is salient.
			_inject_reduction_jobs(view,host,origin);

		if(lock)
			host->leave();
	}
}