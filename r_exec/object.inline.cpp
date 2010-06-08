#include	"replicode_defs.h"
#include	"../r_code/utils.h"


namespace	r_exec{

	inline	Object::Object():r_code::Object(),mem(NULL),hash_value(0){

		psln_thr_sem=new	FastSemaphore(1,1);
		view_map_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	inline	Object::Object(r_code::SysObject	*source,Mem	*m):r_code::Object(source),mem(m),hash_value(0){

		computeHashValue();
		psln_thr_sem=new	FastSemaphore(1,1);
		view_map_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	inline	Object::~Object(){

		if(mem)
			mem->deleteObject(this);
		delete	psln_thr_sem;
		delete	view_map_sem;
		delete	marker_set_sem;
	}

	inline	void	Object::computeHashValue(){	//	assuming hash_value==0

		hash_value=opcode()<<22;							//	16 bits for the opcode;
		hash_value|=(code.size()	&	0x000003FF)<<6;		//	10 bits for the code size
		hash_value|=reference_set.size()	&	0x0000003F;	//	6 bits for the reference set size
	}

	inline	uint32	Object::getOID()	const{

		return	OID;
	}

	inline	float32	Object::get_psln_thr(){

		psln_thr_sem->acquire();
		float32	r=code[code.size()-1].asFloat();	//	psln is always the lat atom in code.
		psln_thr_sem->release();
		return	r;
	}

	inline	bool	Object::mod(uint16	member_index,float32	value){

		if(member_index!=code.size()-1)
			return	false;
		float32	v=code[member_index].asFloat()+value;
		if(v<0)
			v=0;
		else	if(v>1)
			v=1;

		psln_thr_sem->acquire();
		code[member_index]=Atom::Float(v);
		psln_thr_sem->release();
	}

	inline	bool	Object::set(uint16	member_index,float32	value){

		if(member_index!=code.size()-1)
			return	false;

		psln_thr_sem->acquire();
		code[member_index]=Atom::Float(value);
		psln_thr_sem->release();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	View::View():r_code::View(),FastSemaphore(1,1){

		reset_ctrl_values();
	}

	inline	View::View(r_code::SysView	*source,r_code::Object	*object):r_code::View(source,object),FastSemaphore(1,1){

		reset_ctrl_values();
		reset_init_values();
	}

	inline	View::View(View	*view):r_code::View(),FastSemaphore(1,1){

		object=view->object;
		memcpy(code,view->code,VIEW_CODE_MAX_SIZE*sizeof(Atom)+2*sizeof(Object	*));	//	reference_set is contiguous to code; memcpy in one go.

		reset_ctrl_values();
		reset_init_values();
	}

	inline	View::~View(){
	}

	inline	bool	View::isNotification()	const{

		return	false;
	}

	inline	Group	*View::getHost(){

		uint32	host_reference=code[VIEW_HOST].asIndex();
		return	(Group	*)reference_set[host_reference];
	}

	inline	uint64	View::get_ijt()	const{

		return	Timestamp::Get(code+VIEW_IJT);
	}

	inline	float32	View::get_res(){

		return	code[VIEW_RES].asFloat();
	}

	inline	float32	View::get_sln(){

		return	code[VIEW_SLN].asFloat();
	}

	inline	float32	View::get_act_vis(){

		return	code[VIEW_ACT_VIS].asFloat();
	}

	inline	bool	View::get_cov(){

		return	code[GRP_VIEW_COV].asFloat()==1;
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

	inline	bool	View::mod(uint16	member_index,float32	value){

		switch(member_index){
		case	VIEW_SLN:
			mod_sln(value);
			return	true;
		case	VIEW_RES:
			mod_res(value);
			return	true;
		case	VIEW_ACT_VIS:
			mod_act_vis(value);
			return	true;
		default:
			return	false;
		}
	}

	inline	bool	View::set(uint16	member_index,float32	value){

		switch(member_index){
		case	VIEW_SLN:
			set_sln(value);
			return	true;
		case	VIEW_RES:
			set_res(value);
			return	true;
		case	VIEW_ACT_VIS:
			set_act_vis(value);
			return	true;
		default:
			return	false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	Group::Group():Object(),FastSemaphore(1,1){

		reset_ctrl_values();
		reset_stats();
		reset_decay_values();
	}

	inline	Group::Group(r_code::SysObject	*source):Object(source,NULL),FastSemaphore(1,1){

		reset_ctrl_values();
		reset_stats();
		reset_decay_values();
	}

	inline	Group::~Group(){
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::views_begin()	const{

		return	ipgm_views.begin();
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::views_end()	const{

		return	other_views.end();
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::next_view(UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	&it)	const{

		if(it==ipgm_views.end())
			return	anti_ipgm_views.begin();
		if(it==anti_ipgm_views.end())
			return	input_less_ipgm_views.begin();
		if(it==input_less_ipgm_views.end())
			return	notification_views.begin();
		if(it==notification_views.end())
			return	group_views.begin();
		if(it==group_views.end())
			return	other_views.begin();
		return	++it;
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::input_less_ipgm_views_begin()	const{

		return	input_less_ipgm_views.begin();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::input_less_ipgm_views_end()		const{

		return	input_less_ipgm_views.end();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::next_input_less_ipgm_view(UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	&it)	const{

		return	++it;
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::anti_ipgm_views_begin()	const{

		return	anti_ipgm_views.begin();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::anti_ipgm_views_end()	const{

		return	anti_ipgm_views.end();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::next_anti_pgm_view(UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	&it)	const{

		return	++it;
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::ipgm_views_with_inputs_begin()	const{

		return	ipgm_views.begin();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::ipgm_views_with_inputs_end()	const{

		return	anti_ipgm_views.end();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::next_ipgm_view_with_inputs(UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	&it)	const{

		if(it==ipgm_views.end())
			return	anti_ipgm_views.begin();
		return	++it;
	}

	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::non_ntf_views_begin()	const{

		return	ipgm_views.begin();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::non_ntf_views_end()		const{

		return	input_less_ipgm_views.end();
	}
	
	inline	UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	Group::next_non_ntf_view(UNORDERED_SET<r_code::P<View>,View::Hash,View::Equal>::const_iterator	&it)	const{

		if(it==ipgm_views.end())
			return	anti_ipgm_views.begin();
		if(it==anti_ipgm_views.end())
			return	input_less_ipgm_views.begin();
		if(it==input_less_ipgm_views.end())
			return	group_views.begin();
		if(it==group_views.end())
			return	other_views.begin();
		return	++it;
	}

	inline	uint32	Group::get_upr(){

		return	code[GRP_UPR].asFloat();
	}

	inline	uint32	Group::get_spr(){

		return	code[GRP_SPR].asFloat();
	}

	inline	float32	Group::get_sln_thr(){

		return	code[GRP_SLN_THR].asFloat();
	}

	inline	float32	Group::get_act_thr(){

		return	code[GRP_ACT_THR].asFloat();
	}

	inline	float32	Group::get_vis_thr(){

		return	code[GRP_VIS_THR].asFloat();
	}

	inline	float32	Group::get_c_sln_thr(){

		return	code[GRP_C_SLN_THR].asFloat();
	}

	inline	float32	Group::get_c_act_thr(){

		return	code[GRP_C_ACT_THR].asFloat();
	}

	inline	float32	Group::get_c_sln(){

		return	code[GRP_C_SLN].asFloat();
	}

	inline	float32	Group::get_c_act(){

		return	code[GRP_C_ACT].asFloat();
	}

	inline	void	Group::mod_sln_thr(float32	value){

		++sln_thr_changes;
		acc_sln_thr+=value;
	}

	inline	void	Group::set_sln_thr(float32	value){

		++sln_thr_changes;
		acc_sln_thr+=value-get_sln_thr();
	}

	inline	void	Group::mod_act_thr(float32	value){

		++act_thr_changes;
		acc_act_thr+=value;
	}

	inline	void	Group::set_act_thr(float32	value){

		++act_thr_changes;
		acc_act_thr+=value-get_act_thr();
	}

	inline	void	Group::mod_vis_thr(float32	value){

		++vis_thr_changes;
		acc_vis_thr+=value;
	}

	inline	void	Group::set_vis_thr(float32	value){

		++vis_thr_changes;
		acc_vis_thr+=value-get_vis_thr();
	}

	inline	void	Group::mod_c_sln(float32	value){

		++c_sln_changes;
		acc_c_sln+=value;
	}

	inline	void	Group::set_c_sln(float32	value){

		++c_sln_changes;
		acc_c_sln+=value-get_c_sln();
	}

	inline	void	Group::mod_c_act(float32	value){

		++c_act_changes;
		acc_c_act+=value;
	}

	inline	void	Group::set_c_act(float32	value){

		++c_act_changes;
		acc_c_act+=value-get_c_act();
	}

	inline	void	Group::mod_c_sln_thr(float32	value){

		++c_sln_thr_changes;
		acc_c_sln_thr+=value;
	}

	inline	void	Group::set_c_sln_thr(float32	value){

		++c_sln_thr_changes;
		acc_c_sln_thr+=value-get_c_sln_thr();
	}

	inline	void	Group::mod_c_act_thr(float32	value){

		++c_act_thr_changes;
		acc_c_act_thr+=value;
	}

	inline	void	Group::set_c_act_thr(float32	value){

		++c_act_thr_changes;
		acc_c_act_thr+=value-get_c_act_thr();
	}

	inline	float32	Group::get_sln_chg_thr(){

		return	code[GRP_SLN_CHG_THR].asFloat();
	}

	inline	float32	Group::get_sln_chg_prd(){

		return	code[GRP_SLN_CHG_PRD].asFloat();
	}

	inline	float32	Group::get_act_chg_thr(){

		return	code[GRP_ACT_CHG_THR].asFloat();
	}

	inline	float32	Group::get_act_chg_prd(){

		return	code[GRP_ACT_CHG_PRD].asFloat();
	}

	inline	float32	Group::get_avg_sln(){

		return	code[GRP_AVG_SLN].asFloat();
	}

	inline	float32	Group::get_high_sln(){

		return	code[GRP_HIGH_SLN].asFloat();
	}

	inline	float32	Group::get_low_sln(){

		return	code[GRP_LOW_SLN].asFloat();
	}

	inline	float32	Group::get_avg_act(){

		return	code[GRP_AVG_ACT].asFloat();
	}

	inline	float32	Group::get_high_act(){

		return	code[GRP_HIGH_ACT].asFloat();
	}

	inline	float32	Group::get_low_act(){

		return	code[GRP_LOW_ACT].asFloat();
	}

	inline	float32	Group::get_high_sln_thr(){

		return	code[GRP_HIGH_SLN_THR].asFloat();
	}

	inline	float32	Group::get_low_sln_thr(){

		return	code[GRP_LOW_SLN_THR].asFloat();
	}

	inline	float32	Group::get_sln_ntf_prd(){

		return	code[GRP_SLN_NTF_PRD].asFloat();
	}

	inline	float32	Group::get_high_act_thr(){

		return	code[GRP_HIGH_ACT_THR].asFloat();
	}

	inline	float32	Group::get_low_act_thr(){

		return	code[GRP_LOW_ACT_THR].asFloat();
	}

	inline	float32	Group::get_act_ntf_prd(){

		return	code[GRP_ACT_NTF_PRD].asFloat();
	}

	inline	float32	Group::get_low_res_thr(){

		return	code[GRP_LOW_RES_THR].asFloat();
	}

	inline	float32	Group::get_res_ntf_prd(){

		return	code[GRP_RES_NTF_PRD].asFloat();
	}

	inline	float32	Group::get_ntf_new(){

		return	code[GRP_NTF_NEW].asFloat();
	}

	inline	Group	*Group::get_ntf_grp(){

		if(code[GRP_NTF_GRP].readsAsNil())
			return	this;

		uint32	index=code[GRP_NTF_GRP].asIndex();
		return	(Group	*)reference_set[index];
	}

	inline	float32	Group::update_res(View	*v,Mem	*mem){

		float	res=v->update_res();
		if(!v->isNotification()	&&	res<get_low_res_thr())
			mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkLowRes(v->object)));
		return	res;
	}

	inline	float32	Group::update_sln(View	*v,float32	&change,Mem	*mem){

		if(decay_periods_to_go>=0	&&	sln_decay!=0)
			v->mod_sln(v->get_sln()*sln_decay);

		float32	sln=v->update_sln(change,get_low_sln_thr(),get_high_sln_thr());
		avg_sln+=sln;
		++sln_updates;
		if(!v->isNotification()){

			if(v->periods_at_high_sln==get_sln_ntf_prd()){

				v->periods_at_high_sln=0;
				mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkHighSln(v->object)));
			}else	if(v->periods_at_low_sln==get_sln_ntf_prd()){

				v->periods_at_low_sln=0;
				mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkLowSln(v->object)));
			}
		}
		return	sln;
	}

	inline	float32	Group::update_act(View	*v,Mem	*mem){

		float32	act=v->update_act(get_low_act_thr(),get_high_act_thr());
		avg_act+=act;
		++act_updates;
		if(!v->isNotification()){

			if(v->periods_at_high_act==get_act_ntf_prd()){

				v->periods_at_high_act=0;
				mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkHighAct(v->object)));
			}else	if(v->periods_at_low_act==get_act_ntf_prd()){

				v->periods_at_low_act=0;
				mem->injectNotificationNow(new	NotificationView(this,get_ntf_grp(),new	MkLowAct(v->object)));
			}
		}
		return	act;
	}

	inline	void	Group::_mod_0_positive(uint16	member_index,float32	value){

		float32	v=code[member_index].asFloat()+value;
		if(v<0)
			v=0;
		code[member_index]=Atom::Float(v);
	}

	inline	void	Group::_mod_0_plus1(uint16	member_index,float32	value){

		float32	v=code[member_index].asFloat()+value;
		if(v<0)
			v=0;
		else	if(v>1)
			v=1;
		code[member_index]=Atom::Float(v);
	}

	inline	void	Group::_mod_minus1_plus1(uint16	member_index,float32	value){

		float32	v=code[member_index].asFloat()+value;
		if(v<-1)
			v=-1;
		else	if(v>1)
			v=1;
		code[member_index]=Atom::Float(v);
	}

	inline	void	Group::_set_0_positive(uint16	member_index,float32	value){

		if(value<0)
			code[member_index]=Atom::Float(0);
		else
			code[member_index]=Atom::Float(value);
	}

	inline	void	Group::_set_0_plus1(uint16	member_index,float32	value){

		if(value<0)
			code[member_index]=Atom::Float(0);
		else	if(value>1)
			code[member_index]=Atom::Float(1);
		else
			code[member_index]=Atom::Float(value);
	}

	inline	void	Group::_set_minus1_plus1(uint16	member_index,float32	value){

		if(value<-1)
			code[member_index]=Atom::Float(-1);
		else	if(value>1)
			code[member_index]=Atom::Float(1);
		else
			code[member_index]=Atom::Float(value);
	}

	inline	void	Group::_set_0_1(uint16	member_index,float32	value){

		if(value==0	||	value==1)
			code[member_index]=Atom::Float(value);
	}

	inline	bool	Group::mod(uint16	member_index,float32	value){

		switch(member_index){
		case	GRP_UPR:
		case	GRP_SPR:
		case	GRP_DCY_PRD:
		case	GRP_SLN_CHG_PRD:
		case	GRP_ACT_CHG_PRD:
		case	GRP_SLN_NTF_PRD:
		case	GRP_ACT_NTF_PRD:
		case	GRP_RES_NTF_PRD:
			_mod_0_positive(member_index,value);
			return	true;
		case	GRP_SLN_THR:
			mod_sln_thr(value);
			return	true;
		case	GRP_ACT_THR:
			mod_act_thr(value);
			return	true;
		case	GRP_VIS_THR:
			mod_vis_thr(value);
			return	true;
		case	GRP_C_SLN:
			mod_c_sln(value);
			return	true;
		case	GRP_C_SLN_THR:
			mod_c_sln_thr(value);
			return	true;
		case	GRP_C_ACT:
			mod_c_act(value);
			return	true;
		case	GRP_C_ACT_THR:
			mod_c_act_thr(value);
			return	true;
		case	GRP_DCY_PER:
			_mod_minus1_plus1(member_index,value);
			return	true;
		case	GRP_SLN_CHG_THR:
		case	GRP_ACT_CHG_THR:
		case	GRP_HIGH_SLN_THR:
		case	GRP_LOW_SLN_THR:
		case	GRP_HIGH_ACT_THR:
		case	GRP_LOW_ACT_THR:
		case	GRP_LOW_RES_THR:
			_mod_0_plus1(member_index,value);
			return	true;
		default:
			return	false;
		}
	}

	inline	bool	Group::set(uint16	member_index,float32	value){
		
		switch(member_index){
		case	GRP_UPR:
		case	GRP_SPR:
		case	GRP_DCY_PRD:
		case	GRP_SLN_CHG_PRD:
		case	GRP_ACT_CHG_PRD:
		case	GRP_SLN_NTF_PRD:
		case	GRP_ACT_NTF_PRD:
		case	GRP_RES_NTF_PRD:
			_set_0_positive(member_index,value);
			return	true;
		case	GRP_SLN_THR:
			set_sln_thr(value);
			return	true;
		case	GRP_ACT_THR:
			set_act_thr(value);
			return	true;
		case	GRP_VIS_THR:
			set_vis_thr(value);
			return	true;
		case	GRP_C_SLN:
			set_c_sln(value);
			return	true;
		case	GRP_C_SLN_THR:
			set_c_sln_thr(value);
			return	true;
		case	GRP_C_ACT:
			set_c_act(value);
			return	true;
		case	GRP_C_ACT_THR:
			set_c_act_thr(value);
			return	true;
		case	GRP_DCY_PER:
			_set_minus1_plus1(member_index,value);
			return	true;
		case	GRP_SLN_CHG_THR:
		case	GRP_ACT_CHG_THR:
		case	GRP_HIGH_SLN_THR:
		case	GRP_LOW_SLN_THR:
		case	GRP_HIGH_ACT_THR:
		case	GRP_LOW_ACT_THR:
		case	GRP_LOW_RES_THR:
			_set_0_plus1(member_index,value);
			return	true;
		case	GRP_NTF_NEW:
		case	GRP_DCY_TGT:
			_set_0_1(member_index,value);
			return	true;
		default:
			return	false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	Object	*InstantiatedProgram::getPGM()	const{

		uint32	index=(*code.as_std())[IPGM_PGM].asIndex();
		return	(*reference_set.as_std())[index];
	}

	inline	uint64	InstantiatedProgram::get_tsc()	const{

		return	r_code::Timestamp::Get(&getPGM()->code[PGM_TSC]);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	bool	NotificationView::isNotification()	const{

		return	false;
	}
}