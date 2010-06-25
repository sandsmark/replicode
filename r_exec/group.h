#ifndef	group_h
#define	group_h

#include	"../CoreLibrary/utils.h"
#include	"object.h"
#include	"view.h"


namespace	r_exec{

	class	_Mem;

	//	Shared resources:
	//		all parameters: accessed by Mem::update and reduction cores (via overlay mod/set).
	//		all views: accessed by Mem::update and reduction cores.
	//		viewing_groups: accessed by Mem::injectNow and Mem::update.
	class	r_exec_dll	Group:
	public	LObject,
	public	FastSemaphore{
	private:
		//	Ctrl values.
		uint32	sln_thr_changes;
		float32	acc_sln_thr;
		uint32	act_thr_changes;
		float32	acc_act_thr;
		uint32	vis_thr_changes;
		float32	acc_vis_thr;
		uint32	c_sln_changes;
		float32	acc_c_sln;
		uint32	c_act_changes;
		float32	acc_c_act;
		uint32	c_sln_thr_changes;
		float32	acc_c_sln_thr;
		uint32	c_act_thr_changes;
		float32	acc_c_act_thr;
		void	reset_ctrl_values();

		//	Stats.
		float32	avg_sln;
		float32	high_sln;
		float32	low_sln;
		float32	avg_act;
		float32	high_act;
		float32	low_act;
		uint32	sln_updates;
		uint32	act_updates;

		//	Decay.
		float32	sln_decay;
		float32	sln_thr_decay;
		int32	decay_periods_to_go;
		float32	decay_percentage_per_period;
		float32	decay_target;	//	-1: none, 0: sln, 1:sln_thr
		void	reset_decay_values();

		//	Notifications.
		int32	sln_change_monitoring_periods_to_go;
		int32	act_change_monitoring_periods_to_go;

		void	_mod_0_positive(uint16	member_index,float32	value);
		void	_mod_0_plus1(uint16	member_index,float32	value);
		void	_mod_minus1_plus1(uint16	member_index,float32	value);
		void	_set_0_positive(uint16	member_index,float32	value);
		void	_set_0_plus1(uint16	member_index,float32	value);
		void	_set_minus1_plus1(uint16	member_index,float32	value);
		void	_set_0_1(uint16	member_index,float32	value);
	public:
		//	xxx_views are meant for erasing views with res==0. They are specialized by type to ease update operations.
		//	Active overlays are to be found in xxx_ipgm_views.
		UNORDERED_MAP<uint32,P<View> >	ipgm_views;
		UNORDERED_MAP<uint32,P<View> >	anti_ipgm_views;
		UNORDERED_MAP<uint32,P<View> >	input_less_ipgm_views;
		UNORDERED_MAP<uint32,P<View> >	notification_views;
		UNORDERED_MAP<uint32,P<View> >	group_views;
		UNORDERED_MAP<uint32,P<View> >	other_views;

		//	Defined to create reduction jobs in the viewing groups from the viewed group.
		//	Empty when the viewed group is invisible (this means that visible groups can be non c-active or non c-salient).
		//	Maintained by the viewing groups (at update time).
		//	Viewing groups are c-active and c-salient. the bool is the cov.
		UNORDERED_MAP<Group	*,bool>		viewing_groups;

		//	Populated within update; cleared at the beginning of update.
		std::vector<View	*>			newly_salient_views;

		//	Populated upon ipgm injection; used at update time; cleared afterward.
		std::vector<IPGMController	*>	new_controllers;

		typedef	enum{
			MOD=0,
			SET=1
		}OpType;

		class	PendingOperation{
		public:
			PendingOperation(uint32	oid,uint16	member_index,OpType	operation,float32	value):oid(oid),member_index(member_index),operation(operation),value(value){}
			uint32	oid;		//	of the view.
			uint16	member_index;
			OpType	operation;
			float32	value;
		};

		//	Pending mod/set operations on the group's view, exploited and cleared at update time.
		std::vector<PendingOperation>	pending_operations;

		Group();
		Group(r_code::SysObject	*source);
		~Group();

		bool	all_views_cond(UNORDERED_MAP<uint32,P<View> >::const_iterator	&it,UNORDERED_MAP<uint32,P<View> >::const_iterator	&end){
			static	uint8	selector=0;
			while(it==end){
				switch(selector++){
				case	0:
					it=anti_ipgm_views.begin();
					end=anti_ipgm_views.end();
					break;
				case	1:
					it=input_less_ipgm_views.begin();
					end=input_less_ipgm_views.end();
					break;
				case	2:
					it=notification_views.begin();
					end=notification_views.end();
					break;
				case	3:
					it=group_views.begin();
					end=group_views.end();
					break;
				case	4:
					it=other_views.begin();
					end=other_views.end();
					break;
				case	5:
					selector=0;
					return	false;
				}
			}
			return	true;
		}

#define	FOR_ALL_VIEWS_BEGIN(g,it)	{	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	it=g->ipgm_views.begin();	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	end=g->ipgm_views.end();	\
		for(;g->all_views_cond(it,end);++it){

#define	FOR_ALL_VIEWS_BEGIN_NO_INC(g,it)	{	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	it=g->ipgm_views.begin();	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	end=g->ipgm_views.end();	\
		for(;g->all_views_cond(it,end);){
			
#define	FOR_ALL_VIEWS_END	}	\
		}

		bool	ipgm_views_with_inputs_cond(UNORDERED_MAP<uint32,P<View> >::const_iterator	&it,UNORDERED_MAP<uint32,P<View> >::const_iterator	&end){
			static	uint8	selector=0;
			while(it==end){
				switch(selector++){
				case	0:
					it=anti_ipgm_views.begin();
					end=anti_ipgm_views.end();
					break;
				case	1:
					selector=0;
					return	false;
				}
			}
			return	true;
		}

#define	FOR_ALL_IPGM_VIEWS_WITH_INPUTS_BEGIN(g,it)	{	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	it=g->ipgm_views.begin();	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	end=g->ipgm_views.end();	\
		for(;g->ipgm_views_with_inputs_cond(it,end);++it){

#define	FOR_ALL_IPGM_VIEWS_WITH_INPUTS_END	}	\
		}	

		bool	non_ntf_views_cond(UNORDERED_MAP<uint32,P<View> >::const_iterator	&it,UNORDERED_MAP<uint32,P<View> >::const_iterator	&end){
			static	uint8	selector=0;
			while(it==end){
				switch(selector++){
				case	0:
					it=anti_ipgm_views.begin();
					end=anti_ipgm_views.end();
					break;
				case	1:
					it=input_less_ipgm_views.begin();
					end=input_less_ipgm_views.end();
					break;
				case	2:
					it=group_views.begin();
					end=group_views.end();
					break;
				case	3:
					it=other_views.begin();
					end=other_views.end();
					break;
				case	4:
					selector=0;
					return	false;
				}
			}
			return	true;
		}

#define	FOR_ALL_NON_NTF_VIEWS_BEGIN(g,it)	{	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	it=g->ipgm_views.begin();	\
		UNORDERED_MAP<uint32,P<View> >::const_iterator	end=g->ipgm_views.end();	\
		for(;g->non_ntf_views_cond(it,end);++it){
			
#define	FOR_ALL_NON_NTF_VIEWS_END	}	\
		}

		View	*get_view(uint32	OID);

		uint32	get_upr();
		uint32	get_spr();

		float32	get_sln_thr();
		float32	get_act_thr();
		float32	get_vis_thr();

		float32	get_c_sln();
		float32	get_c_act();

		float32	get_c_sln_thr();
		float32	get_c_act_thr();

		void	mod_sln_thr(float32	value);
		void	set_sln_thr(float32	value);
		void	mod_act_thr(float32	value);
		void	set_act_thr(float32	value);
		void	mod_vis_thr(float32	value);
		void	set_vis_thr(float32	value);
		void	mod_c_sln(float32	value);
		void	set_c_sln(float32	value);
		void	mod_c_act(float32	value);
		void	set_c_act(float32	value);
		void	mod_c_sln_thr(float32	value);
		void	set_c_sln_thr(float32	value);
		void	mod_c_act_thr(float32	value);
		void	set_c_act_thr(float32	value);

		float32	update_sln_thr();	//	computes and applies decay on sln thr if any.
		float32	update_act_thr();
		float32	update_vis_thr();
		float32	update_c_sln();
		float32	update_c_act();
		float32	update_c_sln_thr();
		float32	update_c_act_thr();

		float32	get_sln_chg_thr();
		float32	get_sln_chg_prd();
		float32	get_act_chg_thr();
		float32	get_act_chg_prd();

		float32	get_avg_sln();
		float32	get_high_sln();
		float32	get_low_sln();
		float32	get_avg_act();
		float32	get_high_act();
		float32	get_low_act();

		float32	get_high_sln_thr();
		float32	get_low_sln_thr();
		float32	get_sln_ntf_prd();
		float32	get_high_act_thr();
		float32	get_low_act_thr();
		float32	get_act_ntf_prd();
		float32	get_low_res_thr();
		float32	get_res_ntf_prd();

		float32	get_ntf_new();

		Group	*get_ntf_grp();

		//	Delegate to views; update stats and notifies.
		float32	update_res(View	*v,_Mem	*mem);
		float32	update_sln(View	*v,float32	&change,_Mem	*mem);	//	applies decay if any.
		float32	update_act(View	*v,_Mem	*mem);

		//	Target upr, spr, c_sln, c_act, sln_thr, act_thr, vis_thr, c_sln_thr, c-act_thr, sln_chg_thr,
		//	sln_chg_prd, act_chg_thr, act_chg_prd, high_sln_thr, low_sln_thr, sln_ntf_prd, high_act_thr, low_act_thr, act_ntf_prd, low_res_thr, res_ntf_prd, ntf_new,
		//	dcy_per, dcy-tgt, dcy_prd.
		void	mod(uint16	member_index,float32	value);
		void	set(uint16	member_index,float32	value);

		void	reset_stats();				//	called at the begining of an update.
		void	update_stats(_Mem	*m);	//	at the end of an update; may produce notifcations.

		class	Hash{
		public:
			size_t	operator	()(Group	*g)	const{
				return	(size_t)g;
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	Group	*lhs,const	Group	*rhs)	const{
				return	lhs==rhs;
			}
		};
	};
}


#include	"group.inline.cpp"


#endif