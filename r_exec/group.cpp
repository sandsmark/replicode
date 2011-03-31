//	group.cpp
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

#include	"group.h"
#include	"factory.h"
#include	"mem.h"
#include	"pgm_controller.h"
#include	"cst_controller.h"
#include	"mdl_controller.h"
#include	<math.h>


namespace	r_exec{

	View	*Group::get_view(uint32	OID){

		UNORDERED_MAP<uint32,P<View> >::const_iterator	it=other_views.find(OID);
		if(it!=other_views.end())
			return	it->second;
		it=group_views.find(OID);
		if(it!=group_views.end())
			return	it->second;
		it=ipgm_views.find(OID);
		if(it!=ipgm_views.end())
			return	it->second;
		it=anti_ipgm_views.find(OID);
		if(it!=anti_ipgm_views.end())
			return	it->second;
		it=input_less_ipgm_views.find(OID);
		if(it!=input_less_ipgm_views.end())
			return	it->second;
		it=notification_views.find(OID);
		if(it!=notification_views.end())
			return	it->second;
		return	NULL;
	}

	void	Group::reset_ctrl_values(){

		sln_thr_changes=0;
		acc_sln_thr=0;
		act_thr_changes=0;
		acc_act_thr=0;
		vis_thr_changes=0;
		acc_vis_thr=0;
		c_sln_changes=0;
		acc_c_sln=0;
		c_act_changes=0;
		acc_c_act=0;
		c_sln_thr_changes=0;
		acc_c_sln_thr=0;
		c_act_thr_changes=0;
		acc_c_act_thr=0;
	}

	void	Group::reset_stats(){

		avg_sln=0;
		high_sln=0;
		low_sln=1;
		avg_act=0;
		high_act=0;
		low_act=1;

		sln_updates=0;
		act_updates=0;

		if(sln_change_monitoring_periods_to_go<=0){	//	0:reactivate, -1: activate.

			if(get_sln_chg_thr()<1)			//	activate monitoring.
				sln_change_monitoring_periods_to_go=get_sln_chg_prd();
		}else	if(get_sln_chg_thr()==1)	//	deactivate monitoring.
			sln_change_monitoring_periods_to_go=-1;
		else
			--sln_change_monitoring_periods_to_go;	//	notification will occur when =0.

		if(act_change_monitoring_periods_to_go<=0){	//	0:reactivate, -1: activate.

			if(get_act_chg_thr()<1)			//	activate monitoring.
				act_change_monitoring_periods_to_go=get_act_chg_prd();
		}else	if(get_act_chg_thr()==1)	//	deactivate monitoring.
			act_change_monitoring_periods_to_go=-1;
		else
			--act_change_monitoring_periods_to_go;	//	notification will occur when =0.
	}

	void	Group::update_stats(){

		if(decay_periods_to_go>0)
			--decay_periods_to_go;
		
		//	auto restart: iff dcy_auto==1.
		if(code(GRP_DCY_AUTO).asFloat()){

			float32	period=code(GRP_DCY_PRD).asFloat();
			if(decay_periods_to_go<=0	&&	period>0)
				decay_periods_to_go=period;
		}

		if(sln_updates)
			avg_sln=avg_sln/(float32)sln_updates;
		if(act_updates)
			avg_act=avg_act/(float32)act_updates;

		code(GRP_AVG_SLN)=Atom::Float(avg_sln);
		code(GRP_AVG_ACT)=Atom::Float(avg_act);
		code(GRP_HIGH_SLN)=Atom::Float(high_sln);
		code(GRP_LOW_SLN)=Atom::Float(low_sln);
		code(GRP_HIGH_ACT)=Atom::Float(high_act);
		code(GRP_LOW_ACT)=Atom::Float(low_act);

		if(sln_change_monitoring_periods_to_go==0){

			FOR_ALL_NON_NTF_VIEWS_BEGIN(this,v)

				float32	change=v->second->update_sln_delta();
				if(fabs(change)>get_sln_chg_thr()){

					uint16	ntf_grp_count=get_ntf_grp_count();
					for(uint16	i=1;i<=ntf_grp_count;++i)
						_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkSlnChg(_Mem::Get(),v->second->object,change)),false);
				}

