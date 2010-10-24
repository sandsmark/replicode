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
#include	<math.h>


namespace	r_exec{

	View	*Group::get_view(uint32	OID){

		UNORDERED_MAP<uint32,P<View> >::const_iterator	it=other_views.find(OID);
		if(it!=other_views.end())
			return	it->second;
		it=group_views.find(OID);
		if(it!=group_views.end())
			return	it->second;
		it=rgroup_views.find(OID);
		if(it!=rgroup_views.end())
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

	void	Group::update_stats(_Mem	*mem){

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
						mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkSlnChg(mem,v->second->object,change)),false);
				}

			FOR_ALL_NON_NTF_VIEWS_END
		}

		if(act_change_monitoring_periods_to_go==0){

			FOR_ALL_NON_NTF_VIEWS_BEGIN(this,v)

				float32	change=v->second->update_act_delta();
				if(fabs(change)>get_act_chg_thr()){

					uint16	ntf_grp_count=get_ntf_grp_count();
					for(uint16	i=1;i<=ntf_grp_count;++i)
						mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkActChg(mem,v->second->object,change)),false);
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

	float32	Group::update_res(View	*v,_Mem	*mem){

		float	res=v->update_res();
		if(!v->isNotification()	&&	res>0	&&	res<get_low_res_thr()){

			uint16	ntf_grp_count=get_ntf_grp_count();
			for(uint16	i=1;i<=ntf_grp_count;++i)
				mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkLowRes(mem,v->object)),false);
		}
		return	res;
	}

	float32	Group::update_sln(View	*v,_Mem	*mem){

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
					mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkHighSln(mem,v->object)),false);
			}else	if(v->periods_at_low_sln==get_sln_ntf_prd()){

				v->periods_at_low_sln=0;
				uint16	ntf_grp_count=get_ntf_grp_count();
				for(uint16	i=1;i<=ntf_grp_count;++i)
					mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkLowSln(mem,v->object)),false);
			}
		}
		return	sln;
	}

	float32	Group::update_act(View	*v,_Mem	*mem){

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
					mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkHighAct(mem,v->object)),false);
			}else	if(v->periods_at_low_act==get_act_ntf_prd()){

				v->periods_at_low_act=0;
				uint16	ntf_grp_count=get_ntf_grp_count();
				for(uint16	i=1;i<=ntf_grp_count;++i)
					mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(i),new	factory::MkLowAct(mem,v->object)),false);
			}
		}
		return	act;
	}
}