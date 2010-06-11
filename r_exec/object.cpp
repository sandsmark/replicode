#include	"object.h"
#include	"replicode_defs.h"
#include	"../r_code/utils.h"
#include	"mem.h"
#include	<math.h>


namespace	r_exec{

	uint16	Object::GroupOpcode;

	uint16	Object::PTNOpcode;
	uint16	Object::AntiPTNOpcode;

	uint16	Object::IPGMOpcode;
	uint16	Object::PGMOpcode;
	uint16	Object::AntiPGMOpcode;

	uint16	Object::IGoalOpcode;
	uint16	Object::GoalOpcode;
	uint16	Object::AntiGoalOpcode;

	uint16	Object::MkRdx;
	uint16	Object::MkAntiRdx;

	uint16	Object::MkNewOpcode;

	uint16	Object::MkLowResOpcode;
	uint16	Object::MkLowSlnOpcode;
	uint16	Object::MkHighSlnOpcode;
	uint16	Object::MkLowActOpcode;
	uint16	Object::MkHighActOpcode;
	uint16	Object::MkSlnChgOpcode;
	uint16	Object::MkActChgOpcode;

	uint16	Object::InjectOpcode;
	uint16	Object::EjectOpcode;
	uint16	Object::ModOpcode;
	uint16	Object::SetOpcode;
	uint16	Object::NewClassOpcode;
	uint16	Object::DelClassOpcode;
	uint16	Object::LDCOpcode;
	uint16	Object::SwapOpcode;
	uint16	Object::NewDevOpcode;
	uint16	Object::DelDevOpcode;
	uint16	Object::SuspendOpcode;
	uint16	Object::ResumeOpcode;
	uint16	Object::StopOpcode;

	ObjectType	Object::getType()	const{

		return	OTHER;
	}

	bool	Object::isIPGM()	const{

		return	false;
	}

