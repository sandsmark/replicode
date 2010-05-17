#ifndef	object_h
#define	object_h

#include	"atom.h"
#include	"vector.h"
#include	"base.h"


namespace	r_code{

	//	I/O from/to r_code::Image ////////////////////////////////////////////////////////////////////////

	class	dll_export	ImageObject{
	public:
		r_code::vector<Atom>	code;
		r_code::vector<uint32>	reference_set;	//	for views: 0, 1 or 2 elements; these are indexes in the relocation segment for grp (exception: not for root) and possibly org
												//	for sys-objects: any number
		virtual	void	write(word32	*data)=0;
		virtual	void	read(word32		*data)=0;
		virtual	void	trace()=0;
	};

	class	View;

	class	dll_export	SysView:
	public	ImageObject{
	public:
		SysView();
		SysView(View	*source);

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize()	const;
		void	trace();
	};

	class	Object;

	class	dll_export	SysObject:
	public	ImageObject{
	public:
		r_code::vector<uint32>		marker_set;		//	indexes in the relocation segment
		r_code::vector<SysView	*>	view_set;

		SysObject();
		SysObject(Object	*source);
		~SysObject();

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize();
		void	trace();
	};

	// Interfaces for r_exec classes ////////////////////////////////////////////////////////////////////////

	class	Mem{
	public:
		virtual	void	deleteObject(Object	*object)=0;
	};

	class	View;
	class	Group;

	class	dll_export	Object:
	public	_Object{
	private:
		uint64	sln_propagation_ts;	//	to detect (and prevent) propagation loops
		uint32	OID;
		Mem		*mem;
	public:
		r_code::vector<Atom>		code;
		r_code::vector<P<Object> >	marker_set;
		r_code::vector<P<Object> >	reference_set;
		r_code::vector<View	*>		view_set;

		UNORDERED_MAP<Group	*,View	*>	view_map;

		Object();
		Object(SysObject	*source);
		~Object();

		uint32	opcode();

		virtual	bool	isGroup()	const;
		uint32	getOID()	const;
		void	init(uint32	OID,Mem	*mem);
	};

	class	dll_export	Group:
	public	Object{
	public:
		r_code::vector<P<View> >	member_set;
		r_code::vector<Group	*>	visible_groups;	//	and c-salient

		Group();
		Group(SysObject	*source);
		~Group();

		bool	isGroup()	const;
	};

	class	dll_export	View:
	public	_Object{
	private:
		uint32	sln_changes;
		float32	acc_sln;
		uint32	act_vis_changes;
		float32	acc_act_vis;
		uint32	res_changes;
		int64	acc_res;
		void	reset_ctrl_values();
	public:
		P<Object>					object;
		r_code::vector<Atom>		code;
		r_code::vector<Object	*>	reference_set;

		View();
		View(SysView	*source,Object	*object);
		~View();

		Group	*getHost();

		void	mod_res(int64	value);
		void	set_res(int64	value);
		void	mod_sln(float32	value);
		void	set_sln(float32	value);
		void	mod_act_vis(float32	value);
		void	set_act_vis(float32	value);
		float32	update_res();
		float32	update_sln(float32	&change);
		float32	update_act_vis();
	};
}


#endif