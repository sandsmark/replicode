#include	"../r_code/utils.h"


namespace	r_exec{

	inline	View::View():r_code::View(){

		code(VIEW_OID)=GetOID();

		reset_ctrl_values();
	}

	inline	View::View(r_code::SysView	*source,r_code::Object	*object):r_code::View(source,object){

		code(VIEW_OID)=GetOID();

		reset_ctrl_values();
		reset_init_values();
	}

	inline	View::View(View	*view):r_code::View(){

		object=view->object;
		memcpy(_code,view->_code,VIEW_CODE_MAX_SIZE*sizeof(Atom)+2*sizeof(Object	*));	//	reference_set is contiguous to code; memcpy in one go.

		reset_ctrl_values();
		reset_init_values();
	}

	inline	View::~View(){
	}

	inline	uint32	View::getOID()	const{

		return	code(VIEW_OID).asFloat();
	}

	inline	bool	View::isNotification()	const{

		return	false;
	}

	inline	Group	*View::getHost(){

		uint32	host_reference=code(VIEW_HOST).asIndex();
		return	(Group	*)references[host_reference];
	}

	inline	uint64	View::get_ijt()	const{

		return	Timestamp::Get(_code+VIEW_IJT);
	}

	inline	float32	View::get_res(){

		return	code(VIEW_RES).asFloat();
	}

	inline	float32	View::get_sln(){

		return	code(VIEW_SLN).asFloat();
	}

	inline	float32	View::get_act_vis(){

		return	code(VIEW_ACT_VIS).asFloat();
	}

	inline	bool	View::get_cov(){

		return	code(GRP_VIEW_COV).asFloat()==1;
	}

	inline	void	View::mod_res(float32	value){

		acc_res+=value;
		++res_changes;
	}

	inline	void	View::set_res(float32	value){

		acc_res+=value-get_res();
		++res_changes;
	}

	inline	void	View::mod_sln(float32	value){

		acc_sln+=value;
		++sln_changes;
	}

	inline	void	View::set_sln(float32	value){

		acc_res+=value-get_sln();
		++sln_changes;
	}

	inline	void	View::mod_act_vis(float32	value){

		acc_act_vis+=value;
		++act_vis_changes;
	}

	inline	void	View::set_act_vis(float32	value){

		acc_act_vis+=value-get_act_vis();
		++act_vis_changes;
	}

	inline	float32	View::update_sln_delta(){

		float32	delta=get_sln()-initial_sln;
		initial_sln=get_sln();
		return	delta;
	}

	inline	float32	View::update_act_delta(){

		float32	delta=get_act_vis()-initial_act;
		initial_act=get_act_vis();
		return	delta;
	}

	inline	void	View::mod(uint16	member_index,float32	value){

		switch(member_index){
		case	VIEW_SLN:
			mod_sln(value);
			break;
		case	VIEW_RES:
			mod_res(value);
			break;
		case	VIEW_ACT_VIS:
			mod_act_vis(value);
			break;
		}
	}

	inline	void	View::set(uint16	member_index,float32	value){

		switch(member_index){
		case	VIEW_SLN:
			set_sln(value);
			break;
		case	VIEW_RES:
			set_res(value);
			break;
		case	VIEW_ACT_VIS:
			set_act_vis(value);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	bool	NotificationView::isNotification()	const{

		return	false;
	}
}