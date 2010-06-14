#ifndef	r_exec_object_h
#define	r_exec_object_h

#include	"../r_code/object.h"
#include	"pgm_overlay.h"
#include	"../CoreLibrary/utils.h"


namespace	r_exec{

	class	Mem;
	class	View;
	class	Group;

	typedef	enum{
		GROUP=0,
		MARKER=1,
		IPGM=2,
		ANTI_IPGM=3,
		INPUT_LESS_IPGM=4,
		OTHER=5
	}ObjectType;

	//	Shared resources:
	//		view_map: accessed by Mem::injectNow (via various sub calls) and Mem::update.
	//		psln_thr: accessed by reduction cores (via overlay mod/set).
	//		marker_set: accessed by Mem::injectNow ans Mem::_initiate_sln_propagation.
	class	Object:
	public	r_code::Object{
	private:
		uint32	OID;
		size_t	hash_value;
		FastSemaphore	*psln_thr_sem;
		FastSemaphore	*view_map_sem;
		FastSemaphore	*marker_set_sem;
	protected:
		Mem		*mem;
	public:
		//	Opcodes are initialized by Mem::Init().
		static	uint16	GroupOpcode;

		static	uint16	PTNOpcode;
		static	uint16	AntiPTNOpcode;

		static	uint16	IPGMOpcode;
		static	uint16	PGMOpcode;
		static	uint16	AntiPGMOpcode;

		static	uint16	IGoalOpcode;
		static	uint16	GoalOpcode;
		static	uint16	AntiGoalOpcode;

		static	uint16	MkRdx;
		static	uint16	MkAntiRdx;

		static	uint16	MkNewOpcode;

		static	uint16	MkLowResOpcode;
		static	uint16	MkLowSlnOpcode;
		static	uint16	MkHighSlnOpcode;
		static	uint16	MkLowActOpcode;
		static	uint16	MkHighActOpcode;
		static	uint16	MkSlnChgOpcode;
		static	uint16	MkActChgOpcode;

		static	uint16	InjectOpcode;
		static	uint16	EjectOpcode;
		static	uint16	ModOpcode;
		static	uint16	SetOpcode;
		static	uint16	NewClassOpcode;
		static	uint16	DelClassOpcode;
		static	uint16	LDCOpcode;
		static	uint16	SwapOpcode;
		static	uint16	NewDevOpcode;
		static	uint16	DelDevOpcode;
		static	uint16	SuspendOpcode;
		static	uint16	ResumeOpcode;
		static	uint16	StopOpcode;
	
		UNORDERED_MAP<Group	*,View	*>	view_map;	//	to retrieve a view given a group (Cf operator fwv).

		Object();
		Object(r_code::SysObject	*source,Mem	*m);
		virtual	~Object();	//	un-registers from the rMem's object-map and object_io_map.

		void	computeHashValue();

		virtual	ObjectType	getType()	const;
		virtual	bool		isIPGM()	const;

		float32	get_psln_thr();

		uint32	getOID()	const;
		void	init(uint32	OID);

		void	acq_view_map()	const{	view_map_sem->acquire();	}
		void	rel_view_map()	const{	view_map_sem->release();	}
		void	acq_marker_set()	const{	marker_set_sem->acquire();	}
		void	rel_marker_set()	const{	marker_set_sem->release();	}

		//	Target psln_thr only.
		virtual	void	set(uint16	member_index,float32	value);
		virtual	void	mod(uint16	member_index,float32	value);

		class	Hash{
		public:
			size_t	operator	()(Object	*o)	const{
				if(o->hash_value==0)
					o->computeHashValue();
				return	o->hash_value;
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	Object	*lhs,const	Object	*rhs)	const{
				if(lhs->reference_set.size()!=rhs->reference_set.size())
					return	false;
				if(lhs->code.size()!=rhs->code.size())
					return	false;
				uint32	i;
				for(i=0;i<lhs->reference_set.size();++i)
					if((*lhs->reference_set.as_std())[i]!=(*rhs->reference_set.as_std())[i])
						return	false;
				for(i=0;i<lhs->code.size();++i)
					if((*lhs->code.as_std())[i]!=(*rhs->code.as_std())[i])
						return	false;
				return	true;
			}
		};
	};

	//	OID is the 2nd atom in code.
	//	Shared resources:
	//		none: all mod/set operations are pushed on the group and executed at update time.
	class	View:
	public	r_code::View{
	private:
		//	Ctrl values.
		uint32	sln_changes;
		float32	acc_sln;
		uint32	act_vis_changes;
		float32	acc_act_vis;
		uint32	res_changes;
		float32	acc_res;
		void	reset_ctrl_values();

		//	Monitoring
		float32	initial_sln;
		float32	initial_act;
	protected:
		static	uint32	LastOID;
		static	uint32	GetOID();

		void	reset_init_values();
	public:
		static	uint16	ViewOpcode;

		P<IPGMController>	controller;	//	built upon injection of the view (if the object is an ipgm).

		static	float32	MorphValue(float32	value,float32	source_thr,float32	destination_thr);
		static	float32	MorphChange(float32	change,float32	source_thr,float32	destination_thr);

		uint32	periods_at_low_sln;
		uint32	periods_at_high_sln;
		uint32	periods_at_low_act;
		uint32	periods_at_high_act;

		View();
		View(r_code::SysView	*source,r_code::Object	*object);
		View(View	*view,Group	*group);	//	copy the view and assigns it to the group (used for cov); morph ctrl values.
		View(View	*view);	//	simple copy.
		~View();

		uint32	getOID()	const;

		virtual	bool	isNotification()	const;

		Group	*getHost();

		uint64	get_ijt()	const;

		float32	get_res();
		float32	get_sln();
		float32	get_act_vis();
		bool	get_cov();

		void	mod_res(float32	value);
		void	set_res(float32	value);
		void	mod_sln(float32	value);
		void	set_sln(float32	value);
		void	mod_act_vis(float32	value);
		void	set_act_vis(float32	value);

		float32	update_res();
		float32	update_sln(float32	&change,float32	low,float32	high);
		float32	update_act(float32	low,float32	high);
		float32	update_vis();

		float32	update_sln_delta();
		float32	update_act_delta();

		//	Target res, sln, act, vis.
		void	mod(uint16	member_index,float32	value);
		void	set(uint16	member_index,float32	value);
	};