	void	Object::init(uint32	OID){

		this->OID=OID;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	FastSemaphore	OID_sem(1,1);

	uint32	View::LastOID=0;

	uint32	View::GetOID(){

		OID_sem.acquire();
		uint32	oid=LastOID++;
		OID_sem.release();
		return	oid;
	}

	uint16	View::ViewOpcode;

	float32	View::MorphValue(float32	value,float32	source_thr,float32	destination_thr){

		if(source_thr>0)
			return	value*destination_thr/source_thr;
		if(value==0)	//	i.e. value==source_thr.
			return	destination_thr;
		return	destination_thr+value;
	}

	float32	View::MorphChange(float32	change,float32	source_thr,float32	destination_thr){

		if(source_thr>0)
			return	change*destination_thr/source_thr;
		return	destination_thr+change;
	}

	View::View(View	*view,Group	*group):r_code::View(){

		OID=GetOID();

		Group	*source=view->getHost();
		object=view->object;
		memcpy(code,view->code,VIEW_CODE_MAX_SIZE*sizeof(Atom));
		reference_set[0]=group;		//	host.
		reference_set[1]=source;	//	origin.

		//	morph ctrl values; NB: res is not morphed as it is expressed as a multiple of the upr.
		code[VIEW_SLN]=MorphValue(view->code[VIEW_SLN].asFloat(),source->get_sln_thr(),group->get_sln_thr());
		ObjectType	t=((Object	*)object)->getType();
		switch(t){
		case	GROUP:
			code[VIEW_ACT_VIS]=MorphValue(view->code[VIEW_ACT_VIS].asFloat(),source->get_vis_thr(),group->get_vis_thr());
		case	IPGM:
		case	ANTI_IPGM:
		case	INPUT_LESS_IPGM:
			code[VIEW_ACT_VIS]=MorphValue(view->code[VIEW_ACT_VIS].asFloat(),source->get_act_thr(),group->get_act_thr());
		}

		reset_ctrl_values();
		reset_init_values();
	}

	void	View::reset_ctrl_values(){

		sln_changes=0;
		acc_sln=0;
		act_vis_changes=0;
		acc_act_vis=0;
		res_changes=0;
		acc_res=0;

		periods_at_low_sln=0;
		periods_at_high_sln=0;
		periods_at_low_act=0;
		periods_at_high_act=0;
	}

	void	View::reset_init_values(){

		initial_sln=get_sln();
		if(((Object	*)object)->isIPGM())
			initial_act=get_act_vis();
		else
			initial_act=0;
	}

	float32	View::update_res(){

		if(res_changes){

			float32	new_res=(float32)acc_res/(float32)res_changes;
			if(new_res<0)
				new_res=0;
			r_code::Timestamp::Set(&code[VIEW_RES],new_res);
		}
		acc_res=0;
		res_changes=0;
		return	get_res();
	}

	float32	View::update_sln(float32	&change,float32	low,float32	high){

		if(sln_changes){

			change=-code[VIEW_SLN].asFloat();
			float32	new_sln=acc_sln/sln_changes;
			if(new_sln<0)
				new_sln=0;
			else	if(new_sln>1)
				new_sln=1;
			code[VIEW_SLN]=r_code::Atom::Float(new_sln);
			change+=code[VIEW_SLN].asFloat();
		}else
			change=0;
		acc_sln=0;
		sln_changes=0;
		if(get_sln()<low)
			++periods_at_low_sln;
		else{
			
			periods_at_low_sln=0;
			if(get_sln()>high)
				++periods_at_high_sln;
			else
				periods_at_high_sln=0;
		}
		return	get_sln();
	}

	float32	View::update_act(float32	low,float32	high){

		update_vis();
		if(get_act_vis()<low)
			++periods_at_low_act;
		else{
			
			periods_at_low_act=0;
			if(get_act_vis()>high)
				++periods_at_high_act;
			else
				periods_at_high_act=0;
		}
		return	get_act_vis();
	}

	float32	View::update_vis(){

		if(act_vis_changes){

			float32	new_act_vis=acc_act_vis/act_vis_changes;
			if(new_act_vis<0)
				new_act_vis=0;
			else	if(new_act_vis>1)
				new_act_vis=1;
			code[VIEW_ACT_VIS]=r_code::Atom::Float(new_act_vis);
		}
		acc_act_vis=0;
		act_vis_changes=0;
		return	get_act_vis();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ObjectType	Group::getType()	const{

		return	GROUP;
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
		low_sln=0;
		avg_act=0;
		high_act=0;
		low_act=0;

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

	void	Group::update_stats(Mem	*mem){

		avg_sln=avg_sln/(float32)sln_updates;
		avg_act=avg_act/(float32)act_updates;

		if(sln_change_monitoring_periods_to_go==0){

			UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	v;
			for(v=non_ntf_views_begin();v!=non_ntf_views_end();v=next_non_ntf_view(v)){

				float32	change=(*v)->update_sln_delta();
				if(fabs(change)>get_sln_chg_thr())
					mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkSlnChg((*v)->object,change)));
			}
		}

		if(act_change_monitoring_periods_to_go==0){

			UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	v;
			for(v=non_ntf_views_begin();v!=non_ntf_views_end();v=next_non_ntf_view(v)){

				float32	change=(*v)->update_act_delta();
				if(fabs(change)>get_act_chg_thr())
					mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkActChg((*v)->object,change)));
			}
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

		if(code[GRP_DCY_PER].asFloat()==0)
			reset_decay_values();
		else{

			decay_periods_to_go=code[GRP_DCY_PRD].asFloat();
			float32	percentage_per_period=code[GRP_DCY_PER].asFloat()/(float32)decay_periods_to_go;

			if(percentage_per_period!=decay_percentage_per_period	||	code[GRP_DCY_TGT].asFloat()!=decay_target){	//	recompute decay.

				decay_percentage_per_period=percentage_per_period;

				if(code[GRP_DCY_TGT].asFloat()==0){

					sln_thr_decay=0;
					sln_decay=percentage_per_period;
				}else{

					sln_decay=0;
					sln_thr_decay=percentage_per_period;
				}
			}
		}

		if(decay_periods_to_go>0){

			--decay_periods_to_go;
			if(sln_thr_decay!=0)
				mod_sln_thr(get_sln_thr()*sln_thr_decay);
		}
		
		if(sln_thr_changes){

			float32	new_sln_thr=acc_sln_thr/sln_thr_changes;
			if(new_sln_thr<0)
				new_sln_thr=0;
			else	if(new_sln_thr>1)
				new_sln_thr=1;
			code[GRP_SLN_THR]=r_code::Atom::Float(new_sln_thr);
		}
		acc_sln_thr=0;
		sln_thr_changes=0;
		return	get_sln_thr();
	}

	float32	Group::update_act_thr(){

		if(act_thr_changes){

			float32	new_act_thr=acc_act_thr/act_thr_changes;
			if(new_act_thr<0)
				new_act_thr=0;
			else	if(new_act_thr>1)
				new_act_thr=1;
			code[GRP_ACT_THR]=r_code::Atom::Float(new_act_thr);
		}
		acc_act_thr=0;
		act_thr_changes=0;
		return	get_act_thr();
	}

	float32	Group::update_vis_thr(){

		if(vis_thr_changes){

			float32	new_vis_thr=acc_vis_thr/vis_thr_changes;
			if(new_vis_thr<0)
				new_vis_thr=0;
			else	if(new_vis_thr>1)
				new_vis_thr=1;
			code[GRP_VIS_THR]=r_code::Atom::Float(new_vis_thr);
		}
		acc_vis_thr=0;
		vis_thr_changes=0;
		return	get_vis_thr();
	}

