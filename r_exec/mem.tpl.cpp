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
#include	"../r_code/replicode_defs.h"
#include	"operator.h"
#include	"r_group.h"
#include	"model.h"
#include	"factory.h"
#include	"cpp_programs.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	template<class	O>	Mem<O>::Mem():_Mem(){
	}

	template<class	O>	Mem<O>::~Mem(){
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	r_code::Code	*Mem<O>::buildObject(r_code::SysObject	*source){

		switch(source->code[0].getDescriptor()){
		case	Atom::GROUP:
			return	new	Group(source,this);
		case	Atom::REDUCTION_GROUP:
			return	new	RGroup(source,this);
		case	Atom::MODEL:
			return	new	Model(source,this);
		default:
			return	new	O(source,this);
		}
	}

	template<class	O>	r_code::Code	*Mem<O>::buildObject(Atom	head){

		r_code::Code	*object;
		switch(head.getDescriptor()){
		case	Atom::GROUP:
			object=new	Group();
			break;
		case	Atom::REDUCTION_GROUP:
			object=new	RGroup();
			break;
		case	Atom::MODEL:
			object=new	Model();
			break;
		default:
			if(O::RequiresPacking())
				object=new	r_code::LObject();	//	temporary sand box for assembling code; will be packed into an O at injection time.
			else
				object=new	O();
			break;
		}

		object->code(0)=head;
		return	object;
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

	template<class	O>	bool	Mem<O>::load(std::vector<r_code::Code	*>	*objects){	//	NB: no cov at init time.

		uint32	i;
		reduction_cores=new	ReductionCore	*[reduction_core_count];
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]=new	ReductionCore();
		time_cores=new	TimeCore	*[time_core_count];
		for(i=0;i<time_core_count;++i)
			time_cores[i]=new	TimeCore();

		last_oid=0;

		//	load root (always comes first).
		root=(Group	*)(*objects)[0];
		this->objects.push_back(root);
		initial_groups.push_back(root);
		root->setOID(get_oid());

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

			object->setOID(get_oid());

			UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	it;
			for(it=object->views.begin();it!=object->views.end();++it){

				//	init hosts' member_set.
				View	*view=(r_exec::View	*)*it;
				view->set_object(object);
				Group	*host=view->get_host();

				if(!host->load(view,object))
					return	false;
			}

			object->position_in_objects=this->objects.insert(this->objects.end(),object);
			object->is_registered=true;
			if(GetType(object)!=ObjectType::GROUP)	//	load non-group object in register.
				((O	*)object)->position_in_object_register=object_register.insert((O	*)object).first;
			else
				initial_groups.push_back((Group	*)object);	//	convenience to create initial update jobs - see start().
		}

		return	true;
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	r_comp::Image	*Mem<O>::getImage(){

		r_comp::Image	*image=new	r_comp::Image();
		image->timestamp=Now();
		std::list<Code	*>::const_iterator	i;
		for(i=objects.begin();i!=objects.end();++i)
			if(!(*i)->is_invalidated())
				image->operator	<<(*i);
		return	image;
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	Code	*Mem<O>::check_existence(Code	*object){

		switch(object->code(0).getDescriptor()){
		case	Atom::GROUP:
		case	Atom::REDUCTION_GROUP:	//	groups are always new.
			return	object;
		}

		O	*_object;
		if(O::RequiresPacking())			//	false if LObject, true for network-aware objects.
			_object=O::Pack(object,this);	//	non compact form will be deleted (P<> in view) if not an instance of O; compact forms are left unchanged.
		else
			_object=(O	*)object;

		object_registerCS.enter();
		UNORDERED_SET<O	*,typename	O::Hash,typename	O::Equal>::const_iterator	it=object_register.find(_object);
		if(it!=object_register.end()){

			object_registerCS.leave();
			return	*it;
		}
		object_registerCS.leave();
		return	_object;
	}

	template<class	O>	void	Mem<O>::inject(O	*object,View	*view){

		view->set_object(object);
		injectNow(view);
	}

	template<class	O>	void	Mem<O>::inject(View	*view){

		Group	*host=view->get_host();

		host->enter();
		if(host->is_invalidated())
			host->leave();
		host->leave();

		uint64	now=Now();
		uint64	ijt=view->get_ijt();

		if(view->object->code(0).getDescriptor()==Atom::GROUP	||	view->object->code(0).getDescriptor()==Atom::REDUCTION_GROUP){	//	group.

			if(ijt<=now)
				injectGroupNow(view,(Group	*)view->object,host);
			else{
				
				P<TimeJob>	j=new	GInjectionJob(view,(Group	*)view->object,host,ijt);
				time_job_queue->push(j);
			}
		}else	if(view->object->is_registered){	//	existing object.

			if(ijt<=now)
				injectExistingObjectNow(view,view->object,host,true);
			else{
				
				P<TimeJob>	j=new	EInjectionJob(view,ijt);
				time_job_queue->push(j);
			}
		}else{	//	new object.

			if(ijt<=now)
				injectNow(view);
			else{
				
				P<TimeJob>	j=new	InjectionJob(view,ijt);
				time_job_queue->push(j);
			}
		}
	}

	////////////////////////////////////////////////////////////////

	template<class	O>	void	Mem<O>::injectNow(View	*view){

		Group	*host=view->get_host();
		O	*object=(O	*)view->object;	//	has been packed if necessary in inject(view).

		uint64	now=Now();

		object_registerCS.enter();
		object->position_in_object_register=object_register.insert(object).first;
		object_registerCS.leave();

		bind<O>(view,now);

		host->enter();
		host->inject(view,now);
		host->leave();
	}

	template<class	O>	void	Mem<O>::injectGroupNow(View	*view,Group	*object,Group	*host){	//	groups are always new; no cov for groups; no need to protect object.

		uint64	now=Now();
		
		bind<Group>(view,now);

		host->enter();
		host->injectGroup(view,now);
		host->leave();
	}

	template<class	O>	void	Mem<O>::injectNotificationNow(View	*view,bool	lock,Controller	*origin){	//	no notification for notifications; no registration either (object_register and object_io_map) and no cov.
																											//	notifications are ephemeral: they are not held by the marker sets of the object they refer to; this implies no propagation of saliency changes trough notifications.
		Group	*host=view->get_host();
		LObject	*object=(LObject	*)view->object;

		uint64	now=Now();
		
		bind<LObject>(view,now);

		view->code(VIEW_RES)=Atom::Float(ntf_mk_res);
		
		if(lock)
			host->enter();
		host->injectNotification(view,origin);
		if(lock)
			host->leave();
	}
}