			FOR_ALL_NON_NTF_VIEWS_END
		}

		if(act_change_monitoring_periods_to_go==0){

			FOR_ALL_NON_NTF_VIEWS_BEGIN(this,v)

				float32	change=v->second->update_act_delta();
				if(fabs(change)>get_act_chg_thr()){

					uint16	ntf_grp_count=get_ntf_grp_count();
					for(uint16	i=1;i<=ntf_grp_count;++i)
						_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkActChg(_Mem::Get(),v->second->object,change)),false);
				}

			FOR_ALL_NON_NTF_VIEWS_END
		}
	}

	void	Group::reset_decay_values(){
		
		sln_thr_decay=0;
		sln_decay=0;
		decay_periods_to_go=-1;
		decay_percentage_per_period=0;
		decay_target=-1;
	}

	float32	Group::update_sln_thr(){

		float32	percentage=code(GRP_DCY_PER).asFloat();
		float32	period=code(GRP_DCY_PRD).asFloat();
		if(percentage==0	||	period==0)
			reset_decay_values();
		else{

			float32	percentage_per_period=percentage/period;
			if(percentage_per_period!=decay_percentage_per_period	||	code(GRP_DCY_TGT).asFloat()!=decay_target){	//	recompute decay.

				decay_periods_to_go=period;
				decay_percentage_per_period=percentage_per_period;
				decay_target=code(GRP_DCY_TGT).asFloat();

				if(code(GRP_DCY_TGT).asFloat()==0){

					sln_thr_decay=0;
					sln_decay=percentage_per_period;
				}else{

					sln_decay=0;
					sln_thr_decay=percentage_per_period;
				}
			}
		}

		if(decay_periods_to_go>0){
			
			if(sln_thr_decay!=0)
				mod_sln_thr(get_sln_thr()*sln_thr_decay);
		}
		
		if(sln_thr_changes){

			float32	new_sln_thr=get_sln_thr()+acc_sln_thr/sln_thr_changes;
			if(new_sln_thr<0)
				new_sln_thr=0;
			else	if(new_sln_thr>1)
				new_sln_thr=1;
			code(GRP_SLN_THR)=r_code::Atom::Float(new_sln_thr);
		}
		acc_sln_thr=0;
		sln_thr_changes=0;
		return	get_sln_thr();
	}

	float32	Group::update_act_thr(){

		if(act_thr_changes){

			float32	new_act_thr=get_act_thr()+acc_act_thr/act_thr_changes;
			if(new_act_thr<0)
				new_act_thr=0;
			else	if(new_act_thr>1)
				new_act_thr=1;
			code(GRP_ACT_THR)=r_code::Atom::Float(new_act_thr);
		}
		acc_act_thr=0;
		act_thr_changes=0;
		return	get_act_thr();
	}

	float32	Group::update_vis_thr(){

		if(vis_thr_changes){

			float32	new_vis_thr=get_vis_thr()+acc_vis_thr/vis_thr_changes;
			if(new_vis_thr<0)
				new_vis_thr=0;
			else	if(new_vis_thr>1)
				new_vis_thr=1;
			code(GRP_VIS_THR)=r_code::Atom::Float(new_vis_thr);
		}
		acc_vis_thr=0;
		vis_thr_changes=0;
		return	get_vis_thr();
	}

	float32	Group::update_c_sln(){

		if(c_sln_changes){

			float32	new_c_sln=get_c_sln()+acc_c_sln/c_sln_changes;
			if(new_c_sln<0)
				new_c_sln=0;
			else	if(new_c_sln>1)
				new_c_sln=1;
			code(GRP_C_SLN)=r_code::Atom::Float(new_c_sln);
		}
		acc_c_sln=0;
		c_sln_changes=0;
		return	get_c_sln();
	}

	float32	Group::update_c_act(){

		if(c_act_changes){

			float32	new_c_act=get_c_act()+acc_c_act/c_act_changes;
			if(new_c_act<0)
				new_c_act=0;
			else	if(new_c_act>1)
				new_c_act=1;
			code(GRP_C_ACT)=r_code::Atom::Float(new_c_act);
		}
		acc_c_act=0;
		c_act_changes=0;
		return	get_c_act();
	}

	float32	Group::update_c_sln_thr(){

		if(c_sln_thr_changes){

			float32	new_c_sln_thr=get_c_sln_thr()+acc_c_sln_thr/c_sln_thr_changes;
			if(new_c_sln_thr<0)
				new_c_sln_thr=0;
			else	if(new_c_sln_thr>1)
				new_c_sln_thr=1;
			code(GRP_C_SLN_THR)=r_code::Atom::Float(new_c_sln_thr);
		}
		acc_c_sln_thr=0;
		c_sln_thr_changes=0;
		return	get_c_sln_thr();
	}

	float32	Group::update_c_act_thr(){

		if(c_act_thr_changes){

			float32	new_c_act_thr=get_c_act_thr()+acc_c_act_thr/c_act_thr_changes;
			if(new_c_act_thr<0)
				new_c_act_thr=0;
			else	if(new_c_act_thr>1)
				new_c_act_thr=1;
			code(GRP_C_ACT_THR)=r_code::Atom::Float(new_c_act_thr);
		}
		acc_c_act_thr=0;
		c_act_thr_changes=0;
		return	get_c_act_thr();
	}

	float32	Group::update_res(View	*v){

		float	res=v->update_res();
		if(!v->isNotification()	&&	res>0	&&	res<get_low_res_thr()){

			uint16	ntf_grp_count=get_ntf_grp_count();
			for(uint16	i=1;i<=ntf_grp_count;++i)
				_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkLowRes(_Mem::Get(),v->object)),false);
		}
		return	res;
	}

	float32	Group::update_sln(View	*v){

		if(decay_periods_to_go>0	&&	sln_decay!=0)
			v->mod_sln(v->get_sln()*sln_decay);

		float32	sln=v->update_sln(get_low_sln_thr(),get_high_sln_thr());
		avg_sln+=sln;
		if(sln>high_sln)
			high_sln=sln;
		else	if(sln<low_sln)
			low_sln=sln;
		++sln_updates;
		if(!v->isNotification()){

			if(v->periods_at_high_sln==get_sln_ntf_prd()){

				v->periods_at_high_sln=0;
				uint16	ntf_grp_count=get_ntf_grp_count();
				for(uint16	i=1;i<=ntf_grp_count;++i)
					_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkHighSln(_Mem::Get(),v->object)),false);
			}else	if(v->periods_at_low_sln==get_sln_ntf_prd()){

				v->periods_at_low_sln=0;
				uint16	ntf_grp_count=get_ntf_grp_count();
				for(uint16	i=1;i<=ntf_grp_count;++i)
					_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkLowSln(_Mem::Get(),v->object)),false);
			}
		}
		return	sln;
	}

	float32	Group::update_act(View	*v){

		float32	act=v->update_act(get_low_act_thr(),get_high_act_thr());
		avg_act+=act;
		if(act>high_act)
			high_act=act;
		else	if(act<low_act)
			low_act=act;
		++act_updates;
		if(!v->isNotification()){

			if(v->periods_at_high_act==get_act_ntf_prd()){

				v->periods_at_high_act=0;
				uint16	ntf_grp_count=get_ntf_grp_count();
				for(uint16	i=1;i<=ntf_grp_count;++i)
					_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkHighAct(_Mem::Get(),v->object)),false);
			}else	if(v->periods_at_low_act==get_act_ntf_prd()){

				v->periods_at_low_act=0;
				uint16	ntf_grp_count=get_ntf_grp_count();
				for(uint16	i=1;i<=ntf_grp_count;++i)
					_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkLowAct(_Mem::Get(),v->object)),false);
			}
		}
		return	act;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Group::load(View	*view,Code	*object){

		switch(object->code(0).getDescriptor()){
		case	Atom::GROUP:{
			group_views[view->getOID()]=view;

			//	init viewing_group.
			bool	viewing_c_active=get_c_act()>get_c_act_thr();
			bool	viewing_c_salient=get_c_sln()>get_c_sln_thr();
			bool	viewed_visible=view->get_vis()>get_vis_thr();
			if(viewing_c_active	&&	viewing_c_salient	&&	viewed_visible)	//	visible group in a c-salient, c-active group.
				((Group	*)object)->viewing_groups[this]=view->get_cov();	//	init the group's viewing groups.
			break;
		}case	Atom::INSTANTIATED_PROGRAM:
			ipgm_views[view->getOID()]=view;
			if(view->get_act()>get_act_thr()){	//	active ipgm.

				PGMController	*o=new	PGMController(view);	//	now will be added to the deadline at start time.
				view->controller=o;
			}
			break;
		case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
			input_less_ipgm_views[view->getOID()]=view;
			if(view->get_act()>get_act_thr()){	//	active ipgm.

				InputLessPGMController	*o=new	InputLessPGMController(view);	//	now will be added to the deadline at start time.
				view->controller=o;
			}
			break;
		case	Atom::INSTANTIATED_ANTI_PROGRAM:
			anti_ipgm_views[view->getOID()]=view;
			if(view->get_act()>get_act_thr()){	//	active ipgm.

				AntiPGMController	*o=new	AntiPGMController(view);	//	now will be added to the deadline at start time.
				view->controller=o;
			}
			break;
		case	Atom::INSTANTIATED_CPP_PROGRAM:
			ipgm_views[view->getOID()]=view;
			if(view->get_act()>get_act_thr()){	//	active icpp_pgm.

				Controller	*o=CPPPrograms::New(Utils::GetString<Code>(view->object,ICPP_PGM_NAME),view);	//	now will be added to the deadline at start time.
				if(!o)
					return	false;
				view->controller=o;
			}
			break;
		case	Atom::COMPOSITE_STATE:
			ipgm_views[view->getOID()]=view;
			if(view->get_act()>get_act_thr()){	//	active cst.

				CSTController	*o=new	CSTController(view);	//	now will be added to the deadline at start time.
				view->controller=o;
			}
			break;
		case	Atom::MODEL:
			ipgm_views[view->getOID()]=view;
			if(view->get_act()>get_act_thr()){	//	active mdl.

				MDLController	*o=new	MDLController(view);	//	now will be added to the deadline at start time.
				view->controller=o;
			}
			break;
		case	Atom::MARKER:	//	populate the marker set of the referenced objects.
			for(uint32	i=0;i<object->references_size();++i)
				object->get_reference(i)->markers.push_back(object);
		case	Atom::OBJECT:
			other_views[view->getOID()]=view;
			break;
		}

		return	true;
	}

	void	Group::inject(View	*view,uint64	t){	//	the view can hold anything but groups, r-groups and notifications.

		switch(view->object->code(0).getDescriptor()){
		case	Atom::INSTANTIATED_PROGRAM:{
			ipgm_views[view->getOID()]=view;
			PGMController	*o=new	PGMController(view);
			view->controller=o;
			if(view->get_act()>get_act_thr()	&&	get_c_sln()>get_c_sln_thr()	&&	get_c_act()>get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
				for(v=newly_salient_views.begin();v!=newly_salient_views.end();++v)
					o->take_input(*v);	//	view will be copied.
			}
			break;
		}case	Atom::INSTANTIATED_CPP_PROGRAM:{
			ipgm_views[view->getOID()]=view;
			Controller	*o=CPPPrograms::New(Utils::GetString<Code>(view->object,ICPP_PGM_NAME),view);
			if(!o)
				break;
			view->controller=o;
			if(view->get_act()>get_act_thr()	&&	get_c_sln()>get_c_sln_thr()	&&	get_c_act()>get_c_act_thr()){	//	active icpp_pgm in a c-salient and c-active group.

				std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
				for(v=newly_salient_views.begin();v!=newly_salient_views.end();++v)
					o->take_input(*v);	//	view will be copied.
			}
			break;
		}case	Atom::INSTANTIATED_ANTI_PROGRAM:{
			anti_ipgm_views[view->getOID()]=view;
			AntiPGMController	*o=new	AntiPGMController(view);
			view->controller=o;
			if(view->get_act()>get_act_thr()	&&	get_c_sln()>get_c_sln_thr()	&&	get_c_act()>get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
				for(v=newly_salient_views.begin();v!=newly_salient_views.end();++v)
					o->take_input(*v);	//	view will be copied.

				_Mem::Get()->pushTimeJob(new	AntiPGMSignalingJob(o,t+Utils::GetTimestamp<Code>(o->getObject(),IPGM_TSC)));
			}
			break;
		}case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:{
			input_less_ipgm_views[view->getOID()]=view;
			InputLessPGMController	*o=new	InputLessPGMController(view);
			view->controller=o;
			if(view->get_act()>get_act_thr()	&&	get_c_sln()>get_c_sln_thr()	&&	get_c_act()>get_c_act_thr())	//	active ipgm in a c-salient and c-active group.
				_Mem::Get()->pushTimeJob(new	InputLessPGMSignalingJob(o,t+Utils::GetTimestamp<Code>(view->object,IPGM_TSC)));
			break;

		}case	Atom::MARKER:	//	the marker does not exist yet: add it to the mks of its references.
			for(uint32	i=0;i<view->object->references_size();++i){

				Code	*ref=view->object->get_reference(i);
				ref->acq_markers();
				ref->markers.push_back(view->object);
				ref->rel_markers();
			}
		case	Atom::OBJECT:
			other_views[view->getOID()]=view;
			cov(view,t);
			break;
		case	Atom::COMPOSITE_STATE:{
			ipgm_views[view->getOID()]=view;
			CSTController	*o=new	CSTController(view);
			view->controller=o;
			if(view->get_act()>get_act_thr()	&&	get_c_sln()>get_c_sln_thr()	&&	get_c_act()>get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
				for(v=newly_salient_views.begin();v!=newly_salient_views.end();++v)
					o->take_input(*v);	//	view will be copied.
			}
			break;
		}case	Atom::MODEL:{
			ipgm_views[view->getOID()]=view;
			MDLController	*o=new	MDLController(view);
			view->controller=o;
			if(view->get_act()>get_act_thr()	&&	get_c_sln()>get_c_sln_thr()	&&	get_c_act()>get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

				o->gain_activation();
				std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
				for(v=newly_salient_views.begin();v!=newly_salient_views.end();++v)
					o->take_input(*v);	//	view will be copied.
			}
			break;
		}
		}

		if(get_c_sln()>get_c_sln_thr()	&&	view->get_sln()>get_sln_thr()){	//	group is c-salient and view is salient.

			newly_salient_views.insert(view);
			_Mem::Get()->inject_reduction_jobs(view,this);
		}

		notifyNew(view);
	}

	void	Group::injectGroup(View	*view,uint64	t){	//	the view can hold a group or an r-group.

		group_views[view->getOID()]=view;

		if(get_c_sln()>get_c_sln_thr()	&&	view->get_sln()>get_sln_thr()){	//	group is c-salient and view is salient.

			if(view->get_vis()>get_vis_thr())	//	new visible group in a c-active and c-salient host.
				((Group	*)view->object)->viewing_groups[this]=view->get_cov();

			_Mem::Get()->inject_reduction_jobs(view,this);
		}

		//	inject the next update job for the group.
		if(((Group	*)view->object)->get_upr()>0)
			_Mem::Get()->pushTimeJob(new	UpdateJob((Group	*)view->object,t+((Group	*)view->object)->get_upr()*_Mem::Get()->get_base_period()));

		notifyNew(view);
	}

	void	Group::inject_notification(View	*view){

		notification_views[view->getOID()]=view;
		for(uint32	i=0;i<view->object->references_size();++i){

			Code	*ref=view->object->get_reference(i);
			ref->acq_markers();
			ref->markers.push_back(view->object);
			ref->rel_markers();
		}
				
		if(get_c_sln()>get_c_sln_thr()	&&	view->get_sln()>get_sln_thr())	//	group is c-salient and view is salient.
			_Mem::Get()->inject_reduction_jobs(view,this);
	}

	void	Group::notifyNew(View	*view){

		if(get_ntf_new()==1){

			uint16	ntf_grp_count=get_ntf_grp_count();
			for(uint16	i=1;i<=ntf_grp_count;++i)
				_Mem::Get()->inject_notification(new	NotificationView(this,get_ntf_grp(i),new	factory::MkNew(_Mem::Get(),view->object)),get_ntf_grp(i)!=this);	//	the object appears for the first time in the group: notify.
		}
	}

	void	Group::cov(View	*view,uint64	t){

		UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
		for(vg=viewing_groups.begin();vg!=viewing_groups.end();++vg){

			if(vg->second)	//	cov==true, vieiwing group c-salient and c-active (otherwise it wouldn't be a viewing group).
				_Mem::Get()->inject_copy(view,vg->first,t);
		}
	}

	void	Group::cov(uint64	t){

		//	cov, i.e. injecting now newly salient views in the viewing groups from which the group is visible and has cov.
		//	reduction jobs will be added at each of the eligible viewing groups' own update time.
		UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
		for(vg=viewing_groups.begin();vg!=viewing_groups.end();++vg){

			if(vg->second){	//	cov==true.

				std::multiset<P<View>,r_code::View::Less>::const_iterator	v;
				for(v=newly_salient_views.begin();v!=newly_salient_views.end();++v){	//	no cov for pgm (all sorts), groups, notifications.

					if((*v)->isNotification())
						continue;
					switch((*v)->object->code(0).getDescriptor()){
					case	Atom::GROUP:
					case	Atom::INSTANTIATED_PROGRAM:
					case	Atom::INSTANTIATED_CPP_PROGRAM:
					case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
					case	Atom::INSTANTIATED_ANTI_PROGRAM:
					case	Atom::COMPOSITE_STATE:
					case	Atom::MODEL:
						break;
					default:
						_Mem::Get()->inject_copy(*v,vg->first,t);	//	no need to protect group->newly_salient_views[i] since the support values for the ctrl values are not even read.
						break;
					}
				}
			}
		}
	}

	void	Group::delete_view(View	*v){

		if(v->isNotification())
			notification_views.erase(v->getOID());
		else	switch(v->object->code(0).getDescriptor()){
		case	Atom::INSTANTIATED_PROGRAM:
		case	Atom::INSTANTIATED_CPP_PROGRAM:
		case	Atom::COMPOSITE_STATE:
		case	Atom::MODEL:
			ipgm_views.erase(v->getOID());
			break;
		case	Atom::INSTANTIATED_ANTI_PROGRAM:
			anti_ipgm_views.erase(v->getOID());
			break;
		case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
			input_less_ipgm_views.erase(v->getOID());
			break;
		case	Atom::OBJECT:
		case	Atom::MARKER:
			other_views.erase(v->getOID());
			break;
		case	Atom::GROUP:
			group_views.erase(v->getOID());
			break;
		}
	}

}