	float32	Group::update_c_sln(){

		if(c_sln_changes){

			float32	new_c_sln=acc_c_sln/c_sln_changes;
			if(new_c_sln<0)
				new_c_sln=0;
			else	if(new_c_sln>1)
				new_c_sln=1;
			code[GRP_C_SLN]=r_code::Atom::Float(new_c_sln);
		}
		acc_c_sln=0;
		c_sln_changes=0;
		return	get_c_sln();
	}

	float32	Group::update_c_act(){

		if(c_act_changes){

			float32	new_c_act=acc_c_act/c_act_changes;
			if(new_c_act<0)
				new_c_act=0;
			else	if(new_c_act>1)
				new_c_act=1;
			code[GRP_C_ACT]=r_code::Atom::Float(new_c_act);
		}
		acc_c_act=0;
		c_act_changes=0;
		return	get_c_act();
	}

	float32	Group::update_c_sln_thr(){

		if(c_sln_thr_changes){

			float32	new_c_sln_thr=acc_c_sln_thr/c_sln_thr_changes;
			if(new_c_sln_thr<0)
				new_c_sln_thr=0;
			else	if(new_c_sln_thr>1)
				new_c_sln_thr=1;
			code[GRP_C_SLN_THR]=r_code::Atom::Float(new_c_sln_thr);
		}
		acc_c_sln_thr=0;
		c_sln_thr_changes=0;
		return	get_c_sln_thr();
	}

	float32	Group::update_c_act_thr(){

		if(c_act_thr_changes){

			float32	new_c_act_thr=acc_c_act_thr/c_act_thr_changes;
			if(new_c_act_thr<0)
				new_c_act_thr=0;
			else	if(new_c_act_thr>1)
				new_c_act_thr=1;
			code[GRP_C_ACT_THR]=r_code::Atom::Float(new_c_act_thr);
		}
		acc_c_act_thr=0;
		c_act_thr_changes=0;
		return	get_c_act_thr();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	InstantiatedProgram::InstantiatedProgram():Object(){
	}

	InstantiatedProgram::InstantiatedProgram(r_code::SysObject	*source,Mem	*m):Object(source,m){
	}

	InstantiatedProgram::~InstantiatedProgram(){
	}

	ObjectType	InstantiatedProgram::getType()	const{

		if(getPGM()->opcode()==AntiPGMOpcode)
			return	ANTI_IPGM;
		if((*getPGM()->code.as_std())[PGM_INPUTS].getAtomCount()>0)
			return	IPGM;
		return	INPUT_LESS_IPGM;
	}

	bool	InstantiatedProgram::isIPGM()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	NotificationView::NotificationView(Group	*origin,Group	*destination,Object	*marker):View(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::SSet(ViewOpcode,5);	//	Structured Set.
		code[write_index++]=r_code::Atom::IPointer(6);			//	iptr to ijt.
		code[write_index++]=r_code::Atom::Float(1);				//	res.
		code[write_index++]=r_code::Atom::Float(1);				//	sln.
		code[write_index++]=r_code::Atom::RPointer(0);			//	destination.
		code[write_index++]=r_code::Atom::RPointer(1);			//	origin.
		code[6]=r_code::Atom::Timestamp();						//	ijt will be set at injection time.
		reference_set[0]=destination;
		reference_set[1]=origin;
		reset_init_values();

		object=marker;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Marker::Marker():Object(){
	}

	Marker::Marker(r_code::SysObject	*source,Mem	*m):Object(source,m){
	}

	Marker::~Marker(){
	}

	ObjectType	Marker::getType()	const{

		return	MARKER;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkNew::MkNew(Object	*object):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkNewOpcode,5);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowRes::MkLowRes(Object	*object):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkLowResOpcode,5);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowSln::MkLowSln(Object	*object):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkLowSlnOpcode,5);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkHighSln::MkHighSln(Object	*object):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkHighSlnOpcode,5);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowAct::MkLowAct(Object	*object):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkLowActOpcode,5);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkHighAct::MkHighAct(Object	*object):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkHighActOpcode,5);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkSlnChg::MkSlnChg(Object	*object,float32	value):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkSlnChgOpcode,6);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::Float(value);	//	change.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkActChg::MkActChg(Object	*object,float32	value):Marker(){

		uint32	write_index=0;
		code[write_index++]=r_code::Atom::Marker(MkActChgOpcode,6);
		code[write_index++]=r_code::Atom::RPointer(0);	//	object.
		code[write_index++]=r_code::Atom::Float(value);	//	change.
		code[write_index++]=r_code::Atom::View();
		code[write_index++]=r_code::Atom::Mks();
		code[write_index++]=r_code::Atom::Vws();
		code[write_index++]=r_code::Atom::Float(1);		//	psln_thr.
		reference_set[0]=object;
	}
}