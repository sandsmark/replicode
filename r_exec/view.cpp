#include	"view.h"
#include	"../CoreLibrary/utils.h"
#include	"group.h"


namespace	r_exec{

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

		Group	*source=view->getHost();
		object=view->object;
		memcpy(_code,view->_code,VIEW_CODE_MAX_SIZE*sizeof(Atom));
		code(VIEW_OID)=GetOID();
		references[0]=group;		//	host.
		references[1]=source;	//	origin.

		//	morph ctrl values; NB: res is not morphed as it is expressed as a multiple of the upr.
		code(VIEW_SLN)=MorphValue(view->code(VIEW_SLN).asFloat(),source->get_sln_thr(),group->get_sln_thr());
		ObjectType	t=((Object	*)object)->getType();
		switch(t){
		case	GROUP:
			code(VIEW_ACT_VIS)=MorphValue(view->code(VIEW_ACT_VIS).asFloat(),source->get_vis_thr(),group->get_vis_thr());
		case	IPGM:
		case	ANTI_IPGM:
		case	INPUT_LESS_IPGM:
			code(VIEW_ACT_VIS)=MorphValue(view->code(VIEW_ACT_VIS).asFloat(),source->get_act_thr(),group->get_act_thr());
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
			code(VIEW_RES)=new_res;
		}
		acc_res=0;
		res_changes=0;
		return	get_res();
	}

	float32	View::update_sln(float32	&change,float32	low,float32	high){

		if(sln_changes){

			change=-code(VIEW_SLN).asFloat();
			float32	new_sln=acc_sln/sln_changes;
			if(new_sln<0)
				new_sln=0;
			else	if(new_sln>1)
				new_sln=1;
			code(VIEW_SLN)=r_code::Atom::Float(new_sln);
			change+=code(VIEW_SLN).asFloat();
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
			code(VIEW_ACT_VIS)=r_code::Atom::Float(new_act_vis);
		}
		acc_act_vis=0;
		act_vis_changes=0;
		return	get_act_vis();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	NotificationView::NotificationView(Group	*origin,Group	*destination,Object	*marker):View(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::SSet(ViewOpcode,6);	//	Structured Set.
		code(write_index++)=GetOID();							//	oid
		code(write_index++)=r_code::Atom::IPointer(7);			//	iptr to ijt.
		code(write_index++)=r_code::Atom::Float(1);				//	res.
		code(write_index++)=r_code::Atom::Float(1);				//	sln.
		code(write_index++)=r_code::Atom::RPointer(0);			//	destination.
		code(write_index++)=r_code::Atom::RPointer(1);			//	origin.
		code(7)=r_code::Atom::Timestamp();						//	ijt will be set at injection time.
		references[0]=destination;
		references[1]=origin;
		reset_init_values();

		object=marker;
	}
}