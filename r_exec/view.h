#ifndef	view_h
#define	view_h

#include	"../r_code/object.h"
#include	"pgm_overlay.h"
#include	"dll.h"


namespace	r_exec{

	class	Group;
	class	LObject;

	//	OID is the 2nd atom in code.
	//	Shared resources:
	//		none: all mod/set operations are pushed on the group and executed at update time.
	class	r_exec_dll	View:
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
		View(r_code::SysView	*source,r_code::Code	*object);
		View(View	*view,Group	*group);	//	copy the view and assigns it to the group (used for cov); morph ctrl values.
		View(View	*view);	//	simple copy.
		~View();

		uint32	getOID()	const;

		virtual	bool	isNotification()	const;

		Group	*get_host();

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

		class	Hash{
		public:
			size_t	operator	()(View	*v)	const{
				return	(size_t)(Code	*)v->object;
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	View	*lhs,const	View	*rhs)	const{
				return	lhs->object==rhs->object;
			}
		};
	};

	class	r_exec_dll	NotificationView:
	public	View{
	public:
		NotificationView(Group	*origin,Group	*destination,LObject	*marker);	//	res=1, sln=1.

		bool	isNotification()	const;
	};
}


#include	"view.inline.cpp"


#endif