	//	Shared resources:
	//		all parameters: accessed by Mem::update and reduction cores (via overlay mod/set).
	//		all views: accessed by Mem::update and reduction cores.
	//		viewing_groups: accessed by Mem::injectNow and Mem::update.
	class	Group:
	public	Object,
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
		UNORDERED_MAP<uint32,r_code::P<View> >	ipgm_views;
		UNORDERED_MAP<uint32,r_code::P<View> >	anti_ipgm_views;
		UNORDERED_MAP<uint32,r_code::P<View> >	input_less_ipgm_views;
		UNORDERED_MAP<uint32,r_code::P<View> >	notification_views;
		UNORDERED_MAP<uint32,r_code::P<View> >	group_views;
		UNORDERED_MAP<uint32,r_code::P<View> >	other_views;

		//	Defined to create reduction jobs in the viewing groups from the viewed group.
		//	Empty when the viewed group is invisible (this means that visible groups can be non c-active or non c-salient).
		//	Maintained by the viewing groups (at update time).
		//	Viewing groups are c-active and c-salient. the bool is the cov.
		UNORDERED_MAP<Group	*,bool>								viewing_groups;

		//	Populated within update; cleared at the beginning of update.
		std::vector<View	*>									newly_salient_views;

		//	Populated upon ipgm injection; used at update time; cleared afterward.
		std::vector<IPGMController	*>							new_controllers;

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
		std::vector<PendingOperation>							pending_operations;

		Group();
		Group(r_code::SysObject	*source);
		~Group();

		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	views_begin()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	views_end()		const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	next_view(UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	&it)	const;

		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	input_less_ipgm_views_begin()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	input_less_ipgm_views_end()		const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	next_input_less_ipgm_view(UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	&it)	const;

		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	anti_ipgm_views_begin()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	anti_ipgm_views_end()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	next_anti_pgm_view(UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	&it)	const;

		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	ipgm_views_with_inputs_begin()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	ipgm_views_with_inputs_end()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	next_ipgm_view_with_inputs(UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	&it)	const;

		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	non_ntf_views_begin()	const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	non_ntf_views_end()		const;
		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	next_non_ntf_view(UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	&it)	const;

		View	*getView(uint32	OID);

		ObjectType	getType()	const;

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
		float32	update_res(View	*v,Mem	*mem);
		float32	update_sln(View	*v,float32	&change,Mem	*mem);	//	applies decay if any.
		float32	update_act(View	*v,Mem	*mem);

		//	Target upr, spr, c_sln, c_act, sln_thr, act_thr, vis_thr, c_sln_thr, c-act_thr, sln_chg_thr,
		//	sln_chg_prd, act_chg_thr, act_chg_prd, high_sln_thr, low_sln_thr, sln_ntf_prd, high_act_thr, low_act_thr, act_ntf_prd, low_res_thr, res_ntf_prd, ntf_new,
		//	dcy_per, dcy-tgt, dcy_prd.
		void	mod(uint16	member_index,float32	value);
		void	set(uint16	member_index,float32	value);

		void	reset_stats();				//	called at the begining of an update.
		void	update_stats(Mem	*m);	//	at the end of an update; may produce notifcations.

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

	class	InstantiatedProgram:
	public	Object{
	public:
		InstantiatedProgram();
		InstantiatedProgram(r_code::SysObject	*source,Mem	*m);
		~InstantiatedProgram();

		ObjectType	getType()	const;
		bool		isIPGM()	const;

		Object	*getPGM()	const;
		uint64	get_tsc()	const;
	};

	class	NotificationView:
	public	View{
	public:
		NotificationView(Group	*origin,Group	*destination,Object	*marker);	//	res=1, sln=1.

		bool	isNotification()	const;
	};

	class	Marker:
	public	Object{
	protected:
		Marker();
	public:
		Marker(r_code::SysObject	*source,Mem	*m);
		~Marker();

		ObjectType	getType()	const;
	};

	class	MkNew:
	public	Marker{
	public:
		MkNew(Object	*object);
	};

	class	MkLowRes:
	public	Marker{
	public:
		MkLowRes(Object	*object);
	};

	class	MkLowSln:
	public	Marker{
	public:
		MkLowSln(Object	*object);
	};

	class	MkHighSln:
	public	Marker{
	public:
		MkHighSln(Object	*object);
	};

	class	MkLowAct:
	public	Marker{
	public:
		MkLowAct(Object	*object);
	};

	class	MkHighAct:
	public	Marker{
	public:
		MkHighAct(Object	*object);
	};

	class	MkSlnChg:
	public	Marker{
	public:
		MkSlnChg(Object	*object,float32	value);
	};

	class	MkActChg:
	public	Marker{
	public:
		MkActChg(Object	*object,float32	value);
	};
}


#endif