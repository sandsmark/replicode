namespace	r_exec{

	inline	Group::Group():LObject(),FastSemaphore(1,1){

		reset_ctrl_values();
		reset_stats();
		reset_decay_values();
	}

	inline	Group::Group(r_code::SysObject	*source):LObject(source,NULL),FastSemaphore(1,1){

		reset_ctrl_values();
		reset_stats();
		reset_decay_values();
	}

	inline	Group::~Group(){
	}

	inline	uint32	Group::get_upr(){

		return	code(GRP_UPR).asFloat();
	}

	inline	uint32	Group::get_spr(){

		return	code(GRP_SPR).asFloat();
	}

	inline	float32	Group::get_sln_thr(){

		return	code(GRP_SLN_THR).asFloat();
	}

	inline	float32	Group::get_act_thr(){

		return	code(GRP_ACT_THR).asFloat();
	}

	inline	float32	Group::get_vis_thr(){

		return	code(GRP_VIS_THR).asFloat();
	}

	inline	float32	Group::get_c_sln_thr(){

		return	code(GRP_C_SLN_THR).asFloat();
	}

	inline	float32	Group::get_c_act_thr(){

		return	code(GRP_C_ACT_THR).asFloat();
	}

	inline	float32	Group::get_c_sln(){

		return	code(GRP_C_SLN).asFloat();
	}

	inline	float32	Group::get_c_act(){

		return	code(GRP_C_ACT).asFloat();
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

		return	code(GRP_SLN_CHG_THR).asFloat();
	}

	inline	float32	Group::get_sln_chg_prd(){

		return	code(GRP_SLN_CHG_PRD).asFloat();
	}

	inline	float32	Group::get_act_chg_thr(){

		return	code(GRP_ACT_CHG_THR).asFloat();
	}

	inline	float32	Group::get_act_chg_prd(){

		return	code(GRP_ACT_CHG_PRD).asFloat();
	}

	inline	float32	Group::get_avg_sln(){

		return	code(GRP_AVG_SLN).asFloat();
	}

	inline	float32	Group::get_high_sln(){

		return	code(GRP_HIGH_SLN).asFloat();
	}

	inline	float32	Group::get_low_sln(){

		return	code(GRP_LOW_SLN).asFloat();
	}

	inline	float32	Group::get_avg_act(){

		return	code(GRP_AVG_ACT).asFloat();
	}

	inline	float32	Group::get_high_act(){

		return	code(GRP_HIGH_ACT).asFloat();
	}

	inline	float32	Group::get_low_act(){

		return	code(GRP_LOW_ACT).asFloat();
	}

	inline	float32	Group::get_high_sln_thr(){

		return	code(GRP_HIGH_SLN_THR).asFloat();
	}

	inline	float32	Group::get_low_sln_thr(){

		return	code(GRP_LOW_SLN_THR).asFloat();
	}

	inline	float32	Group::get_sln_ntf_prd(){

		return	code(GRP_SLN_NTF_PRD).asFloat();
	}

	inline	float32	Group::get_high_act_thr(){

		return	code(GRP_HIGH_ACT_THR).asFloat();
	}

	inline	float32	Group::get_low_act_thr(){

		return	code(GRP_LOW_ACT_THR).asFloat();
	}

	inline	float32	Group::get_act_ntf_prd(){

		return	code(GRP_ACT_NTF_PRD).asFloat();
	}

	inline	float32	Group::get_low_res_thr(){

		return	code(GRP_LOW_RES_THR).asFloat();
	}

	inline	float32	Group::get_ntf_new(){

		return	code(GRP_NTF_NEW).asFloat();
	}

	inline	Group	*Group::get_ntf_grp(){

		if(code(GRP_NTF_GRP).readsAsNil())
			return	this;

		uint32	index=code(GRP_NTF_GRP).asIndex();
		return	(Group	*)references(index);
	}

	inline	void	Group::_mod_0_positive(uint16	member_index,float32	value){

		float32	v=code(member_index).asFloat()+value;
		if(v<0)
			v=0;
		code(member_index)=Atom::Float(v);
	}

	inline	void	Group::_mod_0_plus1(uint16	member_index,float32	value){

		float32	v=code(member_index).asFloat()+value;
		if(v<0)
			v=0;
		else	if(v>1)
			v=1;
		code(member_index)=Atom::Float(v);
	}

	inline	void	Group::_mod_minus1_plus1(uint16	member_index,float32	value){

		float32	v=code(member_index).asFloat()+value;
		if(v<-1)
			v=-1;
		else	if(v>1)
			v=1;
		code(member_index)=Atom::Float(v);
	}

	inline	void	Group::_set_0_positive(uint16	member_index,float32	value){

		if(value<0)
			code(member_index)=Atom::Float(0);
		else
			code(member_index)=Atom::Float(value);
	}

	inline	void	Group::_set_0_plus1(uint16	member_index,float32	value){

		if(value<0)
			code(member_index)=Atom::Float(0);
		else	if(value>1)
			code(member_index)=Atom::Float(1);
		else
			code(member_index)=Atom::Float(value);
	}

	inline	void	Group::_set_minus1_plus1(uint16	member_index,float32	value){

		if(value<-1)
			code(member_index)=Atom::Float(-1);
		else	if(value>1)
			code(member_index)=Atom::Float(1);
		else
			code(member_index)=Atom::Float(value);
	}

	inline	void	Group::_set_0_1(uint16	member_index,float32	value){

		if(value==0	||	value==1)
			code(member_index)=Atom::Float(value);
	}

	inline	void	Group::mod(uint16	member_index,float32	value){

		switch(member_index){
		case	GRP_UPR:
		case	GRP_SPR:
		case	GRP_DCY_PRD:
		case	GRP_SLN_CHG_PRD:
		case	GRP_ACT_CHG_PRD:
		case	GRP_SLN_NTF_PRD:
		case	GRP_ACT_NTF_PRD:
			_mod_0_positive(member_index,value);
			return	;
		case	GRP_SLN_THR:
			mod_sln_thr(value);
			return;
		case	GRP_ACT_THR:
			mod_act_thr(value);
			return;
		case	GRP_VIS_THR:
			mod_vis_thr(value);
			return;
		case	GRP_C_SLN:
			mod_c_sln(value);
			return;
		case	GRP_C_SLN_THR:
			mod_c_sln_thr(value);
			return;
		case	GRP_C_ACT:
			mod_c_act(value);
			return;
		case	GRP_C_ACT_THR:
			mod_c_act_thr(value);
			return;
		case	GRP_DCY_PER:
			_mod_minus1_plus1(member_index,value);
			return;
		case	GRP_SLN_CHG_THR:
		case	GRP_ACT_CHG_THR:
		case	GRP_HIGH_SLN_THR:
		case	GRP_LOW_SLN_THR:
		case	GRP_HIGH_ACT_THR:
		case	GRP_LOW_ACT_THR:
		case	GRP_LOW_RES_THR:
			_mod_0_plus1(member_index,value);
			return;
		}
	}

	inline	void	Group::set(uint16	member_index,float32	value){
		
		switch(member_index){
		case	GRP_UPR:
		case	GRP_SPR:
		case	GRP_DCY_PRD:
		case	GRP_SLN_CHG_PRD:
		case	GRP_ACT_CHG_PRD:
		case	GRP_SLN_NTF_PRD:
		case	GRP_ACT_NTF_PRD:
			_set_0_positive(member_index,value);
			return;
		case	GRP_SLN_THR:
			set_sln_thr(value);
			return;
		case	GRP_ACT_THR:
			set_act_thr(value);
			return;
		case	GRP_VIS_THR:
			set_vis_thr(value);
			return;
		case	GRP_C_SLN:
			set_c_sln(value);
			return;
		case	GRP_C_SLN_THR:
			set_c_sln_thr(value);
			return;
		case	GRP_C_ACT:
			set_c_act(value);
			return;
		case	GRP_C_ACT_THR:
			set_c_act_thr(value);
			return;
		case	GRP_DCY_PER:
			_set_minus1_plus1(member_index,value);
			return;
		case	GRP_SLN_CHG_THR:
		case	GRP_ACT_CHG_THR:
		case	GRP_HIGH_SLN_THR:
		case	GRP_LOW_SLN_THR:
		case	GRP_HIGH_ACT_THR:
		case	GRP_LOW_ACT_THR:
		case	GRP_LOW_RES_THR:
			_set_0_plus1(member_index,value);
			return;
		case	GRP_NTF_NEW:
		case	GRP_DCY_TGT:
			_set_0_1(member_index,value);
			return;
		}
	}
}